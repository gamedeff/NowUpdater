//
// NowUpdater
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//

#include "nu_http_session.h"

#include "Poco/InflatingStream.h"
#include "Poco/Net/MediaType.h"
#include "Poco/StreamConverter.h"
#include "Poco/LineEndingConverter.h"
#include "Poco/TextEncoding.h"
#include "Poco/Latin1Encoding.h"
#include "Poco/UTF8String.h"
#include "Poco/JSON/Parser.h"
#include "Poco/JSON/Object.h"

#include "Poco/Net/HTTPBasicCredentials.h"

#include "Poco/Net/HTTPSClientSession.h"

#include "Poco/Net/HTTPSessionFactory.h"
#include "Poco/Net/HTTPSessionInstantiator.h"
#include "Poco/Net/HTTPSSessionInstantiator.h"
#include "Poco/Net/AcceptCertificateHandler.h"
#include "Poco/Net/SSLManager.h"

using Poco::Net::HTTPSessionFactory;
using Poco::Net::HTTPSessionInstantiator;
using Poco::Net::HTTPSSessionInstantiator;
using Poco::Net::AcceptCertificateHandler;
using Poco::Net::SSLManager;

#include <iostream>
#include <sstream>

#include "for_each.h"
#include "util.h"

#include <fstream>

#ifdef __MACH__
#include <CoreFoundation/CoreFoundation.h>  // NOLINT
#include <CoreServices/CoreServices.h>  // NOLINT
#endif

#if defined(_WIN32) || defined(WIN32)
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif


#include "Poco/CountingStream.h"
#include "Poco/Net/MultipartWriter.h"

using Poco::Net::MultipartWriter;

class HTMLFormNU: public Poco::Net::HTMLForm
{
public:
	void prepareSubmit(HTTPRequest& request);
	void write(std::ostream& ostr);
protected:
	void writeUrl(std::ostream& ostr);
};

void HTMLFormNU::writeUrl(std::ostream& ostr)
{
	for(Poco::Net::NameValueCollection::ConstIterator it = begin(); it != end(); ++it)
	{
		if (it != begin()) ostr << "&";
		std::string name;
		URI::encode(it->first, "!?#/'\",;:$&()[]*=@", name);
		std::string value;
		URI::encode(it->second, "!?#/'\",;:$&()[]*=@", value);
		ostr << name << "=" << value;
	}
}

void HTMLFormNU::prepareSubmit(HTTPRequest& request)
{
	if (request.getMethod() == HTTPRequest::HTTP_POST || request.getMethod() == HTTPRequest::HTTP_PUT)
	{
		if (getEncoding() == ENCODING_URL)
		{
			request.setContentType(getEncoding());
			request.setChunkedTransferEncoding(false);
			Poco::CountingOutputStream ostr;
			writeUrl(ostr);
			request.setContentLength(ostr.chars());
		}
		else
		{
			HTMLForm::prepareSubmit(request);
		}
	}
	else
	{
		std::string uri = request.getURI();
		std::ostringstream ostr;
		writeUrl(ostr);
		uri.append("?");
		uri.append(ostr.str());
		request.setURI(uri);
	}
}

void HTMLFormNU::write(std::ostream& ostr)
{
	if (getEncoding() == ENCODING_URL)
		writeUrl(ostr);
	else
		writeMultipart(ostr);
}


void print_nvc(Poco::Net::NameValueCollection &nvc)
{
	std::string name, value;
	Poco::Net::NameValueCollection::ConstIterator it = nvc.begin();
	while(it != nvc.end())
	{
		name  = it->first;
		value = it->second;

		std::cout << name << "=\"" << value << "\"" << std::endl << std::flush;
		++it;
	}
}

void print_response(std::istream& rs, HTTPResponse &response, URI uri, HTTPRequest &request)
{
	std::cout << "============================================================" << std::endl;
	std::cout << request.getMethod() << " " << uri.getPathAndQuery() << std::endl;
	std::cout << "============================================================" << std::endl;
	std::cout << "==> REQUEST HEADERS:" << std::endl;
	std::cout << "------------------------------------------------------------" << std::endl;
	print_nvc(request);

	std::cout << "------------------------------------------------------------" << std::endl;
	std::cout << response.getStatus() << " " << response.getReason() << std::endl;
	std::cout << "------------------------------------------------------------" << std::endl;
	std::cout << "<== RESPONSE HEADERS:" << std::endl;
	std::cout << "------------------------------------------------------------" << std::endl;
	print_nvc(response);

	std::vector<HTTPCookie> cookies;
	response.getCookies(cookies);
	std::cout << "------------------------------------------------------------" << std::endl;
	std::cout << "<== RESPONSE COOKIES:" << std::endl;
	std::cout << "------------------------------------------------------------" << std::endl;
	std::vector<HTTPCookie>::const_iterator it = cookies.begin();
	while(it != cookies.end())
	{
		std::cout << it->toString() << std::endl << std::flush;
		++it;
	}
	//std::ostringstream stream;
	//StreamCopier::copyStream(rs, stream);
	//std::cout << stream.str() << std::endl;
	////std::cout << res.rdbuf();  // dump server response so you can view it
}

