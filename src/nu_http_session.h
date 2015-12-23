//
// NowUpdater
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#ifndef NU_HTTP_SESSION_H
#define NU_HTTP_SESSION_H
//-----------------------------------------------------------------------------------
#include "config.h"
#include "nu_types.h"

#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTMLForm.h"
#include "Poco/Net/HTTPCookie.h"
#include "Poco/Net/HTTPCredentials.h"

#include "Poco/StreamCopier.h"
#include "Poco/NullStream.h"
#include "Poco/Path.h"
#include "Poco/URI.h"
#include "Poco/Exception.h"

using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;
using Poco::Net::HTTPCookie;
using Poco::StreamCopier;
using Poco::Path;
using Poco::URI;
using Poco::Exception;

typedef std::pair<std::string, std::string> string_pair_t;

struct http_session_t
{
	std::string domain;
	URI uri;
	HTTPClientSession *session;
	//HTTPRequest request;
	HTTPResponse response;

	std::map<std::string, std::string> cookies;
	//std::vector<HTTPCookie> cookies;
	std::vector<HTTPCookie> combined_cookies;
	Poco::Net::NameValueCollection last_cookies;

	http_session_t(std::string site);

	std::string receive_response(HTTPRequest &request, const std::string &cookie_name);
	std::string receive_response(std::istream& rs, HTTPRequest &request, const std::string &cookie_name);
	std::streamsize receive_response(std::istream& rs, std::ostream& ostr, HTTPRequest &request, const std::string &cookie_name);

	bool send_json_request(const std::string &uri, const std::string &json_request_str, const std::string &cookie_name);

	std::ostream& send_request(HTTPRequest &request, const std::string &content_type, const std::string &charset, const std::string &useragent, const std::string &referer_url, uint32_t content_size, const std::string &cookie_name);

	void send_request(HTTPRequest &request, const std::string &content_type, const std::string &charset, const std::string &useragent, const std::string &referer_url, const std::string &request_body, const std::string &cookie_name);

	std::string send_request(const std::string &method, const std::string &request_url, const std::string &request_body, const std::string &cookie_name);

	std::string send_request(const std::string &method, const std::string &useragent, const std::string &request_url, const std::string &request_body, const std::string &username, const std::string &password);

	std::string send_request(const std::string &method, const std::string &request_url, const std::string &request_body, const std::string &username, const std::string &password);

	std::string send_form(const std::string &method, const std::string &request_url, const std::string &username, const std::string &password, const std::string &cookie_name);

	//std::string send_https(HTTPRequest &request, Poco::Net::HTMLForm &form, const std::string &cookie_name);

	std::string go_to(const std::string &location, const std::string &cookie_name);

	std::string redirect_to(const std::string &location, const std::string &cookie_name);

	bool download_to(const std::string &filename, const std::string &location, const std::string &content_type, const std::string &charset, const std::string &useragent, const std::string &referer_url, const std::string &cookie_name);

	std::string get(const std::string &location);

	void update_combined_cookies();

	std::string send_request(const std::string &cookie_name);

	std::string update_cookies(const std::string &cookie_name);

	void handle_redirection(const std::string &cookie_name);

	void fill_request(HTTPRequest &request, const std::string &accept, const std::string &content_type, const std::string &charset, const std::string &useragent, const std::string &referer_url, uint32_t content_size, const std::string &cookie_name);

	std::string send_request(HTTPRequest &request, const std::string &content_type, const std::string &charset, const std::string &useragent, const std::string &referer_url, const std::vector<string_pair_t> &params, const std::string &username, const std::string &password, const std::string &cookie_name);

	std::string send_request(const std::string &method, const std::string &request_url, const std::vector<string_pair_t> &params, const std::string &useragent, const std::string &username, const std::string &password, const std::string &cookie_name);

	std::string send_request(const std::string &method, const std::string &request_url, const std::string &referer_url, const std::vector<string_pair_t> &params, const std::string &useragent, const std::string &username, const std::string &password, const std::string &cookie_name);

	std::string send_request(const std::string &method, const std::string &request_url, const string_pair_t &param, const std::string &useragent, const std::string &username, const std::string &password, const std::string &cookie_name);

	void reset(const std::string &url);
};

//-----------------------------------------------------------------------------------
#endif
