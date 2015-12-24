//
// NowUpdater
//
// nu_site_info.cpp
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#include "nu_site_info.h"
//-----------------------------------------------------------------------------------
#include "nu_user_info.h"

#include "nu_json.h"

#include "Poco/JSON/Parser.h"
#include "Poco/JSON/Object.h"

#include <cctype>

#include <regex>
//-----------------------------------------------------------------------------------

#include "Poco/Process.h"
#include "Poco/Pipe.h"
#include "Poco/PipeStream.h"

using Poco::Process;
using Poco::ProcessHandle;
using Poco::Pipe;
using Poco::PipeInputStream;
using Poco::PipeOutputStream;

std::string LaunchWithRedirectOut(std::string cmd, std::string args_str)
{
	std::vector<std::string> args = split(args_str, ' ');

	Pipe outPipe;
	ProcessHandle ph = Process::launch(cmd, args, 0, &outPipe, 0);
	PipeInputStream istr(outPipe);
	std::string s;
	int c = istr.get();
	while (c != -1) { if(!std::iscntrl(c)) s += (char) c; c = istr.get(); }

	int rc = ph.wait();
	//assert(rc == 1);

	return s;
}

//void str_replace(std::string &s, const std::string &search, const std::string &replace)
//{
//	for( size_t pos = 0; ; pos += replace.length() ) 
//	{
//		pos = s.find( search, pos );
//		if( pos == std::string::npos ) break;
//
//		s.erase( pos, search.length() );
//		s.insert( pos, replace );
//	}
//}

template <class T> T do_nothing_just_pass_value(const pugi::char_t *value)
{
	return value;
}

//template <class T> T select_from(pugi::xml_document &doc, const std::string &xpath, T (*f)(const pugi::char_t*))
//{
//	pugi::xpath_node node = doc.select_node(xpath.c_str());
//
//	return f(node.node() ? node.node().value() : node.attribute().value());
//}
//
//std::string select_from(pugi::xml_document &doc, const std::string &xpath)
//{
//	return select_from(doc, xpath, do_nothing_just_pass_value<const pugi::char_t*>);
//}

template <class T> bool parse_f(const pugi::char_t *node_str, const parser_entity_t &entity, T &result_value, T (*convert_f)(const pugi::char_t *))
{
	if(!node_str)
		return false;

	if(strlen(node_str) > 0 && !entity.regexp.empty())
	{
		const std::regex pattern(entity.regexp/*, std::regex::nosubs*/);

		// object that will contain the sequence of sub-matches
		std::match_results<const pugi::char_t *> result;

		// match the node text with the regular expression
		bool valid = std::regex_search(node_str, result, pattern/*, std::regex_constants::match_continuous*/);

		// if the node text matched the regex, then return the first part
		if(valid && !result.empty())
		{
			result_value = convert_f(result.str(result.size() - 1).c_str());
			return true;
		}
		else
			return false;
	}

	result_value = convert_f(node_str);
	return true;
}

template <class T> bool parse_f(pugi::xml_node &node, const parser_entity_t &entity, T &result_value, T (*convert_f)(const pugi::char_t *))
{
	if(entity.xpath.empty())
	{
		pugi::string_t node_str = pugi::serialization::node_to_raw_string(node);
		//pugi::string_t node_str;

		////pugi::xpath_node_set ns = node.select_nodes(PUGIXML_TEXT("text()"));

		////for(size_t i = 0; i < ns.size(); ++i)
		////	node_str += ns[i].node().value();

		//for(pugi::xml_node child = node.first_child(); child; child = child.next_sibling())
		//	if(child.type() == pugi::node_pcdata)
		//		node_str += child.value();

		return parse_f(node_str.c_str(), entity, result_value, convert_f);
	}
	else
	{
		pugi::xpath_node node_selected = node.select_node(entity.xpath.c_str());
		if(!node_selected)
			return false;
 
		const pugi::char_t *node_str = node_selected.node() ? node_selected.node().value() : node_selected.attribute().value();

		return parse_f(node_str, entity, result_value, convert_f);
	}
}

bool parse(pugi::xml_node &node, const parser_entity_t &entity, std::string &result_value)
{
	return parse_f<std::string>(node, entity, result_value, do_nothing_just_pass_value);
}

bool parse(pugi::xml_node &node, const parser_entity_t &entity, uint32_t &result_value)
{
	return parse_f(node, entity, result_value, get_uint32_number);
}

bool parse(pugi::xml_node &node, const parser_entity_t &entity, float &result_value)
{
	return parse_f(node, entity, result_value, get_float_number);
}
//-----------------------------------------------------------------------------------

std::string site_info_t::imdb_get_title_id_str(const title_info_t &title)
{
	std::string imdb_title_id = std::to_string(title.id); //imdb_get_title_id(title.name);
	assert(imdb_title_id.size() <= 7);
	std::string imdb_title_id_prefix = "tt";
	for(int i = 0; i < 7 - imdb_title_id.size(); ++i)
		imdb_title_id_prefix += "0";
	imdb_title_id = imdb_title_id_prefix + imdb_title_id;
	return imdb_title_id;
}


bool site_info_t::send_request_search_title_imdb(site_user_info_t &site_user, const std::string &title_name, std::vector<title_info_t> &found_titles)
{
	if(!authenticate_imdb(site_user))
		return false;

	http->domain = "www.imdb.com";

	std::string title_name_encoded_str;
	Poco::URI::encode(title_name, "", title_name_encoded_str);
	std::string search_uri = "http://www.imdb.com/xml/find?json=1&nr=1&tt=on&q=" + title_name_encoded_str;

	std::string json_response_str = http->get(search_uri);//http->redirect_to(search_uri, login_cookie);
	std::string json_decoded_response_str;
	Poco::URI::decode(json_response_str, json_decoded_response_str);
	Poco::JSON::Parser parser;
	Poco::Dynamic::Var json_response = parser.parse(json_decoded_response_str);

	// use pointers to avoid copying
	Poco::JSON::Object::Ptr object = json_response.extract<Poco::JSON::Object::Ptr>();

	std::string title_matches[] = { "title_popular", "title_exact", "title_approx" };

	for(int i = 0; i < countof(title_matches); ++i)
	{
		// use pointers to avoid copying
		Poco::JSON::Array::Ptr title_match_array = object->getArray(title_matches[i]);
		if(title_match_array)
		{
			for(int k = 0; k < title_match_array->size(); ++k)
			{
				Poco::JSON::Object::Ptr title_match_array_object = title_match_array->getObject(k); // holds { "property" : "value" }
				if(title_match_array_object)
				{
					Poco::Dynamic::Var title_match_array_object_id = title_match_array_object->get("id");
					if(!title_match_array_object_id.isEmpty())
					{
						Poco::Dynamic::Var title_match_array_object_title_description = title_match_array_object->get("title_description");
						if(!title_match_array_object_title_description.isEmpty())
						{
							std::string imdb_title_description = title_match_array_object_title_description.extract<std::string>();
							if(imdb_title_description.find("video game") != std::string::npos)
								continue;
						}

						std::string imdb_title_id = title_match_array_object_id.extract<std::string>();

						title_info_t title;
						title.uri = "http://www.imdb.com/title/" + imdb_title_id + "/";

						pugi::xml_document doc;

						if(!load_xhtml(doc, http->redirect_to(title.uri, login_cookie)))
							continue; //return false;

						if(parse_title_info_imdb(doc, site_user, title))
							found_titles.push_back(title);
					}
				}
			}
		}
	}

	return !found_titles.empty();
}

std::string site_info_t::imdb_get_title_id(const std::string &title_name)
{
	std::string title_name_encoded_str;
	Poco::URI::encode(title_name, "", title_name_encoded_str);
	std::string search_uri = "http://www.imdb.com/xml/find?json=1&nr=1&tt=on&q=" + title_name_encoded_str;

	std::string json_response_str = http->get(search_uri);//http->redirect_to(search_uri, login_cookie);
	std::string json_decoded_response_str;
	Poco::URI::decode(json_response_str, json_decoded_response_str);
	Poco::JSON::Parser parser;
	Poco::Dynamic::Var json_response = parser.parse(json_decoded_response_str);

	// use pointers to avoid copying
	Poco::JSON::Object::Ptr object = json_response.extract<Poco::JSON::Object::Ptr>();

	std::string title_matches[] = { "title_popular", "title_exact", "title_approx" };

	for(int i = 0; i < countof(title_matches); ++i)
	{
		// use pointers to avoid copying
		Poco::JSON::Array::Ptr title_match_array = object->getArray(title_matches[i]);
		if(title_match_array)
		{
			for(int k = 0; k < title_match_array->size(); ++k)
			{
				Poco::JSON::Object::Ptr title_match_array_object = title_match_array->getObject(k); // holds { "property" : "value" }
				if(title_match_array_object)
				{
					Poco::Dynamic::Var title_match_array_object_id = title_match_array_object->get("id");
					if(!title_match_array_object_id.isEmpty())
					{
						return title_match_array_object_id.extract<std::string>();
					}
				}
			}
		}
	}

	return "";

	//return http->send_json_request(title.uri, json_request_str, login_cookie);

	//$imdb_title = urlencode($rating['title']);
	//$content = file_get_contents('http://www.imdb.com/xml/find?json=1&nr=1&tt=on&q='.$imdb_title);
	//if ($content === false)
	//{
	//	throw new Exception('Error while fetching tconst for '.$rating['title'].'.');
	//}
	//$json = json_decode($content, true);
	//if ($json === null)
	//{
	//	throw new Exception('Could not decode json result for '.$rating['title'].'.');
	//}
	//// title_popular, title_exact, title_approx
	//// TODO: If we fail to find the exact title, try the next category until we've gone through them all
	//if (array_key_exists('title_popular', $json))
	//{
	//	$type = 'title_popular';
	//}
	//else if (array_key_exists('title_exact', $json))
	//{
	//	$type = 'title_exact';
	//}
	//else if (array_key_exists('title_approx', $json))
	//{
	//	$type = 'title_approx';
	//}
	//else
	//{
	//	continue;
	//}
	//// Check title matches
	//$matched_title_index = -1;
	//$i = -1;
	//foreach ($json[$type] as $movie)
	//{
	//	++$i;
	//	if ($movie['title'] === $rating['title'])
	//	{
	//		$matched_title_index = $i;
	//		break;
	//	}
	//}
	//if ($matched_title_index === -1)
	//{
	//	echo 'Non matching title ' . $rating['title'].PHP_EOL;
	//	return null;
	//}
	//return $json[$type][$matched_title_index]['id'];
}