// Converts the vector of HTTPCookie objects into a NameValueCollection
Poco::Net::NameValueCollection ConvertCookies(std::vector<Poco::Net::HTTPCookie> cookies)
{
	Poco::Net::NameValueCollection nvc;
	//std::vector<Poco::Net::HTTPCookie>::const_iterator it = cookies.begin();
	std::vector<Poco::Net::HTTPCookie>::const_reverse_iterator it = cookies.rbegin();
	while(it != cookies.rend())
	{
		nvc.add((*it).getName(), (*it).getValue());
		++it;
	}
	return nvc;
}

http_session_t::http_session_t(std::string site) : domain(site), uri("http://" + domain)//, session(domain)
{
//	Poco::Net::initializeSSL();

	// Register http and https
	HTTPSessionFactory::defaultFactory().registerProtocol("http",  new HTTPSessionInstantiator);
	HTTPSessionFactory::defaultFactory().registerProtocol("https", new HTTPSSessionInstantiator);
	// Prepare for SSLManager
	Poco::SharedPtr<AcceptCertificateHandler> ptrCert = new AcceptCertificateHandler(false);
	//const Poco::Net::Context::Ptr context = new Context(Context::CLIENT_USE, "", "", "", Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
	//const Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
	const Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "");
	SSLManager::instance().initializeClient(0, ptrCert, context);
	// Now you have the HTTP(S)ClientSession
	session = HTTPSessionFactory::defaultFactory().createClientSession(uri);
}

std::string http_session_t::receive_response(HTTPRequest &request, const std::string &cookie_name)
{
	// Get and print out the response from the server
	std::istream& rs = session->receiveResponse(response);

	return receive_response(rs, request, cookie_name);
}

std::string http_session_t::receive_response(std::istream& rs, HTTPRequest &request, const std::string &cookie_name)
{
	print_response(rs, response, uri, request);

	//if(response.getStatus() == HTTPResponse::HTTP_MOVED_PERMANENTLY || response.has("Location"))
	if(response.getStatus() == HTTPResponse::HTTP_MOVED_PERMANENTLY && response.has("Location"))
		handle_redirection(cookie_name);

	Poco::InflatingInputStream zlib(rs, Poco::InflatingStreamBuf::STREAM_ZLIB);
	Poco::InflatingInputStream gzip(rs, Poco::InflatingStreamBuf::STREAM_GZIP);

	std::istream* prs = &rs;

	if(response.has("Content-Encoding"))
	{
		std::string encoding_type = response.get("Content-Encoding");
		if(encoding_type == "gzip")
			prs = &gzip;
		else if(encoding_type == "deflate")
			prs = &zlib;
		else
			return "ERR_WRONG_COMPRESSION";
	}

	Poco::Net::MediaType media_type(response.getContentType());

	std::ostringstream stream;

	//if(media_type.hasParameter("charset"))
	//{
	//	Poco::Latin1Encoding latin1;
	//	Poco::InputStreamConverter encoding_converter(*prs, Poco::TextEncoding::byName(media_type.getParameter("charset")), latin1);

	//	//Poco::InputLineEndingConverter line_ending_converter(encoding_converter);

	//	StreamCopier::copyStream(/*line_ending_converter*/encoding_converter, stream);
	//}
	//else
	StreamCopier::copyStream(*prs, stream);

	// this /|\ shit is broken, so...
	if(media_type.hasParameter("charset") && media_type.getParameter("charset") == "UTF-8")
	{
		std::string str = stream.str();
		Poco::UTF8::removeBOM(str);
		return str;
	}
	else return stream.str();
}

std::streamsize http_session_t::receive_response(std::istream& rs, std::ostream& ostr, HTTPRequest &request, const std::string &cookie_name)
{
	print_response(rs, response, uri, request);

	if(response.getStatus() == HTTPResponse::HTTP_MOVED_PERMANENTLY || response.has("Location"))
		handle_redirection(cookie_name);

	Poco::InflatingInputStream zlib(rs, Poco::InflatingStreamBuf::STREAM_ZLIB);
	Poco::InflatingInputStream gzip(rs, Poco::InflatingStreamBuf::STREAM_GZIP);

	std::istream* prs = &rs;

	if(response.has("Content-Encoding"))
	{
		std::string encoding_type = response.get("Content-Encoding");
		if(encoding_type == "gzip")
			prs = &gzip;
		else if(encoding_type == "deflate")
			prs = &zlib;
		else
			return 0; // "ERR_WRONG_COMPRESSION";
	}

	return StreamCopier::copyStream(*prs, ostr);
}


void json_encode(const std::string& str, const std::string& reserved, std::string& encodedStr)
{
	for(std::string::const_iterator it = str.begin(); it != str.end(); ++it)
	{
		char c = *it;
		if ((c >= 'a' && c <= 'z') || 
			(c >= 'A' && c <= 'Z') || 
			(c >= '0' && c <= '9') ||
			c == '-' || c == '_' || 
			c == '.' || c == '~')
		{
			encodedStr += c;
		}
		else
		{
			encodedStr += '%';
			encodedStr += Poco::NumberFormatter::formatHex((unsigned) (unsigned char) c, 2);
		}
	}
}

bool http_session_t::send_json_request(const std::string &uri, const std::string &json_request_str, const std::string &cookie_name)
{
	try
	{
		std::string json_encoded_request_str;
		json_encode(json_request_str, "", json_encoded_request_str);

		std::string request_str = "ajax=" + json_encoded_request_str;
		//std::string request_str_utf8;
		//Poco::Latin1Encoding latin1;
		//Poco::UTF8Encoding utf8;
		//Poco::TextConverter to_utf8(latin1, utf8);
		//to_utf8.convert(request_str, request_str_utf8);
		std::cout << json_request_str << std::endl;
		std::cout << request_str << std::endl;

		std::string json_response_str = send_request(HTTPRequest::HTTP_POST, uri, request_str, cookie_name);
		//replace_whitespace(json_response_str);
		std::string json_decoded_response_str;
		Poco::URI::decode(json_response_str, json_decoded_response_str);
		Poco::JSON::Parser parser;
		Poco::Dynamic::Var json_response = parser.parse(json_decoded_response_str);

		// use pointers to avoid copying
		Poco::JSON::Object::Ptr object = json_response.extract<Poco::JSON::Object::Ptr>();
		Poco::JSON::Object::Ptr response_object = object->getObject("response"); // holds { "property" : "value" }
		if(response_object)
		{
			if(response_object->get("success"))
			{
				return true;
			}
			else
			{
				Poco::Dynamic::Var errorMessage = response_object->get("error");
				if(!errorMessage.isEmpty())
				{
					std::cout << "response error message: " << errorMessage.extract<std::string>() << std::endl;
				}
			}
		}
		else
		{
#ifdef _DEBUG

			Poco::Dynamic::Var errorMessage = object->get("errorMessage");
			if(!errorMessage.isEmpty())
			{
				std::cout << "json error message: " << errorMessage.extract<std::string>() << std::endl;
			}

			Poco::Dynamic::Var errorCode = object->get("errorCode");
			if(!errorCode.isEmpty())
			{
				std::cout << "json error code: " << errorCode.extract<int>() << std::endl;
			}
#endif
		}
	}
	catch(...)
	{
		return false;
	}

	return false;
}

void http_session_t::fill_request(HTTPRequest &request, const std::string &accept, const std::string &content_type, const std::string &charset, const std::string &useragent, const std::string &referer_url, uint32_t content_size, const std::string &cookie_name)
{
	if(!useragent.empty())
		request.add("User-Agent", useragent);

	//request.add("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
	if(!accept.empty())
		request.add("Accept", accept);
	else
		request.add("Accept", "*/*");

	request.add("Accept-Language", "en-US,en;q=0.5");
	request.add("Accept-Encoding", "gzip, deflate");

	if(!content_type.empty())
	{
		if(!charset.empty())
		{
			Poco::Net::MediaType media_type(content_type);

			media_type.setParameter("charset", charset);

			request.setContentType(media_type);
		}
		else
			request.setContentType(content_type);
	}

	//request.setKeepAlive(true); // notice setKeepAlive is also called on session (above)
	request.set(Poco::Net::HTTPMessage::CONNECTION, "keep-alive");

	if(!referer_url.empty())
		request.add("Referer", referer_url);

	if(!cookie_name.empty())
		request.add(HTTPRequest::COOKIE, update_cookies(cookie_name));

	if(content_size > 0)
		request.setContentLength(content_size);
}

std::ostream& http_session_t::send_request(HTTPRequest &request, const std::string &content_type, const std::string &charset, const std::string &useragent, const std::string &referer_url, uint32_t content_size, const std::string &cookie_name)
{
	fill_request(request, "", content_type, charset, useragent, referer_url, content_size, cookie_name);

	return session->sendRequest(request); // sends request, returns open stream
}