std::string site_info_t::imdb_get_auth_token(const std::string &imdb_title_id)
{
	//login_cookie = "id";

	//$cookie_details = ['id' => $this->id];
	//$context_options = ['http' =>
	//						[
	//						'method' => 'GET',
	//						'header' => 'Cookie: '.
	//						]
	//					];
	pugi::xml_document doc;

	if(!load_xhtml(doc, http->redirect_to("http://www.imdb.com/title/" + imdb_title_id + "/", login_cookie)))
		return false;

	std::string data_auth;
	//parse(doc, parser_entity_t("//div[@class='rating rating-list']/@data-auth"), data_auth);
	parse(doc, parser_entity_t("//input[@id='seen-config']/@data-apptoken"), data_auth);
	//$page_content = file_get_contents($page_url, false, $context);
	//$data_auth_begin = strpos($page_content, 'data-auth');
	//$data_auth_end = strpos($page_content, '"', $data_auth_begin + 11);
	//$data_auth = substr($page_content, $data_auth_begin + 11, $data_auth_end - ($data_auth_begin + 11));
	return data_auth;
}

bool site_info_t::send_request_get_title_rating_imdb(site_user_info_t &site_user, const title_info_t &title, float &rating)
{
	if(!authenticate_imdb(site_user))
		return false;

	http->domain = "www.imdb.com";

	std::string imdb_title_id = imdb_get_title_id_str(title);

	std::string imdb_auth_token = imdb_get_auth_token(imdb_title_id);

	std::vector<string_pair_t> params;
	params.push_back(std::make_pair("caller", "tt_ov"));
	params.push_back(std::make_pair("appToken", imdb_auth_token));
	params.push_back(std::make_pair("action", "fetch"));
	params.push_back(std::make_pair("titleIds[]", imdb_title_id));

	std::string json_response_str = http->send_request(HTTPRequest::HTTP_POST, "/seen/_ajax/seen", "http://www.imdb.com/title/" + imdb_title_id + "/", params, parser_info.useragent, "", "", login_cookie);

	std::string json_decoded_response_str;
	Poco::URI::decode(json_response_str, json_decoded_response_str);
	Poco::JSON::Parser parser;
	Poco::Dynamic::Var json_response = parser.parse(json_decoded_response_str);

	// use pointers to avoid copying
	Poco::JSON::Object::Ptr object = json_response.extract<Poco::JSON::Object::Ptr>();
	Poco::JSON::Object::Ptr response_object = object->getObject("titleIds"); // holds { "property" : "value" }
	if(response_object)
	{
		Poco::Dynamic::Var imdb_title_id_var = response_object->get(imdb_title_id);
		if(!imdb_title_id_var.isEmpty())
		{
			std::string rating_str = imdb_title_id_var.extract<std::string>();
			rating = get_float_number(rating_str.c_str());
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

	return false;
}


bool site_info_t::send_request_change_title_status_imdb(site_user_info_t &site_user, const title_info_t &title, uint32_t status)
{
	if(!authenticate_imdb(site_user))
		return false;

	//for(std::vector<imdb_list_t>::iterator imdb_lists_it = imdb_lists.begin(); imdb_lists_it != imdb_lists.end(); ++imdb_lists_it)
	//for(uint32_t i = 0; i < imdb_lists.size(); ++i)
	//	if(imdb_lists[i].status == status)
	//		send_request_add_title_imdb_list_ajax(site_user, title, imdb_list[i]);

	//return http->send_json_request(title.uri, json_request_str, login_cookie);
}

bool site_info_t::send_request_change_title_rating_imdb(site_user_info_t &site_user, const title_info_t &title, float rating)
{
//void submit_rating($rating, $tconst, $auth)

	http->domain = "www.imdb.com";

// http://www.imdb.com/ratings/_ajax/title
// tconst = movie id (tt3123123)
// rating = your rating on 10
// auth = auth key to submit
// tracking_tag = 'title-maindetails'

	std::string imdb_title_id = imdb_get_title_id(title.name);
	//std::string imdb_title_id = imdb_get_title_id_str(title);

	pugi::xml_document doc;

	if(!load_xhtml(doc, http->redirect_to("http://www.imdb.com/title/" + imdb_title_id + "/", login_cookie)))
		return false;

	std::string rate_url;
	parse(doc, parser_entity_t("//a[contains(@href, \"vote?v=" + std::to_string(uint32_t(rating / rating_mulcoef)) + "\")]/@href"), rate_url);

	return rate_url.empty() ? false : load_xhtml(doc, http->redirect_to(rate_url, login_cookie));

	// other way to do it:
	/*std::string imdb_auth_token = imdb_get_auth_token(imdb_title_id);

	std::vector<string_pair_t> params;
	//params.push_back(std::make_pair("tconst", tconst));
	//params.push_back(std::make_pair("rating", std::to_string(uint32_t(rating / rating_mulcoef))));
	//params.push_back(std::make_pair("auth", imdb_get_auth_token(tconst)));
	//params.push_back(std::make_pair("tracking_tag", "title-maindetails"));
	params.push_back(std::make_pair("caller", "tt_ov"));
	params.push_back(std::make_pair("appToken", imdb_auth_token));
	params.push_back(std::make_pair("titleId", imdb_title_id));
	params.push_back(std::make_pair("action", std::to_string(uint32_t(rating / rating_mulcoef))));

	std::string s;

	//s = http->send_request(HTTPRequest::HTTP_POST, "/ratings/_ajax/title", params, imdb_useragent, "", "", login_cookie);
	s = http->send_request(HTTPRequest::HTTP_POST, "/seen/_ajax/seen", "http://www.imdb.com/title/" + imdb_title_id + "/", params, parser_info.useragent, "", "", login_cookie);
	//return s != "";
	return s == "{}";*/

	//echo 'Submitting rating for '.$rating['title'].PHP_EOL;
	//$cookie_details = ['id' => $this->id];
	//$imdb_rating = floor($rating['rating'] / $this->rating_base * 10);
	//$data = ['tconst' => $tconst, 'rating' => $imdb_rating, 'auth' => $auth, 'tracking_tag' => 'title-maindetails'];
	//$data = http_build_query($data);
	//$context_options = ['http' =>
	//	[
	//		'method' => 'POST',
	//		'header' => 'Cookie: '.http_build_cookie($cookie_details)."\r\n".
	//		'Content-type: application/x-www-form-urlencoded'."\r\n".
	//		'Content-Length: '.strlen($data),
	//		'content' => $data
	//	]
	//];
	//$context = stream_context_create($context_options);
	//$page_url = 'http://www.imdb.com/ratings/_ajax/title';
	//$page_content = file_get_contents($page_url, false, $context);
	//echo 'Submitted to http://www.imdb.com/title/'.$tconst.PHP_EOL;
}

// other way to do it:
bool site_info_t::send_request_change_title_rating_imdb_ajax(site_user_info_t &site_user, const title_info_t &title, float rating)
{
//void submit_rating($rating, $tconst, $auth)

	http->domain = "www.imdb.com";

// http://www.imdb.com/ratings/_ajax/title
// tconst = movie id (tt3123123)
// rating = your rating on 10
// auth = auth key to submit
// tracking_tag = 'title-maindetails'

	std::string imdb_title_id = imdb_get_title_id_str(title);

	std::string imdb_auth_token = imdb_get_auth_token(imdb_title_id);

	std::vector<string_pair_t> params;
	//params.push_back(std::make_pair("tconst", tconst));
	//params.push_back(std::make_pair("rating", std::to_string(uint32_t(rating / rating_mulcoef))));
	//params.push_back(std::make_pair("auth", imdb_get_auth_token(tconst)));
	//params.push_back(std::make_pair("tracking_tag", "title-maindetails"));
	params.push_back(std::make_pair("caller", "tt_ov"));
	params.push_back(std::make_pair("appToken", imdb_auth_token));
	params.push_back(std::make_pair("titleId", imdb_title_id));
	params.push_back(std::make_pair("action", std::to_string(uint32_t(rating / rating_mulcoef))));

	std::string s;

	//s = http->send_request(HTTPRequest::HTTP_POST, "/ratings/_ajax/title", params, imdb_useragent, "", "", login_cookie);
	s = http->send_request(HTTPRequest::HTTP_POST, "/seen/_ajax/seen", "http://www.imdb.com/title/" + imdb_title_id + "/", params, parser_info.useragent, "", "", login_cookie);
	//return s != "";
	return s == "{}";
}


bool site_info_t::send_request_get_list_item_id_imdb_list_ajax(site_user_info_t &site_user, const title_info_t &title, imdb_list_t &imdb_list, std::string &imdb_list_item_id, std::string &imdb_hidden_key_name, std::string &imdb_hidden_key)
{
	http->domain = "www.imdb.com";

	std::string imdb_title_id = imdb_get_title_id_str(title);

	std::vector<string_pair_t> params;
	params.push_back(std::make_pair("consts[]", imdb_title_id));
	params.push_back(std::make_pair("tracking_tag", "wlb-lite"));

	bool imdb_watchlist_has = false;

	std::string json_response_str = http->send_request(HTTPRequest::HTTP_POST, "/list/_ajax/watchlist_has", "http://www.imdb.com/title/" + imdb_title_id + "/reference", params, parser_info.useragent, "", "", login_cookie);

	std::string json_decoded_response_str;
	Poco::URI::decode(json_response_str, json_decoded_response_str);
	Poco::JSON::Parser parser;
	Poco::Dynamic::Var json_response = parser.parse(json_decoded_response_str);

	// use pointers to avoid copying
	Poco::JSON::Object::Ptr object = json_response.extract<Poco::JSON::Object::Ptr>();
	Poco::Dynamic::Var status_var = object->get("status");
	if(!status_var.isEmpty() && status_var.extract<int>() == 200)
	{
		Poco::JSON::Object::Ptr imdb_watchlist_has_object = object->getObject("has"); // holds { "property" : "value" }
		if(imdb_watchlist_has_object)
		{
			imdb_watchlist_has = imdb_watchlist_has_object->has(imdb_title_id);
			if(imdb_watchlist_has)
			{
				Poco::Dynamic::Var imdb_title_id_var = imdb_watchlist_has_object->get(imdb_title_id);
				if(!imdb_title_id_var.isEmpty())
				{
					imdb_list_item_id = imdb_title_id_var.extract<std::string>();
				}
			}
		}
		Poco::Dynamic::Var imdb_list_id_var = object->get("list_id");
		if(!imdb_list_id_var.isEmpty())
		{
			imdb_list.list_id = imdb_list_id_var.extract<std::string>();
		}
		Poco::JSON::Object::Ptr imdb_hidden_key_object = object->getObject("extra"); // holds { "property" : "value" }
		if(imdb_hidden_key_object)
		{
			Poco::Dynamic::Var imdb_hidden_key_name_var = imdb_hidden_key_object->get("name");
			if(!imdb_hidden_key_name_var.isEmpty())
			{
				imdb_hidden_key_name = imdb_hidden_key_name_var.extract<std::string>();
			}
			Poco::Dynamic::Var imdb_hidden_key_var = imdb_hidden_key_object->get("value");
			if(!imdb_hidden_key_var.isEmpty())
			{
				imdb_hidden_key = imdb_hidden_key_var.extract<std::string>();
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

	return imdb_watchlist_has;
}

bool site_info_t::send_request_delete_title_imdb_list_ajax(site_user_info_t &site_user, const title_info_t &title, imdb_list_t &imdb_list)
{
	http->domain = "www.imdb.com";

	std::string imdb_title_id = imdb_get_title_id_str(title);

	std::string imdb_list_item_id;
	std::string imdb_hidden_key_name;
	std::string imdb_hidden_key;

	if(!send_request_get_list_item_id_imdb_list_ajax(site_user, title, imdb_list, imdb_list_item_id, imdb_hidden_key_name, imdb_hidden_key))
		return false;

	std::vector<string_pair_t> params;
	params.push_back(std::make_pair("action", "delete"));
	params.push_back(std::make_pair("list_id", imdb_list.list_id));
	params.push_back(std::make_pair("list_item_id", imdb_list_item_id));
	//params.push_back(std::make_pair("ref_tag", "legacy_title_lhs"));
	params.push_back(std::make_pair("ref_tag", "watchlist-ribbon"));
	params.push_back(std::make_pair("list_class", imdb_list.list_class));
	params.push_back(std::make_pair(imdb_hidden_key_name, imdb_hidden_key));

	std::string json_response_str = http->send_request(HTTPRequest::HTTP_POST, "/list/_ajax/edit", "http://www.imdb.com/title/" + imdb_title_id + "/", params, parser_info.useragent, "", "", login_cookie);

	std::string json_decoded_response_str;
	Poco::URI::decode(json_response_str, json_decoded_response_str);
	Poco::JSON::Parser parser;
	Poco::Dynamic::Var json_response = parser.parse(json_decoded_response_str);

	// use pointers to avoid copying
	Poco::JSON::Object::Ptr object = json_response.extract<Poco::JSON::Object::Ptr>();
	Poco::Dynamic::Var status_var = object->get("status");
	if(!status_var.isEmpty() && status_var.extract<int>() == 200)
	{
		return true;
	}

	return false;
}

bool site_info_t::send_request_add_title_imdb(site_user_info_t &site_user, const title_info_t &title, uint32_t status)
{
	if(!authenticate_imdb(site_user))
		return false;

	if(status == NU_TITLE_STATUS_PLAN_TO_WATCH)
		;
	//else
	//	return send_request_change_title_rating_imdb(site_user, title, );


	http->domain = "www.imdb.com";

	std::string imdb_title_id = imdb_get_title_id_str(title);

	std::vector<string_pair_t> params;
	params.push_back(std::make_pair("consts[]", imdb_title_id));
	params.push_back(std::make_pair("tracking_tag", "wlb-lite"));

	std::string imdb_hidden_key_name;
	std::string imdb_hidden_key;
	std::string imdb_list_id;
	bool imdb_watchlist_has;

	std::string json_response_str = http->send_request(HTTPRequest::HTTP_POST, "/list/_ajax/watchlist_has", "http://www.imdb.com/title/" + imdb_title_id + "/reference", params, parser_info.useragent, "", "", login_cookie);

	std::string json_decoded_response_str;
	Poco::URI::decode(json_response_str, json_decoded_response_str);
	Poco::JSON::Parser parser;
	Poco::Dynamic::Var json_response = parser.parse(json_decoded_response_str);

	// use pointers to avoid copying
	Poco::JSON::Object::Ptr object = json_response.extract<Poco::JSON::Object::Ptr>();
	Poco::Dynamic::Var status_var = object->get("status");
	if(!status_var.isEmpty() && status_var.extract<int>() == 200)
	{
		Poco::JSON::Object::Ptr imdb_watchlist_has_object = object->getObject("has"); // holds { "property" : "value" }
		if(imdb_watchlist_has_object)
		{
			imdb_watchlist_has = imdb_watchlist_has_object->has(imdb_title_id);
		}
		else
			imdb_watchlist_has = false;
		Poco::Dynamic::Var imdb_list_id_var = object->get("list_id");
		if(!imdb_list_id_var.isEmpty())
		{
			imdb_list_id = imdb_list_id_var.extract<std::string>();
		}
		Poco::JSON::Object::Ptr imdb_hidden_key_object = object->getObject("extra"); // holds { "property" : "value" }
		if(imdb_hidden_key_object)
		{
			Poco::Dynamic::Var imdb_hidden_key_name_var = imdb_hidden_key_object->get("name");
			if(!imdb_hidden_key_name_var.isEmpty())
			{
				imdb_hidden_key_name = imdb_hidden_key_name_var.extract<std::string>();
			}
			Poco::Dynamic::Var imdb_hidden_key_var = imdb_hidden_key_object->get("value");
			if(!imdb_hidden_key_var.isEmpty())
			{
				imdb_hidden_key = imdb_hidden_key_var.extract<std::string>();
			}
		}
		{
			std::vector<string_pair_t> params;
			params.push_back(std::make_pair("const", imdb_title_id));
			params.push_back(std::make_pair("list_id", imdb_list_id));
			params.push_back(std::make_pair("ref_tag", "legacy_title_lhs"));
			params.push_back(std::make_pair(imdb_hidden_key_name, imdb_hidden_key));

			std::string json_response_str = http->send_request(HTTPRequest::HTTP_POST, "/list/_ajax/edit", "http://www.imdb.com/title/" + imdb_title_id + "/reference", params, parser_info.useragent, "", "", login_cookie);

			std::string json_decoded_response_str;
			Poco::URI::decode(json_response_str, json_decoded_response_str);
			Poco::JSON::Parser parser;
			Poco::Dynamic::Var json_response = parser.parse(json_decoded_response_str);

			// use pointers to avoid copying
			Poco::JSON::Object::Ptr object = json_response.extract<Poco::JSON::Object::Ptr>();
			Poco::Dynamic::Var status_var = object->get("status");
			if(!status_var.isEmpty() && status_var.extract<int>() == 200)
			{
				Poco::Dynamic::Var position_var = object->get("position");
				if(!position_var.isEmpty())
				{
					std::string position = position_var.extract<std::string>();
				}
				//Poco::Dynamic::Var _var = object->get("");
				//if(!_var.isEmpty())
				//{
				//	std::string  = _var.extract<std::string>();
				//}
				//Poco::Dynamic::Var _var = object->get("");
				//if(!_var.isEmpty())
				//{
				//	std::string  = _var.extract<std::string>();
				//}
				//{"status":200,"":"13","list_item_id":"848030118","i"
				return true;
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

	return false;
}

bool site_info_t::send_request_delete_title_imdb(site_user_info_t &site_user, const title_info_t &title)
{
	return send_request_change_title_rating_imdb_ajax(site_user, title, 0);
}

bool site_info_t::authenticate_imdb(site_user_info_t &site_user)
{
	if(!site_user.login_cookies.empty())
	{
		http->combined_cookies.clear();

		for(std::map<std::string, std::string>::const_iterator login_cookies_it = site_user.login_cookies.begin(); login_cookies_it != site_user.login_cookies.end(); ++login_cookies_it)
			http->combined_cookies.push_back(HTTPCookie(login_cookies_it->first, login_cookies_it->second));
	}
	else
		if(!authenticate_imdb_with_tesseract(site_user))
			return false;
		else
			for(std::vector<HTTPCookie>::const_iterator combined_cookies_it = http->combined_cookies.begin(); combined_cookies_it != http->combined_cookies.end(); ++combined_cookies_it)
				site_user.login_cookies[combined_cookies_it->getName()] = combined_cookies_it->getValue();

	return true;
}

bool site_info_t::authenticate_imdb_with_tesseract(site_user_info_t &site_user)
{
	http->domain = "secure.imdb.com";

	http->cookies.clear();
	http->last_cookies.clear();
	http->combined_cookies.clear();

	uint32_t auth_attempt = 0, max_auth_attempts = 50;

	while(http->response.getStatus() != HTTPResponse::HTTP_FOUND && auth_attempt < max_auth_attempts)
	{
		pugi::xml_document doc;

		if(!load_xhtml(doc, http->redirect_to(login_uri, login_cookie)))
			return false;

		std::string imdb_captcha_uri;
		//	parse(doc, parser_entity_t("/html[1]/body[1]/div[2]/div[1]/div[2]/div[1]/div[5]/div[1]/form[1]/table[1]/tbody[1]/tr[4]"), data_auth);
		std::string imdb_hidden_key_name = "49e6c";
		std::string imdb_hidden_key;
		if(parse(doc, parser_entity_t("//div[@id='imdb-login']/form/table/tr/td/img/@src"), imdb_captcha_uri) && // "//img[contains(@src,'/widget/captcha')]/@src"
			parse(doc, parser_entity_t("//div[@id='imdb-login']/form/input[@name='" + imdb_hidden_key_name + "']/@value"), imdb_hidden_key))
		{
			std::string imdb_captcha = "imdb_captcha";
			std::string imdb_captcha_img = imdb_captcha + ".jpg";
			//std::string imdb_captcha = http->go_to(imdb_captcha_uri, login_cookie);
			http->download_to(imdb_captcha_img, imdb_captcha_uri, "", "", parser_info.useragent, login_uri, login_cookie);
			std::string captcha_answer = LaunchWithRedirectOut("cmd", "/c \"\"%PROGRAMFILES%\\Tesseract-OCR\\tesseract.exe\" " + imdb_captcha_img + " " + imdb_captcha + 
																	" -psm 6 nobatch letters && type " + imdb_captcha + ".txt && del " + imdb_captcha + ".txt && del " + imdb_captcha_img + "\"");
			//char captcha_answer[255] = "aaaa";

			//std::replace_if(captcha_answer.begin(), captcha_answer.end(), std::iscntrl, 0);
			std::replace_if(captcha_answer.begin(), captcha_answer.end(), std::isspace, '+');

			std::cout << "captcha_answer = \"" << captcha_answer << "\"" << std::endl;

			std::vector<string_pair_t> params;
			params.push_back(std::make_pair(imdb_hidden_key_name, imdb_hidden_key));
			params.push_back(std::make_pair("login", site_user.username));
			params.push_back(std::make_pair("password", site_user.password));
			params.push_back(std::make_pair("captcha_answer", captcha_answer));

			std::string s;
			//login_cookie="";
			//s = http->send_request(HTTPRequest::HTTP_POST, login_uri, params, /*parser_info.useragent*/"Mozilla/5.0 (Windows NT 6.2; rv:37.0) Gecko/20100101 Firefox/37.0", "", "", login_cookie);
			//s = http->send_request(HTTPRequest::HTTP_POST, login_uri, params, /*parser_info.useragent*/"Linux Firefox", "", "", login_cookie);
			s = http->send_request(HTTPRequest::HTTP_POST, login_uri, params, parser_info.useragent, "", "", login_cookie);
		}
		++auth_attempt;
	}

	//std::string s;

	//if(http->cookies.find(login_cookie) == http->cookies.end())
	//	s = http->send_form(HTTPRequest::HTTP_POST, login_uri, username, password, login_cookie);

	//return http->cookies.find(login_cookie) != http->cookies.end();
	std::vector<HTTPCookie> response_cookies;
	http->response.getCookies(response_cookies);
	if(!response_cookies.empty())
		http->combined_cookies.insert(http->combined_cookies.end(), response_cookies.begin(), response_cookies.end());

	http->cookies.clear();
	http->last_cookies.clear();

	return !response_cookies.empty();
}

#define NU_ERROR(x) { std::cout << x << std::endl; DebugBreak(); }

//#define NU_PARSE_F(a, x, f, e)	{ bool found = parse_f(doc, parser_info.title_##x, a.x, f); if(e && !found) NU_ERROR(#a "." #x " not found at " + parser_info.title_##x.xpath); }
#define NU_PARSE(a, x, e)       { bool found = parse(node, parser_info.title_##x, a.x);      if(e && !found) NU_ERROR(#a "." #x " not found:" + (parser_info.title_##x.xpath.empty() ? parser_info.title_##x.xpath : " at \"" + parser_info.title_##x.xpath + "\"") + (parser_info.title_##x.regexp.empty() ? parser_info.title_##x.regexp : " no match for \"" + parser_info.title_##x.regexp + "\"")); }

bool site_info_t::parse_title_info_imdb(pugi::xml_node &node, site_user_info_t &site_user, title_info_t &title)
{
	NU_PARSE(title, cover_thumb_uri, true);
	NU_PARSE(title, cover_uri, true);

	//title.cover_texture.handle = 0;

	if(title.name.empty())
	{
		NU_PARSE(title, name, true);

		// remove line breaks:
		//title.name.erase(std::remove_if(title.name.begin(), title.name.end(), std::iscntrl), title.name.end());
		//replace_whitespace(title.name);
		std::replace_if(title.name.begin(), title.name.end(), std::iscntrl, ' ');

		std::cout << title.name << "(" << title.uri << ")" << std::endl;
	}

	NU_PARSE(title, id, true);

	std::string title_type_str; { bool found = parse(node, parser_info.title_type, title_type_str); }

	for(std::vector<std::string>::const_iterator title_type_it = title_types.begin(); title_type_it != title_types.end(); ++title_type_it)
		if(title_type_str.find(*title_type_it) != std::string::npos)
		{
			title.type = title_type_it - title_types.begin();

			std::cout << title.name << "(" << title_types[title.type] << ")" << std::endl;

			break;
		}

	NU_PARSE(title, year, true);
	NU_PARSE(title, average_rating, true);

	NU_PARSE(title, best_rating, false);
	NU_PARSE(title, worst_rating, false);
	NU_PARSE(title, votes_num, true);
	NU_PARSE(title, rank, false);

	title.episodes_num = 1;
	NU_PARSE(title, episodes_num, false);

	return true;
}

bool site_info_t::sync_imdb(const std::string &username, const std::string &password, site_user_info_t &site_user)
{
	uint32_t prev_added_titles_num = 0;

	std::vector<title_info_t> site_titles;

	if(!authenticate_imdb(site_user))
		return false;

	pugi::xml_document doc;

	if(!load_xhtml(doc, http->redirect_to("http://" + url, login_cookie)))
		return false;

	std::string user_page_uri;
	parse(doc, parser_entity_t("//*[@id='navUserMenu']/p[@class='navCategory singleLine']/a/@href"), user_page_uri);

	if(!authenticate_imdb(site_user))
		return false;

	if(!load_xhtml(doc, http->redirect_to(user_page_uri, login_cookie)))
		return false;

	//parse(doc, parser_entity_t("//div[contains(@class, 'userId')]/@data-userId", "ur(\\d+)"), site_user.id);
	parse(doc, parser_entity_t("//div[@class='user-profile userId']/@data-userid", "ur(\\d+)"), site_user.id);

	for(uint32_t i = 0; i < imdb_lists.size(); ++i)
	{
		if(!authenticate_imdb(site_user))
			return false;

		if(!load_xhtml(doc, http->redirect_to("/user/ur" + std::to_string(site_user.id) + "/" + imdb_lists[i].name, login_cookie)))
			continue; //return false;

		//foreach_xml_xpath_node(node, doc, parser_info.titlelist_uri.xpath.c_str(), parser_info.titlelist_next_page_uri.xpath.c_str())
		{
			//tcout << node.name() << std::endl;

			//std::string titlelist_uri = node.attribute("href").value();

			//std::cout << titlelist_uri << std::endl;

			//if(!load_xhtml(doc, http->redirect_to(titlelist_uri, login_cookie)))
			//	return false;

			const pugi::xpath_node_set nodes = doc.select_nodes(parser_info.titlelist_title_uri.xpath.c_str());

			for(pugi::xpath_node_set::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
			{
				//pugi::xml_node node = it->node();
				const pugi::char_t *node_str = it->node() ? it->node().value() : it->attribute().value();

				title_info_t title; //{ node.first_child().value(), node.attribute("href").value() };
				user_title_info_t user_title = { 0 };

				user_title.index = titles.size();
				user_title.last_updated = Poco::Timestamp::TIMEVAL_MIN;

				if(!parse_title_and_user_title_info(site_user, node_str, title, user_title))
					return false;

				if(!send_request_get_title_rating_imdb(site_user, title, user_title.rating))
					return false;

				user_title.episodes_watched_num = title.episodes_num;
				user_title.status = imdb_lists[i].status;

				site_titles.push_back(title);

				std::vector<title_info_t>::const_iterator title_it = std::find_if(titles.begin(), titles.end(), std::bind2nd(compare_by_name<title_info_t>(), title));

				if(title_it != titles.end())
				{
					user_title.index = title_it - titles.begin();
					titles[user_title.index] = title;
				}
				else
				{
					titles.push_back(title);
				}

				std::vector<user_title_info_t>::iterator user_title_it = std::find_if(site_user.user_titles.begin(), site_user.user_titles.end(), std::bind2nd(compare_by_index<user_title_info_t>(), user_title));

				if(user_title_it != site_user.user_titles.end())
				{
					user_title_it->episodes_watched_num = title.episodes_num;
				}
				else
				{
					site_user.user_titles.push_back(user_title);
					++prev_added_titles_num;
				}
			}

			//prev_added_titles_num = titles.size();
		}

		remove_titles(site_user, site_titles);
	}

	return true;
}




bool site_info_t::parse_title_info_by_id_mal(site_user_info_t &site_user, title_info_t &title)
{
	// As MAL API doesn't have a method to get information by ID, we're using an
	// undocumented call that is normally used to display information bubbles
	// when hovering over anime/manga titles at the website. The downside is,
	// it doesn't provide all the information we need.
	pugi::xml_document doc;

	if(!load_xhtml(doc, http->send_request(HTTPRequest::HTTP_GET, parser_info.useragent, "/includes/ajax.inc.php?t=64&id=" + std::to_string(title.id), "", site_user.username, site_user.password)))
		return false;

	pugi::xml_node node = doc;//.first_child();

	// Available data:
	// - ID
	// - Title (truncated, followed by year aired)
	// - Synopsis (limited to 200 characters)
	// - Genres
	// - Status (in string form)
	// - Type (in string form)
	// - Episodes
	// - Score
	// - Rank
	// - Popularity
	// - Members
	//string_t id = InStr(http_response.body,
	//	L"/anime/", L"/");
	//string_t title = InStr(http_response.body,
	//	L"class=\"hovertitle\">", L"</a>");
	//string_t genres = InStr(http_response.body,
	//	L"Genres:</span> ", L"<br />");
	//string_t status = InStr(http_response.body,
	//	L"Status:</span> ", L"<br />");
	//string_t type = InStr(http_response.body,
	//	L"Type:</span> ", L"<br />");
	//string_t popularity = InStr(http_response.body,
	//	L"Popularity:</span> ", L"<br />");

	NU_PARSE(title, year, true);
	NU_PARSE(title, average_rating, false); // can be "Score: N/A"

	//NU_PARSE(title, best_rating, true);
	//NU_PARSE(title, worst_rating, true);
	NU_PARSE(title, votes_num, true);
	NU_PARSE(title, rank, false); // can be "Ranked: N/A"

	NU_PARSE(title, episodes_num, true);

	if(title.name.empty())
	{
		NU_PARSE(title, name, true);

		//bool title_is_truncated = false;

		//if (EndsWith(title, L")") && title.length() > 7)
		//	title = title.substr(0, title.length() - 7);
		//if (EndsWith(title, L"...") && title.length() > 3) {
		//	title = title.substr(0, title.length() - 3);
		//	title_is_truncated = true;
		//}
	}

	//std::vector<std::wstring> genres_vector;
	//Split(genres, L", ", genres_vector);

	//StripHtmlTags(score);
	//int pos = InStr(score, L" (", 0);
	//if (pos > -1)
	//	score.resize(pos);

	//TrimLeft(popularity, L"#");

	//::anime::Item anime_item;
	//anime_item.SetSource(this->id());
	//anime_item.SetId(id, this->id());
	//if (!title_is_truncated)
	//	anime_item.SetTitle(title);
	//anime_item.SetAiringStatus(TranslateSeriesStatusFrom(status));
	//anime_item.SetType(TranslateSeriesTypeFrom(type));
	//anime_item.SetEpisodeCount(ToInt(episodes));
	//anime_item.SetGenres(genres_vector);
	//anime_item.SetPopularity(ToInt(popularity));
	//anime_item.SetScore(ToDouble(score));
	//anime_item.SetLastModified(time(nullptr));  // current time

	return true;
}

bool site_info_t::sync_mal(const std::string &username, const std::string &password, site_user_info_t &site_user)
{
	return import_mal(http->send_request(HTTPRequest::HTTP_GET, parser_info.useragent, "/malappinfo.php?u=" + username + "&status=all", "", username, password), site_user);
}

bool site_info_t::import_mal(const std::string &xml_str, site_user_info_t &site_user)
{
	pugi::xml_document doc;

	if(!load_xml(doc, xml_str))
		return false;

	return import_mal(doc, site_user);
}

bool site_info_t::import_mal(pugi::xml_node &xml_doc_node, site_user_info_t &site_user)
{
	uint32_t prev_added_titles_num = 0;

	std::vector<title_info_t> site_titles;

	foreach_xml_node(node_myanimelist, xml_doc_node, "myanimelist")
	{
		// Available tags:
		// - user_id
		// - user_name
		// - user_watching
		// - user_completed
		// - user_onhold
		// - user_dropped
		// - user_plantowatch
		// - user_days_spent_watching
		//pugi::xml_node node_myinfo = node_myanimelist.child(L"myinfo");
		//user_.id = XmlReadStrValue(node_myinfo, L"user_id");
		//user_.username = XmlReadStrValue(node_myinfo, L"user_name");

		// Available tags:
		// - series_animedb_id
		// - series_title
		// - series_synonyms (separated by "; ")
		// - series_type
		// - series_episodes
		// - series_status
		// - series_start
		// - series_end
		// - series_image
		// - my_id (deprecated)
		// - my_watched_episodes
		// - my_start_date
		// - my_finish_date
		// - my_score
		// - my_status
		// - my_rewatching
		// - my_rewatching_ep
		// - my_last_updated
		// - my_tags
		foreach_xml_node(node, node_myanimelist, "anime")
		{
			title_info_t title;
			user_title_info_t user_title = { 0 };

			user_title.index = titles.size();
			user_title.last_updated = Poco::Timestamp::TIMEVAL_MIN;

			if(!parse_title_info_mal(node, site_user, title))
				return false;

			if(!parse_user_title_info_mal(node, user_title))
				return false;

			site_titles.push_back(title);

			std::vector<title_info_t>::const_iterator title_it = std::find_if(titles.begin(), titles.end(), std::bind2nd(compare_by_name<title_info_t>(), title));

			if(title_it != titles.end())
			{
				user_title.index = title_it - titles.begin();
				titles[user_title.index] = title;
			}
			else
			{
				titles.push_back(title);
			}

			std::vector<user_title_info_t>::iterator user_title_it = std::find_if(site_user.user_titles.begin(), site_user.user_titles.end(), std::bind2nd(compare_by_index<user_title_info_t>(), user_title));

			if(user_title_it != site_user.user_titles.end())
			{
				*user_title_it = user_title;
			}
			else
			{
				site_user.user_titles.push_back(user_title);
				++prev_added_titles_num;
			}
		}
	}

	remove_titles(site_user, site_titles);

	return true;
}

bool site_info_t::sync(const std::string &username, const std::string &password, site_user_info_t &site_user)
{
	if(sync_func)
		return sync_func(username, password, site_user);

	uint32_t prev_added_titles_num = 0;

	std::vector<title_info_t> site_titles;

	pugi::xml_document doc;

	if(!load_xhtml(doc, http->send_form(HTTPRequest::HTTP_POST, login_uri, username, password, login_cookie)))
		return false;

	foreach_xml_xpath_node(node, doc, parser_info.titlelist_uri.xpath.c_str(), parser_info.titlelist_next_page_uri.xpath.c_str())
	{
		tcout << node.name() << std::endl;

		std::string titlelist_uri = node.attribute("href").value();

		std::cout << titlelist_uri << std::endl;

		if(!load_xhtml(doc, http->redirect_to(titlelist_uri, login_cookie)))
			return false;

		const pugi::xpath_node_set nodes = doc.select_nodes(parser_info.titlelist_title_uri.xpath.c_str());

		for(pugi::xpath_node_set::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
		{
			//pugi::xml_node node = it->node();
			const pugi::char_t *node_str = it->node() ? it->node().value() : it->attribute().value();

			title_info_t title; //{ node.first_child().value(), node.attribute("href").value() };
			user_title_info_t user_title = { 0 };

			user_title.index = titles.size();
			user_title.last_updated = Poco::Timestamp::TIMEVAL_MIN;

			if(!parse_title_and_user_title_info(site_user, node_str, title, user_title))
				return false;

			site_titles.push_back(title);

			std::vector<title_info_t>::const_iterator title_it = std::find_if(titles.begin(), titles.end(), std::bind2nd(compare_by_name<title_info_t>(), title));

			if(title_it != titles.end())
			{
				user_title.index = title_it - titles.begin();
				titles[user_title.index] = title;
			}
			else
			{
				titles.push_back(title);
			}

			std::vector<user_title_info_t>::const_iterator user_title_it = std::find_if(site_user.user_titles.begin(), site_user.user_titles.end(), std::bind2nd(compare_by_index<user_title_info_t>(), user_title));

			if(user_title_it != site_user.user_titles.end())
			{
			}
			else
			{
				site_user.user_titles.push_back(user_title);
				++prev_added_titles_num;
			}
		}

		//prev_added_titles_num = titles.size();
	}

	remove_titles(site_user, site_titles);

	return true;
}


bool site_info_t::parse_title_info(pugi::xml_document &doc, site_user_info_t &site_user, const std::string &title_uri, title_info_t &title)
{
	title.uri = title_uri;

	if(title.uri.empty())
	{
		if(!parse_title_info_by_id_mal(site_user, title))
			return false;
	}
	else
	{
		if(login_cookie.empty())
		{
			if(!load_xhtml(doc, http->send_request(HTTPRequest::HTTP_GET, parser_info.useragent, title.uri, "", site_user.username, site_user.password/*, login_cookie*/)))
				return false;
		}
		else
			if(!load_xhtml(doc, http->redirect_to(title.uri, login_cookie)))
				return false;

		if(!parse_title_info(doc, site_user, title))
			return false;
	}

	return true;
}

//bool site_info_t::parse_title_info(pugi::xml_document &doc, const std::string &title_name, const std::string &title_uri, title_info_t &title)
//{
//	title.name = title_name;
//
//	// remove line breaks:
//	//title.name.erase(std::remove_if(title.name.begin(), title.name.end(), std::iscntrl), title.name.end());
//	//replace_whitespace(title.name);
//	std::replace_if(title.name.begin(), title.name.end(), std::iscntrl, ' ');
//
//	std::cout << title.name << "(" << title.uri << ")" << std::endl;
//
//	return parse_title_info(doc, title_uri, title);
//}
//
//bool site_info_t::parse_title_info(const std::string &title_name, const std::string &title_uri, title_info_t &title)
//{
//	pugi::xml_document doc;
//
//	if(!parse_title_info(doc, title_name, title_uri, title))
//		return false;
//
//	return true;
//}

bool site_info_t::parse_title_info(site_user_info_t &site_user, const std::string &title_uri, title_info_t &title)
{
	pugi::xml_document doc;

	if(!parse_title_info(doc, site_user, title_uri, title))
		return false;

	return true;
}

bool site_info_t::parse_user_title_info(site_user_info_t &site_user, const title_info_t &title, user_title_info_t &user_title)
{
	if(!authenticate(site_user))
		return false;

	pugi::xml_document doc;

	if(!load_xhtml(doc, http->redirect_to(title.uri, login_cookie)))
		return false;

	if(!parse_user_title_info(doc, user_title))
		return false;

	return true;
}

bool site_info_t::parse_user_title_info(site_user_info_t &site_user, user_title_info_t &user_title)
{
	return parse_user_title_info(site_user, titles[user_title.index], user_title);
}

bool site_info_t::parse_title_and_user_title_info(pugi::xml_document &doc, site_user_info_t &site_user, const std::string &title_uri, title_info_t &title, user_title_info_t &user_title)
{
	if(!parse_title_info(doc, site_user, title_uri, title))
		return false;

	if(!parse_user_title_info(doc, user_title))
		return false;

	return true;
}

bool site_info_t::parse_title_and_user_title_info(site_user_info_t &site_user, const std::string &title_uri, title_info_t &title, user_title_info_t &user_title)
{
	pugi::xml_document doc;

	if(!parse_title_and_user_title_info(doc, site_user, title_uri, title, user_title))
		return false;

	return true;
}

bool site_info_t::parse_title_info(pugi::xml_node &node, site_user_info_t &site_user, title_info_t &title)
{
	if(parse_title_info_func)
		return parse_title_info_func(node, site_user, title);

	NU_PARSE(title, cover_thumb_uri, true);
	NU_PARSE(title, cover_uri, true);

	//title.cover_texture.handle = 0;

	if(title.name.empty())
	{
		NU_PARSE(title, name, true);

		// remove line breaks:
		//title.name.erase(std::remove_if(title.name.begin(), title.name.end(), std::iscntrl), title.name.end());
		//replace_whitespace(title.name);
		std::replace_if(title.name.begin(), title.name.end(), std::iscntrl, ' ');

		std::cout << title.name << "(" << title.uri << ")" << std::endl;
	}

	NU_PARSE(title, id, true);

	std::string title_type_str; { bool found = parse(node, parser_info.title_type, title_type_str); }

	for(std::vector<std::string>::const_iterator title_type_it = title_types.begin(); title_type_it != title_types.end(); ++title_type_it)
		if(title_type_str.find(*title_type_it) != std::string::npos)
		{
			title.type = title_type_it - title_types.begin();

			std::cout << title.name << "(" << title_types[title.type] << ")" << std::endl;

			break;
		}

	NU_PARSE(title, year, true);
	NU_PARSE(title, average_rating, true);

	NU_PARSE(title, best_rating, true);
	NU_PARSE(title, worst_rating, true);
	NU_PARSE(title, votes_num, true);
	NU_PARSE(title, rank, true);

	NU_PARSE(title, episodes_num, true);

	return true;
}

bool site_info_t::parse_user_title_info(pugi::xml_node &node, user_title_info_t &user_title)
{
	std::string title_status_str; { bool found = parse(node, parser_info.title_status, title_status_str); }

	std::vector<const char *>::const_iterator title_status_it = std::find(title_statuses.begin(), title_statuses.end(), title_status_str);

	if(title_status_it != title_statuses.end())
	{
		user_title.status = title_status_it - title_statuses.begin();

		//std::cout << title.name << "(" << title_statuses[user_title.status] << ")" << std::endl;
	}

	NU_PARSE(user_title, episodes_watched_num, false/*user_title.status != NU_TITLE_STATUS_NOT_ADDED && user_title.status != NU_TITLE_STATUS_PLAN_TO_WATCH*/);
	NU_PARSE(user_title, times_watched_num,    user_title.status == NU_TITLE_STATUS_WATCHED);
	NU_PARSE(user_title, rating,               false/*user_title.status != NU_TITLE_STATUS_NOT_ADDED && user_title.status != NU_TITLE_STATUS_PLAN_TO_WATCH*/);

	return true;
}

//#undef NU_PARSE_F
#undef NU_PARSE

#define NU_PARSE(a, x, name, e) { pugi::xml_node node_child = node.child(name); if(node_child) pugi::serialization::read(node_child, a.x); if(e && !node) NU_ERROR(#a "." #x " not found at " #name); }

Poco::DateTime parse_date_mal(pugi::xml_node &node, const pugi::char_t *name)
{
	const pugi::char_t *node_value = (node.type() != pugi::node_pcdata) ? node.child_value() : node.value();

	int tzd;

	//return Poco::DateTimeParser::parse(Poco::DateTimeFormat::SORTABLE_FORMAT, node.child_value(name), tzd);
	return node_value && pugi::string_t(node_value).size() > 0 ? Poco::DateTimeParser::parse(node_value, tzd) : Poco::Timestamp(Poco::Timestamp::TIMEVAL_MIN);
}

bool site_info_t::parse_title_info_mal(pugi::xml_node &node, site_user_info_t &site_user, title_info_t &title)
{
	NU_PARSE(title, cover_thumb_uri, "series_image", true);
	//NU_PARSE(title, cover_uri, true);

	//title.cover_texture.handle = 0;

	if(title.name.empty())
		NU_PARSE(title, name, "series_title", true);

	//anime_item.SetSynonyms(XmlReadStrValue(node, L"series_synonyms"));

	NU_PARSE(title, id, "series_animedb_id", true);

	bool title_type_found = false;

	if(!title_type_found)
	{
		std::string title_type_str = node.child_value("series_type");

		for(std::vector<std::string>::const_iterator title_type_it = title_types.begin(); title_type_it != title_types.end(); ++title_type_it)
			if(title_type_str.find(*title_type_it) != std::string::npos)
			{
				title.type = title_type_it - title_types.begin();

				std::cout << title.name << "(" << title_types[title.type] << ")" << std::endl;

				title_type_found = true;
				break;
			}
	}

	if(!title_type_found)
	{
		int title_type = get_int_number(node.child_value("series_type"));

		std::vector<int>::const_iterator title_type_it = std::find(title_type_ids.begin(), title_type_ids.end(), title_type);

		if(title_type_it != title_type_ids.end())
		{
			title.type = title_type_it - title_type_ids.begin();

			std::cout << title.name << "(" << title_types[title.type] << ")" << std::endl;

			title_type_found = true;
		}
	}

	//anime_item.SetAiringStatus(TranslateSeriesStatusFrom(XmlReadIntValue(node, L"series_status")));

	Poco::DateTime series_start = parse_date_mal(node, "series_start");
	title.started = series_start.timestamp();

	Poco::DateTime series_end = parse_date_mal(node, "series_end");
	title.ended = series_end.timestamp();

	//NU_PARSE(title, year, "series_start", true);
	title.year = series_start.year();

	//	NU_PARSE(title, average_rating, true);

	//	NU_PARSE(title, best_rating, true);
	//	NU_PARSE(title, worst_rating, true);
	//	NU_PARSE(title, votes_num, true);
	//	NU_PARSE(title, rank, true);

	NU_PARSE(title, episodes_num, "series_episodes", true);

	//anime_item.AddtoUserList();

	if(!parse_title_info_by_id_mal(site_user, title))
		return false;

	if(title.cover_thumb_uri.empty() && !title.cover_texture.handle)
	{
		std::vector<title_info_t> last_found_titles;

		if(send_request_search_title_mal(site_user, title.name, last_found_titles))
			for(uint32_t i = 0; i <  last_found_titles.size(); ++i)
				if(last_found_titles[i].name == title.name) // exact match
				{
					title.cover_thumb_uri = last_found_titles[i].cover_thumb_uri;

					break;
				}
	}

	return true;
}

bool site_info_t::parse_title_info_search_entry(pugi::xml_node &node, title_info_t &title)
{
	NU_PARSE(title, cover_thumb_uri, "image", true);
	//NU_PARSE(title, cover_uri, true);

	//title.cover_texture.handle = 0;

	if(title.name.empty())
		NU_PARSE(title, name, "title", true);

	//anime_item.SetSynonyms(XmlReadStrValue(node, L"series_synonyms"));

	NU_PARSE(title, id, "id", true);

	std::string title_type_str = node.child_value("type");

	for(std::vector<std::string>::const_iterator title_type_it = title_types.begin(); title_type_it != title_types.end(); ++title_type_it)
		if(title_type_str.find(*title_type_it) != std::string::npos)
		{
			title.type = title_type_it - title_types.begin();

			std::cout << title.name << "(" << title_types[title.type] << ")" << std::endl;

			break;
		}

	//anime_item.SetAiringStatus(TranslateSeriesStatusFrom(XmlReadIntValue(node, L"series_status")));

	Poco::DateTime series_start = parse_date_mal(node, "start_date");
	title.started = series_start.timestamp();

	Poco::DateTime series_end = parse_date_mal(node, "end_date");
	title.ended = series_end.timestamp();

	NU_PARSE(title, episodes_num, "episodes", true);

	return true;
}

bool site_info_t::parse_user_title_info_mal(pugi::xml_node &node, user_title_info_t &user_title)
{
	bool title_status_found = false;

	if(!title_status_found)
	{
		std::string title_status_str = node.child_value("my_status");

		std::vector<const char *>::const_iterator title_status_it = std::find(title_statuses.begin(), title_statuses.end(), title_status_str);

		if(title_status_it != title_statuses.end())
		{
			user_title.status = title_status_it - title_statuses.begin();

			//std::cout << title.name << "(" << title_statuses[user_title.status] << ")" << std::endl;

			title_status_found = true;
		}
	}

	if(!title_status_found)
	{
		int title_status = get_int_number(node.child_value("my_status"));

		std::vector<int>::const_iterator title_status_it = std::find(title_status_ids.begin(), title_status_ids.end(), title_status);

		if(title_status_it != title_status_ids.end())
		{
			user_title.status = title_status_it - title_status_ids.begin();

			//std::cout << title.name << "(" << title_statuses[user_title.status] << ")" << std::endl;

			title_status_found = true;
		}
	}

	NU_PARSE(user_title, episodes_watched_num, "my_watched_episodes", false/*user_title.status != NU_TITLE_STATUS_NOT_ADDED && user_title.status != NU_TITLE_STATUS_PLAN_TO_WATCH*/);
	NU_PARSE(user_title, times_watched_num,    "my_rewatching",       user_title.status == NU_TITLE_STATUS_WATCHED);
	NU_PARSE(user_title, rating,               "my_score",            false/*user_title.status != NU_TITLE_STATUS_NOT_ADDED && user_title.status != NU_TITLE_STATUS_PLAN_TO_WATCH*/);

	Poco::DateTime my_start_date = parse_date_mal(node, "my_start_date");
	user_title.started = my_start_date.timestamp();

	Poco::DateTime my_finish_date = parse_date_mal(node, "my_finish_date");
	user_title.ended = my_finish_date.timestamp();

	//anime_item.SetMyRewatchingEp(XmlReadIntValue(node, L"my_rewatching_ep"));
	NU_PARSE(user_title, last_updated,    "my_last_updated",       false);
	//anime_item.SetMyTags(XmlReadStrValue(node, L"my_tags"));

	//AnimeDatabase.UpdateItem(anime_item);

	return true;
}

#undef NU_PARSE

bool site_info_t::send_request_change_title_episodes_watched_num(site_user_info_t &site_user, uint32_t i, uint32_t episodes_watched_num)
{
	return send_request_change_title_episodes_watched_num(site_user, titles[i], episodes_watched_num);
}

bool site_info_t::send_request_change_title_status(site_user_info_t &site_user, uint32_t i, uint32_t status)
{
	return send_request_change_title_status(site_user, titles[i], status);
}

bool site_info_t::send_request_change_title_rating(site_user_info_t &site_user, uint32_t i, float rating)
{
	return send_request_change_title_rating(site_user, titles[i], rating);
}

bool site_info_t::send_request_add_title(site_user_info_t &site_user, uint32_t i, uint32_t status)
{
	return send_request_add_title(site_user, titles[i], status);
}

bool site_info_t::send_request_delete_title(site_user_info_t &site_user, uint32_t i)
{
	return send_request_delete_title(site_user, titles[i]);
}



bool site_info_t::authenticate(site_user_info_t &site_user)
{
	if(authenticate_func)
		return authenticate_func(site_user);

	if(http->cookies.find(login_cookie) == http->cookies.end())
		http->send_form(HTTPRequest::HTTP_POST, login_uri, site_user.username, site_user.password, login_cookie);

	return http->cookies.find(login_cookie) != http->cookies.end();
}

bool site_info_t::send_request_change_title_episodes_watched_num(site_user_info_t &site_user, const title_info_t &title, uint32_t episodes_watched_num)
{
	if(send_request_change_title_episodes_watched_num_func)
		return send_request_change_title_episodes_watched_num_func(site_user, title, episodes_watched_num);

	if(!authenticate(site_user))
		return false;

	std::string json_request_str = make_json_request_str(std::string("ajax_set_episodes"),
														 title.id,
														 std::to_string(episodes_watched_num));

	return http->send_json_request(title.uri, json_request_str, login_cookie);
}

bool site_info_t::send_request_change_title_status(site_user_info_t &site_user, const title_info_t &title, uint32_t status)
{
	if(send_request_change_title_status_func)
		return send_request_change_title_status_func(site_user, title, status);

	if(!authenticate(site_user))
		return false;

	std::string json_request_str = make_json_request_str(std::string("ajax_set_status"),
														 title.id,
														 std::string("anime"),
														 std::to_string(title_status_ids[status]));

	return http->send_json_request(title.uri, json_request_str, login_cookie);
}

bool site_info_t::send_request_change_title_rating(site_user_info_t &site_user, const title_info_t &title, float rating)
{
	if(send_request_change_title_rating_func)
		return send_request_change_title_rating_func(site_user, title, rating);

	if(!authenticate(site_user))
		return false;

	std::string json_request_str = make_json_request_str(std::string("ajax_set_rating"),
														 title.id,
														 std::string("anime"),
														 std::to_string(rating / rating_mulcoef));

	return http->send_json_request(title.uri, json_request_str, login_cookie);
}

bool site_info_t::send_request_add_title(site_user_info_t &site_user, const title_info_t &title, uint32_t status)
{
	if(send_request_add_title_func)
		return send_request_add_title_func(site_user, title, status);

	return send_request_change_title_status(site_user, title, status);
}

bool site_info_t::send_request_delete_title(site_user_info_t &site_user, const title_info_t &title)
{
	if(send_request_delete_title_func)
		return send_request_delete_title_func(site_user, title);

	return send_request_change_title_status(site_user, title, NU_TITLE_STATUS_NOT_ADDED);
}

bool site_info_t::send_request_search_title(site_user_info_t &site_user, const std::string &title_name, std::vector<title_info_t> &found_titles)
{
	if(send_request_search_title_func)
		return send_request_search_title_func(site_user, title_name, found_titles);

	if(!authenticate(site_user))
		return false;

	std::string title_name_encoded_str;
	Poco::URI::encode(title_name, "", title_name_encoded_str);
	std::string search_uri = "/anime/all?name=" + title_name_encoded_str;

	pugi::xml_document doc;

	if(!load_xhtml(doc, http->redirect_to(search_uri, login_cookie)))
		return false;

	const pugi::xpath_node_set nodes = doc.select_nodes(parser_info.titlesearch_titles.xpath.c_str());

	for(pugi::xpath_node_set::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
	{
		pugi::xml_node node = it->node();

		title_info_t title; // = { /*node.first_child().value()*/"", node.attribute("href").value() };

		//// remove line breaks:
		////title.name.erase(std::remove_if(title.name.begin(), title.name.end(), std::iscntrl), title.name.end());
		////replace_whitespace(title.name);
		//std::replace_if(title.name.begin(), title.name.end(), std::iscntrl, ' ');

		if(parse_title_info(site_user, node.attribute("href").value(), title))
			found_titles.push_back(title);
	}

	return !found_titles.empty();
}

bool site_info_t::send_request_search_title_mal(site_user_info_t &site_user, const std::string &title_name, std::vector<title_info_t> &found_titles)
{
	std::string title_name_encoded_str;
	Poco::URI::encode(title_name, "", title_name_encoded_str);
	std::string search_uri = "/api/anime/search.xml?q=" + title_name_encoded_str;

	pugi::xml_document doc;

	std::vector<string_pair_t> params;

	if(!load_xml(doc, http->send_request(HTTPRequest::HTTP_GET, search_uri, params, parser_info.useragent, site_user.username, site_user.password, login_cookie)))
		return false;

	foreach_xml_node(node_anime, doc, "anime")
		foreach_xml_node(node, node_anime, "entry")
		{
			title_info_t title; // = { /*node.first_child().value()*/"", node.attribute("href").value() };

			//// remove line breaks:
			////title.name.erase(std::remove_if(title.name.begin(), title.name.end(), std::iscntrl), title.name.end());
			////replace_whitespace(title.name);
			//std::replace_if(title.name.begin(), title.name.end(), std::iscntrl, ' ');

			if(parse_title_info_search_entry(node, title))
				//if(parse_title_info_by_id_mal(site_user.username, site_user.password, title))
					found_titles.push_back(title);
		}

	return !found_titles.empty();
}


template<class T> std::string send_request_mal(http_session_t *http, std::string action, const std::string &useragent, const site_user_info_t &site_user, const title_info_t &title, const pugi::char_t *param_name, const T &param)
{
	//http_request.method = L"POST";
	//http_request.header[L"Content-Type"] = L"application/x-www-form-urlencoded";

	if(action.empty())
		action = "update";

	//http_request.url.path = L"/api/animelist/" + request.data[L"action"] + L"/" +
	//	request.data[canonical_name_ + L"-id"] + L".xml";

	std::vector<string_pair_t> params;
	params.push_back(std::make_pair("username", site_user.username));
	params.push_back(std::make_pair("password", site_user.password));

	// Delete method doesn't require us to provide additional data
	if(action != "delete")
	{
		pugi::xml_document doc;
		pugi::xml_node node_declaration = doc.append_child(pugi::node_declaration);
		node_declaration.append_attribute("version") = "1.0";
		node_declaration.append_attribute("encoding") = "UTF-8";
		pugi::xml_node node_entry = doc.append_child("entry");

	#if 0
		// MAL allows setting a new value to "times_rewatched" and others, but
		// there's no way to get the current value. So we avoid them altogether.
		const wchar_t* tags[] = {
			L"episode",
			L"status",
			L"score",
			//    L"downloaded_episodes",
			//    L"storage_type",
			//    L"storage_value",
			//    L"times_rewatched",
			//    L"rewatch_value",
			L"date_start",
			L"date_finish",
			//    L"priority",
			//    L"enable_discussion",
			L"enable_rewatching",
			//    L"comments",
			//    L"fansub_group",
			L"tags"
		};
		std::set<std::wstring> valid_tags(tags, tags + sizeof(tags) / sizeof(*tags));
		foreach_(it, request.data) {
			auto tag = valid_tags.find(TranslateKeyTo(it->first));
			if (tag != valid_tags.end()) {
				std::wstring value = it->second;
				if (*tag == L"status") {
					value = ToWstr(TranslateMyStatusTo(ToInt(value)));
				} else if (StartsWith(*tag, L"date")) {
					value = TranslateMyDateTo(value);
				}
				XmlWriteStrValue(node_entry, tag->c_str(), value.c_str());
			}
		}
	#endif

		pugi::serialization::write_child(node_entry, param, param_name);

		//pugi::serialization::write_child(node_entry, user_title.episodes_watched_num, "episode");
		//pugi::serialization::write_child(node_entry, user_title.status, "status");
		//pugi::serialization::write_child(node_entry, user_title.rating, "score");

		//pugi::serialization::write_child(node_entry, user_title.started, "date_start");
		//pugi::serialization::write_child(node_entry, user_title.ended, "date_finish");

		//pugi::serialization::write_child(node_entry, user_title.episodes_watched_num, "episode");
		//pugi::serialization::write_child(node_entry, user_title.episodes_watched_num, "episode");

		params.push_back(std::make_pair("data", pugi::serialization::node_to_string(doc)));
	}

	return http->send_request(HTTPRequest::HTTP_POST, "/api/animelist/" + action + "/" + std::to_string(title.id) + ".xml", params, useragent, site_user.username, site_user.password, "");
}

bool site_info_t::authenticate_mal(site_user_info_t &site_user)
{
	//if(http->cookies.find(login_cookie) == http->cookies.end())
	//	http->send_form(HTTPRequest::HTTP_POST, login_uri, username, password, login_cookie);

	//return http->cookies.find(login_cookie) != http->cookies.end();
	return true;
}

bool site_info_t::send_request_change_title_episodes_watched_num_mal(site_user_info_t &site_user, const title_info_t &title, uint32_t episodes_watched_num)
{
	std::string response_str = send_request_mal(http, "update", parser_info.useragent, site_user, title, "episode", episodes_watched_num);

	return response_str == "Updated";
}

bool site_info_t::send_request_change_title_status_mal(site_user_info_t &site_user, const title_info_t &title, uint32_t status)
{
	if(!status)
		return send_request_delete_title_mal(site_user, title);

	std::string response_str = send_request_mal(http, "update", parser_info.useragent, site_user, title, "status", title_status_ids[status]);

	return response_str == "Updated";
}

bool site_info_t::send_request_change_title_rating_mal(site_user_info_t &site_user, const title_info_t &title, float rating)
{
	std::string response_str = send_request_mal(http, "update", parser_info.useragent, site_user, title, "score", rating / rating_mulcoef);

	return response_str == "Updated";
}

bool site_info_t::send_request_add_title_mal(site_user_info_t &site_user, const title_info_t &title, uint32_t status)
{
	std::string response_str = send_request_mal(http, "add", parser_info.useragent, site_user, title, "status", title_status_ids[status]);

	return response_str.find("<title>201 Created</title>") != std::string::npos || 0 != get_int_number(response_str.c_str());
}

bool site_info_t::send_request_delete_title_mal(site_user_info_t &site_user, const title_info_t &title)
{
	std::string response_str = send_request_mal(http, "delete", parser_info.useragent, site_user, title, "", 0);

	return response_str == "Deleted";
}


void site_info_t::remove_titles(site_user_info_t &site_user, std::vector<title_info_t> &site_titles)
{
	// remove titles removed not from NowUpdater (from web or other client):
	for(int i = 0; i < site_user.user_titles.size(); ++i)
	{
		std::vector<title_info_t>::const_iterator title_it = std::find_if(site_titles.begin(), site_titles.end(), std::bind2nd(compare_by_name<title_info_t>(), titles[site_user.user_titles[i].index]));

		bool found = title_it != site_titles.end();

		if(!found)
		{
			uint32_t s_index = site_user.user_titles[i].index;
			titles.erase(titles.begin() + s_index);

			remove_title(site_user, i);
		}
	}
}

uint32_t site_info_t::remove_title(site_user_info_t &site_user, uint32_t i)
{
	uint32_t user_title_index = i;

	uint32_t s_index = site_user.user_titles[user_title_index].index;
	//std::vector<title_info_t>::iterator sit = sites[current_site].titles.erase(sites[current_site].titles.begin() + s_index);
	std::vector<user_title_info_t>::iterator it = site_user.user_titles.erase(site_user.user_titles.begin() + user_title_index);
	if(it != site_user.user_titles.end())
		user_title_index = it - site_user.user_titles.begin();
	else
		user_title_index = site_user.user_titles.size() - 1;

	// fix indexes:
	for(int i = 0; i < site_user.user_titles.size(); ++i)
		if(site_user.user_titles[i].index >= s_index)
			--site_user.user_titles[i].index;

	return user_title_index;
}
//-----------------------------------------------------------------------------------