std::string http_session_t::send_request(HTTPRequest &request, const std::string &content_type, const std::string &charset, const std::string &useragent, const std::string &referer_url, const std::vector<string_pair_t> &params, const std::string &username, const std::string &password, const std::string &cookie_name)
{
	if(!username.empty() && !password.empty())
	{
		//if(response.getStatus() == Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED)
		//{
		//	Poco::Net::HTTPCredentials credentials(username, password);
		//	credentials.authenticate(request, response);
		//}

		Poco::Net::HTTPBasicCredentials credentials(username, password);
		credentials.authenticate(request);
	}

	fill_request(request, "", content_type, charset, useragent, referer_url, 0, cookie_name);

	//std::vector<HTTPCookie> cookies;
	//response.getCookies(cookies);

	//request.setCookies(ConvertCookies(cookies));
	update_combined_cookies();
	if(!cookie_name.empty())
	{
		std::vector<HTTPCookie> current_cookies;

		std::vector<std::string> cookie_names = split(cookie_name, '|');
		for(int i = 0; i < cookie_names.size(); ++i)
			for(std::vector<HTTPCookie>::iterator combined_cookies_it = combined_cookies.begin(); combined_cookies_it != combined_cookies.end(); ++combined_cookies_it)
				if(combined_cookies_it->getName() == cookie_names[i])
				{
					current_cookies.push_back(*combined_cookies_it);
					break;
				}

		request.setCookies(ConvertCookies(current_cookies));
	}
	else
		request.setCookies(ConvertCookies(combined_cookies));

	// Create a new HTMLForm to submit to and add some string data
	//Poco::Net::HTMLForm form;
	HTMLFormNU form;
	FOR_EACH(const string_pair_t &param, params)
		form.add(param.first, param.second);

	if(!params.empty())
		form.prepareSubmit(request); // Fill out the request object and write to output stream

	std::ostream& send = session->sendRequest(request); // sends request, returns open stream

	if(!params.empty())
		form.write(send);

	return receive_response(request, cookie_name);
}


void http_session_t::send_request(HTTPRequest &request, const std::string &content_type, const std::string &charset, const std::string &useragent, const std::string &referer_url, const std::string &request_body, const std::string &cookie_name)
{
	std::ostream& send = send_request(request, content_type, charset, useragent, referer_url, request_body.length(), cookie_name);

	if(!request_body.empty())
	{
		if(!charset.empty())
		{
			Poco::Latin1Encoding latin1;
			Poco::OutputStreamConverter converter(send, latin1, Poco::TextEncoding::byName(charset));

			converter << request_body/* << std::endl*/;  // sends the body
		}
		else
			send << request_body/* << std::endl*/;  // sends the body
	}
}

//void http_session_t::send_request(HTTPRequest &request, const std::string &content_type, const std::string &charset, const std::string &useragent, const std::string &referer_url, const pugi::xml_node &request_body_node, const std::string &cookie_name)
//{
//	std::ostream& send = send_request(request, content_type, charset, useragent, referer_url, request_body_node.traverse(), cookie_name);
//
//	if(!request_body_node.empty())
//	{
//		if(!charset.empty())
//		{
//			Poco::Latin1Encoding latin1;
//			Poco::OutputStreamConverter converter(send, latin1, Poco::TextEncoding::byName(charset));
//
//			request_body_node.print(converter);  // sends the body
//		}
//		else
//			request_body_node.print(send);  // sends the body
//	}
//}

std::string http_session_t::send_request(const std::string &method, const std::string &request_url, const std::string &request_body, const std::string &cookie_name)
{
	std::string url = "http://" + domain + request_url;
	reset(url);

	HTTPRequest request(method, request_url, HTTPMessage::HTTP_1_1);

	send_request(request, "application/x-www-form-urlencoded", "UTF-8", "", url, request_body, cookie_name);

	return receive_response(request, cookie_name);
}


bool autodetect_proxy(std::vector<std::string> *proxy_strings)
{
    poco_assert(proxy_strings);

    proxy_strings->clear();

#if defined(_WIN32) || defined(WIN32)
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie_config = { 0 };
    if(!WinHttpGetIEProxyConfigForCurrentUser(&ie_config))
	{
        //std::stringstream ss;
        //ss << "WinHttpGetIEProxyConfigForCurrentUser error: "
        //   << GetLastError();
        //return ss.str();
		return false;
    }
    if(ie_config.lpszProxy)
	{
        std::wstring proxy_url_wide(ie_config.lpszProxy);
        std::string s("");
        Poco::UnicodeConverter::toUTF8(proxy_url_wide, s);
        if(s.find(';') == std::string::npos)
			proxy_strings->push_back(s);
		else
		{
			split(s, ';', *proxy_strings);
			for(std::vector<std::string>::iterator proxy_strings_it = (*proxy_strings).begin(); proxy_strings_it != (*proxy_strings).end(); ++proxy_strings_it)
			{				
				(*proxy_strings_it) = substr_after_first_of((*proxy_strings_it), '=');
			}
		}
    }
#endif

    // Inspired by VLC source code
    // https://github.com/videolan/vlc/blob/master/src/darwin/netconf.c
#ifdef __MACH__
    CFDictionaryRef dicRef = CFNetworkCopySystemProxySettings();
    if (NULL != dicRef) {
        const CFStringRef proxyCFstr = (const CFStringRef)CFDictionaryGetValue(
            dicRef, (const void*)kCFNetworkProxiesHTTPProxy);
        const CFNumberRef portCFnum = (const CFNumberRef)CFDictionaryGetValue(
            dicRef, (const void*)kCFNetworkProxiesHTTPPort);
        if (NULL != proxyCFstr && NULL != portCFnum) {
            int port = 0;
            if (!CFNumberGetValue(portCFnum, kCFNumberIntType, &port)) {
                CFRelease(dicRef);
                return noError;
            }

            const std::size_t kBufsize(4096);
            char host_buffer[kBufsize];
            memset(host_buffer, 0, sizeof(host_buffer));
            if (CFStringGetCString(proxyCFstr, host_buffer, sizeof(host_buffer)
                                   - 1, kCFStringEncodingUTF8)) {
                char buffer[kBufsize];
                snprintf(buffer, kBufsize, "%s:%d", host_buffer, port);
                proxy_strings->push_back(std::string(buffer));
            }
        }

        CFRelease(dicRef);
    }
#endif

    return true;
}

bool configure_proxy(Poco::Net::HTTPClientSession *session)
{
    std::string proxy_url("");

    if(proxy_url.empty())
	{
        std::vector<std::string> proxy_strings;
        if(!autodetect_proxy(&proxy_strings))
			return false;
        if(!proxy_strings.empty())
            proxy_url = proxy_strings[0];
    }

    if(!proxy_url.empty())
	{
        if(proxy_url.find("://") == std::string::npos)
            proxy_url = "http://" + proxy_url;

        Poco::URI proxy_uri(proxy_url);

        //std::stringstream ss;
        //ss << "Using proxy URI=" + proxy_uri.toString()
        //   << " host=" << proxy_uri.getHost()
        //   << " port=" << proxy_uri.getPort();
        //logger.debug(ss.str());

        session->setProxy(proxy_uri.getHost(), proxy_uri.getPort());

        if(!proxy_uri.getUserInfo().empty())
		{
            Poco::Net::HTTPCredentials credentials;
            credentials.fromUserInfo(proxy_uri.getUserInfo());
            session->setProxyCredentials(credentials.getUsername(), credentials.getPassword());

            //logger.debug("Proxy credentials detected username="
            //             + credentials.getUsername());
        }

		return true;
    }

	return false;
}

std::string http_session_t::send_request(const std::string &method, const std::string &request_url, const std::vector<string_pair_t> &params, const std::string &useragent, const std::string &username, const std::string &password, const std::string &cookie_name)
{
	//std::string url = "http://" + domain + request_url;
	//reset(url);

	uri.resolve(request_url);

	std::string path(uri.getPathAndQuery());
	if(path.empty()) path = "/";

	session = HTTPSessionFactory::defaultFactory().createClientSession(uri);
	configure_proxy(session);
	session->setKeepAlive(true);

	HTTPRequest request(method, path, HTTPMessage::HTTP_1_1);

	return send_request(request, method == HTTPRequest::HTTP_POST ? "application/x-www-form-urlencoded" : "", "UTF-8", useragent, request_url, params, username, password, cookie_name);
}

std::string http_session_t::send_request(const std::string &method, const std::string &request_url, const std::string &referer_url, const std::vector<string_pair_t> &params, const std::string &useragent, const std::string &username, const std::string &password, const std::string &cookie_name)
{
	//std::string url = "http://" + domain + request_url;
	//reset(url);

	uri.setHost(domain);
	uri.resolve(request_url);

	std::string path(uri.getPathAndQuery());
	if(path.empty()) path = "/";

	session = HTTPSessionFactory::defaultFactory().createClientSession(uri);
	configure_proxy(session);
	session->setKeepAlive(true);

	HTTPRequest request(method, path, HTTPMessage::HTTP_1_1);

	return send_request(request, method == HTTPRequest::HTTP_POST ? "application/x-www-form-urlencoded" : "", "UTF-8", useragent, referer_url, params, username, password, cookie_name);
}

std::string http_session_t::send_request(const std::string &method, const std::string &request_url, const string_pair_t &param, const std::string &useragent, const std::string &username, const std::string &password, const std::string &cookie_name)
{
	std::vector<string_pair_t> params;
	params.push_back(param);

	return send_request(method, request_url, params, useragent, username, password, cookie_name);
}

std::string http_session_t::send_request(const std::string &method, const std::string &useragent, const std::string &request_url, const std::string &request_body, const std::string &username, const std::string &password)
{
	std::string url = "http://" + domain + request_url;
	reset(url);

	HTTPRequest request(method, request_url, HTTPMessage::HTTP_1_1);

	std::string response_str;

	//send_request(request, charset, media_type, url, request_body, "");

	//response_str = receive_response(request, "");

	//if(response.getStatus() == Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED)
	//{
		Poco::Net::HTTPCredentials credentials(username, password);
		credentials.authenticate(request, response);

		send_request(request, "", "UTF-8", useragent, url, request_body, "");

		response_str = receive_response(request, "");
	//}

	return response_str;
}

std::string http_session_t::send_request(const std::string &method, const std::string &request_url, const std::string &request_body, const std::string &username, const std::string &password)
{
	return send_request(method, "", request_url, request_body, username, password);
}

std::string http_session_t::send_form(const std::string &method, const std::string &request_url, const std::string &username, const std::string &password, const std::string &cookie_name)
{
	//bool is_https = request_url.find("https://") != std::string::npos;

	//std::string url = is_https ? request_url : "http://" + domain + request_url;
	//reset(url);

	HTTPRequest request(method, request_url, HTTPMessage::HTTP_1_1);
	request.add("User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/33.0.1750.146 Safari/537.36");
	//request.add("User-Agent", progname);
	request.add("Accept-Encoding", "gzip,deflate");
	request.setContentType("application/x-www-form-urlencoded");
	request.setKeepAlive(true); // notice setKeepAlive is also called on session (above)

	//std::string request_body("username=user1@" + domain + "&password=mypword");
	//request.setContentLength(request_body.length());

	//std::ostream& myOStream = session.sendRequest(request); // sends request, returns open stream
	//myOStream << request_body;  // sends the body

	//request.write(std::cout);

	//Poco::Net::HTTPResponse response;
	//std::istream& iStr = session.receiveResponse(response);  // get the response from server
	//return iStr.rdbuf();  // dump server response so you can view it

	std::string s;

	uri = URI(request_url);

	session = HTTPSessionFactory::defaultFactory().createClientSession(uri);
	configure_proxy(session);

	std::ostream& send = session->sendRequest(request);

	// Get and print out the response from the server
	std::istream& rs = session->receiveResponse(response);

	s = receive_response(rs, request, cookie_name);

	update_combined_cookies();
	request.setCookies(ConvertCookies(combined_cookies));

	// Create a new HTMLForm to submit to and add some basic string data
	Poco::Net::HTMLForm form;
	//form.add("username", username);
	form.add("login", username);
	form.add("password", password);

	// Fill out the request object and write to output stream
	form.prepareSubmit(request);

	//if(is_https)
	//{
	//	return send_https(request, form, cookie_name);
	//}
	//else
	{
		std::ostream& send = session->sendRequest(request);
		form.write(send);

		return receive_response(request, cookie_name);
	}
}

std::string http_session_t::go_to(const std::string &location, const std::string &cookie_name)
{
	uri = URI("http://" + domain);

	std::string domain_tmp = domain;

	std::string res = redirect_to(location, cookie_name);

	domain = domain_tmp;

	uri = URI("http://" + domain);

	return res;
}

std::string http_session_t::redirect_to(const std::string &location, const std::string &cookie_name)
{
	uri.resolve(location);

	std::string response_str;
	do 
	{
		response_str = send_request(cookie_name);
	}
	while(response_str.empty());

	return response_str;
}

bool http_session_t::download_to(const std::string &filename, const std::string &location, const std::string &content_type, const std::string &charset, const std::string &useragent, const std::string &referer_url, const std::string &cookie_name)
{
	uri.resolve(location);

	std::string path(uri.getPathAndQuery());
	if(path.empty()) path = "/";

	session = HTTPSessionFactory::defaultFactory().createClientSession(uri);
	configure_proxy(session);
	session->setKeepAlive(true);

	HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
	////request.add("User-Agent", progname);
	//request.add("User-Agent", "Mozilla/5.0 (Windows NT 6.2; rv:37.0) Gecko/20100101 Firefox/37.0");
	//request.add("Accept-Encoding", "gzip, deflate");

	////request.add(HTTPRequest::COOKIE, update_cookies(cookie_name));

	//request.setKeepAlive(true);

	fill_request(request, "image/png,image/*;q=0.8,*/*;q=0.5", content_type, charset, useragent, referer_url, 0, cookie_name);

	update_combined_cookies();
	request.setCookies(ConvertCookies(combined_cookies));

	session->sendRequest(request);

	// Get and print out the response from the server
	std::istream& rs = session->receiveResponse(response);

	std::ofstream ofs(filename.c_str(), std::ios_base::binary);
	if(ofs.rdstate() & std::ios::failbit)
	{
		//throw Exception::FileError("Error on creating File", filename);
		return false;
	}

	return receive_response(rs, ofs, request, cookie_name) > 0;
}

std::string http_session_t::get(const std::string &location)
{
	uri.resolve(location);

	std::string path(uri.getPathAndQuery());
	if(path.empty()) path = "/";

	//session.reset();
	//session.setHost(uri.getHost());
	//session.setPort(uri.getPort());

	session = HTTPSessionFactory::defaultFactory().createClientSession(uri);
	configure_proxy(session);
	//session->setKeepAlive(true);

	HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
	//request.add("User-Agent", progname);
	//request.add("Accept-Encoding", "gzip,deflate");

	//request.setHost(uri.getHost());
	//request.set("User-Agent", "Mozilla/5.0 (Windows NT 6.2; rv:35.0) Gecko/20100101 Firefox/35.0");
	//request.set("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
	//request.set("Accept-Language", "en-US,en;q=0.5");
	////request.set("Accept-Encoding", "gzip, deflate");
	//request.set("Referer", "http://www.anime-planet.com/login.php");

	std::string imdb_useragent = /*parser_info.useragent*/"Mozilla/5.0 (Windows NT 6.2; rv:37.0) Gecko/20100101 Firefox/37.0";

	fill_request(request, "", "", "UTF-8", imdb_useragent, "", 0, "");

	//request.setKeepAlive(true);

	session->sendRequest(request);

	return receive_response(request, "");
}

std::string http_session_t::send_request(const std::string &cookie_name)
{
	std::string path(uri.getPathAndQuery());
	if(path.empty()) path = "/";

	//session.reset();
	//session.setHost(uri.getHost());
	//session.setPort(uri.getPort());

	session = HTTPSessionFactory::defaultFactory().createClientSession(uri);
	configure_proxy(session);
	session->setKeepAlive(true); /// !

	HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
	//request.add("User-Agent", progname);
///	request.add("Accept-Encoding", "gzip,deflate");

	std::string imdb_useragent = /*parser_info.useragent*/"Mozilla/5.0 (Windows NT 6.2; rv:37.0) Gecko/20100101 Firefox/37.0";

	fill_request(request, "", "", "UTF-8", imdb_useragent, "", 0, "");

	//request.setHost(uri.getHost());
	//request.set("User-Agent", "Mozilla/5.0 (Windows NT 6.2; rv:35.0) Gecko/20100101 Firefox/35.0");
	//request.set("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
	//request.set("Accept-Language", "en-US,en;q=0.5");
	////request.set("Accept-Encoding", "gzip, deflate");
	//request.set("Referer", "http://www.anime-planet.com/login.php");

///	request.add(HTTPRequest::COOKIE, update_cookies(cookie_name));
	update_combined_cookies();
	if(!cookie_name.empty())
	{
		std::vector<HTTPCookie> current_cookies;

		std::vector<std::string> cookie_names = split(cookie_name, '|');
		for(int i = 0; i < cookie_names.size(); ++i)
			for(std::vector<HTTPCookie>::iterator combined_cookies_it = combined_cookies.begin(); combined_cookies_it != combined_cookies.end(); ++combined_cookies_it)
				if(combined_cookies_it->getName() == cookie_names[i])
				{
					current_cookies.push_back(*combined_cookies_it);
					break;
				}

				request.setCookies(ConvertCookies(current_cookies));
	}
	else
		request.setCookies(ConvertCookies(combined_cookies));

	//std::vector<HTTPCookie> cookies;
	//response.getCookies(cookies);

	//std::vector<HTTPCookie>::const_iterator it = cookies.begin();
	//while(it != cookies.end())
	//{
	//	request.add(HTTPRequest::COOKIE, it->toString());
	//	++it;
	//}

	request.setKeepAlive(true);

	session->sendRequest(request);

	return receive_response(request, cookie_name);
}

void http_session_t::handle_redirection(const std::string &cookie_name)
{
	while(response.getStatus() == HTTPResponse::HTTP_MOVED_PERMANENTLY || response.has("Location"))
	{
		std::string Location = response.get("Location");
		uri.resolve(Location);
		std::string path(uri.getPathAndQuery());
		if(path.empty()) path = "/";

		//session.reset();
		//session.setHost(uri.getHost());
		//session.setPort(uri.getPort());

		session = HTTPSessionFactory::defaultFactory().createClientSession(uri);
		configure_proxy(session);

		//request.setURI(path);
		//request.setContentType(HTTPRequest::EMPTY);
		//request.setContentLength(HTTPRequest::UNKNOWN_CONTENT_LENGTH);
		//request.clear();
		HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
		//request.add("User-Agent", progname);
		request.add("Accept-Encoding", "gzip,deflate");

		//request.setHost(uri.getHost());
		//request.set("User-Agent", "Mozilla/5.0 (Windows NT 6.2; rv:35.0) Gecko/20100101 Firefox/35.0");
		//request.set("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
		//request.set("Accept-Language", "en-US,en;q=0.5");
		////request.set("Accept-Encoding", "gzip, deflate");
		//request.set("Referer", "http://www.anime-planet.com/login.php");

		request.add(HTTPRequest::COOKIE, update_cookies(cookie_name));

		//std::vector<HTTPCookie> cookies;
		//response.getCookies(cookies);

		//std::vector<HTTPCookie>::const_iterator it = cookies.begin();
		//while(it != cookies.end())
		//{
		//	request.add(HTTPRequest::COOKIE, it->toString());
		//	++it;
		//}

		request.setKeepAlive(true);

		session->sendRequest(request);

		// Get and print out the response from the server
		std::istream& rs = session->receiveResponse(response);
		print_response(rs, response, uri, request);
	}
}

std::string http_session_t::update_cookies(const std::string &cookie_name)
{
	Poco::Net::NameValueCollection::ConstIterator it = response.find(HTTPResponse::SET_COOKIE);
	while (it != response.end() && Poco::icompare(it->first, HTTPResponse::SET_COOKIE) == 0)
	{
		Poco::Net::NameValueCollection nvc;
		response.splitParameters(it->second.begin(), it->second.end(), nvc);
		Poco::Net::NameValueCollection::ConstIterator cookie = nvc.find(cookie_name);
		if(cookie != nvc.end() && Poco::icompare(cookie->first, cookie_name) == 0)
			cookies[cookie_name] = cookie->first + "=" + cookie->second;//request.add(HTTPRequest::COOKIE, cookie->first + "=" + cookie->second);
		++it;
	}

	if(cookies.find(cookie_name) != cookies.end())
		return cookies[cookie_name];

	return "";
}

void http_session_t::reset(const std::string &url)
{
	//std::string url = "http://" + domain + request_url;
	uri = URI(url);

	//session.reset();
	//session.setHost(uri.getHost());
	//session.setPort(uri.getPort());

	session = HTTPSessionFactory::defaultFactory().createClientSession(uri);
	configure_proxy(session);

	session->setKeepAlive(true);
}

void http_session_t::update_combined_cookies()
{
	std::vector<HTTPCookie> response_cookies;
	response.getCookies(response_cookies);
	if(!response_cookies.empty())
	{
		last_cookies = ConvertCookies(response_cookies);
		for(std::vector<HTTPCookie>::iterator response_cookies_it = response_cookies.begin(); response_cookies_it != response_cookies.end(); ++response_cookies_it)
		{
			bool cookie_found = false;
			for(std::vector<HTTPCookie>::iterator combined_cookies_it = combined_cookies.begin(); combined_cookies_it != combined_cookies.end(); ++combined_cookies_it)
				if(combined_cookies_it->getName() == response_cookies_it->getName())
				{
					cookie_found = true;
					break;
				}
			if(!cookie_found)
				combined_cookies.push_back(*response_cookies_it);
		}
		//combined_cookies.insert(combined_cookies.end(), response_cookies.begin(), response_cookies.end());
	}
	//request.setCookies(last_cookies);
}

//std::string http_session_t::send_https(HTTPRequest &request, Poco::Net::HTMLForm &form, const std::string &cookie_name)
//{
//	//const Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
//	const Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "");
//	Poco::Net::HTTPSClientSession session(uri.getHost(), uri.getPort(), context);
//
//	std::ostream& send = session.sendRequest(request);
//	form.write(send);
//
//	// Get and print out the response from the server
//	std::istream& rs = session.receiveResponse(response);
//
//	return receive_response(rs, request, cookie_name);
//}
