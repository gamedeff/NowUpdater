//
// NowUpdater
//
// nu_imdb_site_info.cpp
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#include "nu_imdb_site_info.h"
//-----------------------------------------------------------------------------------
#include "nu_user_info.h"
//-----------------------------------------------------------------------------------
#include "Poco/JSON/Parser.h"
#include "Poco/JSON/Object.h"

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
//-----------------------------------------------------------------------------------
imdb_site_info_t::imdb_site_info_t()
{
	name = "imdb";
	url = "www.imdb.com";
	login_uri = "https://secure.imdb.com/register-imdb/login";
	// Use mobile login because it actually works
	//login_uri = "https://secure.imdb.com/oauth/m_login?origpath=/";
	//login_cookie = "id|sid|uu|session-id|session-id-time|cache";
	login_cookie = "id|sid|uu|session-id|session-id-time";

	site_parser_info_t imdb_parser_info = 
	{
		"/html[1]/body[1]/div[1]/div[2]/div[1]/div[1]/ul[1]/li[1]/a[1]",
		"/html/body/div/div/ul[@class='nav']/li[@class='next']/a",

		"//div[@class='list_item odd']/div[@class='info']/b//a/text()", //"/html[1]/body[1]/div[2]/table[1]/tbody[1]/tr[*]/td[1]/a",  // name
		"//div[contains(@class, 'list_item')]/div[@class='info']/b//a/@href", //"/html[1]/body[1]/div[2]/table[1]/tbody[1]/tr[*]/td[1]/a",  // uri

		"//meta[@property='og:image']/@content",
		"//div[@class='poster']/a/img/@src",

		//"//div[@class='title_wrapper']/h1[@itemprop='name']/text()", // name
		"//h1[@itemprop='name']/text()", // name
		//parser_entity_t("//meta[@property='og:url']/@content", "http\\:\\/\\/www\\.imdb\\.com\\/title\\/tt(\\d{7})\\/"), // id
		parser_entity_t("//div[@class='ratings_wrapper']/div/@data-titleid", "tt(\\d+)"), // id
		"//meta[@property='og:type']/@content", //"//div[@id='siteContainer']/@itemtype", // type
		//parser_entity_t("//meta[@property='og:title']/@content", "\\((\\d{4})\\)"), // year
		"//meta[@itemprop='datePublished']/@content", // year
		"//div[@class='imdbRating']/div[@class='ratingValue']/strong/span[@itemprop='ratingValue']/text()", // average rating
		//"//meta[@itemprop='bestRating']/@content", // best rating
		"//span[@itemprop='bestRating']/text()", // best rating
		"//meta[@itemprop='worstRating']/@content", // worst rating
		parser_entity_t("//div[@class='imdbRating']/a/span[@itemprop='ratingCount']/text()", "(?:\\d|[,\\.])+"), // votes num
		parser_entity_t("//div[@class='titleReviewBarItem'][last()]/div[@class='titleReviewBarSubItem']/div/span[@class='subText']/text()", "(?:\\d|[,\\.])+"), // rank
		"//span[@class='bp_sub_heading']/text()", // episodes num
		//"concat(//span[@class='bp_sub_heading']/text(), substring('1', 1 div not(//span[@class='bp_sub_heading']/text())))", // episodes num

		//"//span[@itemprop='creator']/a[@itemprop='url']/span[@itemprop='name']/text()", // studio name
		"//span[@itemtype='http:////schema.org//Organization']/a[@itemprop='url']/span[@itemprop='name']/text()", // studio name
		"//span[@itemtype='http:////schema.org//Organization']/a[@itemprop='url']/@href", // studio uri

		"//form/select[@class='changeStatus']/option[@selected]/text()", //"/html/body/div/div/form/select[@class='changeStatus']/option[@selected]/text()", // status
		"//form/select[@class='episodes']/option[@selected]/text()", // episodes watched
		"//form/select[@class='timeswatched']/option[@selected]/text()", // times watched
		//"//div[@class='ratings_wrapper']/div[@class='inline']/div[@class='rating']/text()", // rating
		"//div[@class='rating rating-list']/span[@class='rating-rating rating-your']/span[@class='value']/text()", // rating

		"/html/body/div/table/tbody/tr/td[@class='tableType']/text()", //"/html[1]/body[1]/div[2]/table[1]/tbody[1]/tr[*]/td[2]",    // type
		"/html/body/div/table/tbody/tr/td[@class='tableYear']/a/text()", //"/html[1]/body[1]/div[2]/table[1]/tbody[1]/tr[*]/td[3]/a",  // year
		"/html/body/div/table/tbody/tr/td[@class='tableAverage']/text()",  //"/html[1]/body[1]/div[2]/table[1]/tbody[1]/tr[*]/td[4]",    // average rating

		"/html/body/div/table/tbody/tr/td[@class='epsRating']/form/select[@class='changeStatus']/option[@selected]/text()", //"/html/body/div/table/tbody/tr/td[@class='tableStatus']/text()", //"/html[1]/body[1]/div[2]/table[1]/tbody[1]/tr[*]/td[5]",    // status
		"/html/body/div/table/tbody/tr/td[@class='epsRating']/form/select[@class='episodes']/option[@selected]/text()", //"/html/body/div/table/tbody/tr/td[@class='tableEps']/text()",  //"/html[1]/body[1]/div[2]/table[1]/tbody[1]/tr[*]/td[6]"     // episodes watched
		"/html/body/div/table/tbody/tr/td[@class='epsRating']/form/select[@class='timeswatched']/option[@selected]/text()", //"/html/body/div/table/tbody/tr/td[@class='tableTimesWatched']/text()",
		"/html/body/div/table/tbody/tr/td[@class='epsRating']/form/div/text()", //"/html/body/div/table/tbody/tr/td[@class='tableRating']/div/text()",
		"/html/body/div/table/tbody/tr/td[@class='epsRating']/form/span[@class='totalEps']/text()",

		"//ul[@class='cardDeck pure-g cd-narrow']/li/a", //"/html/body/div/div/div/table/tbody/tr/td[@class='tableTitle']/a"
		"Mozilla/5.0 (Windows NT 6.2; rv:37.0) Gecko/20100101 Firefox/37.0"
	};

	parser_info = imdb_parser_info;

	http = new http_session_t(url);

	const char *IMDB_TT[] =
	{
		"video.other",
		"video.tv_show",
		"video.movie",
		"Special",
		"video.episode", //"OVA",
		"Web",
		"Music Video"
	};

	title_types.assign(IMDB_TT, IMDB_TT + countof(IMDB_TT));

	const int IMDB_TT_IDS[] =
	{
		0,
		1,
		2,
		3,
		4,
		5,
		6
	};

	title_type_ids.assign(IMDB_TT_IDS, IMDB_TT_IDS + countof(IMDB_TT_IDS));

	const char *IMDB_TS[] =
	{
		"Not added",
		"Watched",
		"Watching",
		"Want to Watch",
		"Stalled",
		"Dropped",
		"Won't Watch"
	};

	title_statuses.assign(IMDB_TS, IMDB_TS + countof(IMDB_TS));

	const int IMDB_TS_IDS[] =
	{
		0,
		1,
		2,
		4,
		5,
		3,
		6
	};

	title_status_ids.assign(IMDB_TS_IDS, IMDB_TS_IDS + countof(IMDB_TS_IDS));

	rating_mulcoef = 1;

	cover_image_scale_x = 0.15f;
	cover_image_scale_y = 0.15f;

	color = ImVec4(0.6f, 0.0f, 0.6f, 1.0f);

	imdb_list_t imdb_default_lists[] = 
	{
		{ "ratings",   "", "RATINGS",   NU_TITLE_STATUS_WATCHED },
		{ "watchlist", "", "WATCHLIST", NU_TITLE_STATUS_PLAN_TO_WATCH }
	};

	imdb_lists.assign(imdb_default_lists, imdb_default_lists + countof(imdb_default_lists));
}

std::string imdb_site_info_t::imdb_get_title_id_str(const title_info_t &title)
{
	std::string imdb_title_id = std::to_string(title.id); //imdb_get_title_id(title.name);
	assert(imdb_title_id.size() <= 7);
	std::string imdb_title_id_prefix = "tt";
	for(int i = 0; i < 7 - imdb_title_id.size(); ++i)
		imdb_title_id_prefix += "0";
	imdb_title_id = imdb_title_id_prefix + imdb_title_id;
	return imdb_title_id;
}


bool imdb_site_info_t::send_request_search_title(site_user_info_t &site_user, const std::string &title_name, std::vector<title_info_t> &found_titles)
{
	if(!authenticate(site_user))
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

						if(parse_title_info(doc, site_user, title))
							found_titles.push_back(title);
					}
				}
			}
		}
	}

	return !found_titles.empty();
}

std::string imdb_site_info_t::imdb_get_title_id(const std::string &title_name)
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
}

std::string imdb_site_info_t::imdb_get_auth_token(const std::string &imdb_title_id)
{
	pugi::xml_document doc;

	if(!load_xhtml(doc, http->redirect_to("http://www.imdb.com/title/" + imdb_title_id + "/", login_cookie)))
		return false;

	std::string data_auth;
	//parse(doc, parser_entity_t("//div[@class='rating rating-list']/@data-auth"), data_auth);
	parse(doc, parser_entity_t("//input[@id='seen-config']/@data-apptoken"), data_auth);

	return data_auth;
}

bool imdb_site_info_t::send_request_get_title_rating(site_user_info_t &site_user, const title_info_t &title, float &rating)
{
	if(!authenticate(site_user))
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


bool imdb_site_info_t::send_request_change_title_episodes_watched_num(site_user_info_t &site_user, const title_info_t &title, uint32_t episodes_watched_num)
{
	return false;
}

bool imdb_site_info_t::send_request_change_title_status(site_user_info_t &site_user, const title_info_t &title, uint32_t status)
{
	if(!authenticate(site_user))
		return false;

	//for(std::vector<imdb_list_t>::iterator imdb_lists_it = imdb_lists.begin(); imdb_lists_it != imdb_lists.end(); ++imdb_lists_it)
	//for(uint32_t i = 0; i < imdb_lists.size(); ++i)
	//	if(imdb_lists[i].status == status)
	//		send_request_add_title_imdb_list_ajax(site_user, title, imdb_list[i]);

	//return http->send_json_request(title.uri, json_request_str, login_cookie);
}

bool imdb_site_info_t::send_request_change_title_rating(site_user_info_t &site_user, const title_info_t &title, float rating)
{
	http->domain = "www.imdb.com";

	std::string imdb_title_id = imdb_get_title_id(title.name);
	//std::string imdb_title_id = imdb_get_title_id_str(title);

	pugi::xml_document doc;

	if(!load_xhtml(doc, http->redirect_to("http://www.imdb.com/title/" + imdb_title_id + "/", login_cookie)))
		return false;

	std::string rate_url;
	parse(doc, parser_entity_t("//a[contains(@href, \"vote?v=" + std::to_string(uint32_t(rating / rating_mulcoef)) + "\")]/@href"), rate_url);

	return rate_url.empty() ? false : load_xhtml(doc, http->redirect_to(rate_url, login_cookie));
}

// other way to do it:
bool imdb_site_info_t::send_request_change_title_rating_imdb_ajax(site_user_info_t &site_user, const title_info_t &title, float rating)
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


bool imdb_site_info_t::send_request_get_list_item_id_imdb_list_ajax(site_user_info_t &site_user, const title_info_t &title, imdb_list_t &imdb_list, std::string &imdb_list_item_id, std::string &imdb_hidden_key_name, std::string &imdb_hidden_key)
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

bool imdb_site_info_t::send_request_delete_title_imdb_list_ajax(site_user_info_t &site_user, const title_info_t &title, imdb_list_t &imdb_list)
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

bool imdb_site_info_t::send_request_add_title(site_user_info_t &site_user, const title_info_t &title, uint32_t status)
{
	if(!authenticate(site_user))
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

bool imdb_site_info_t::send_request_delete_title(site_user_info_t &site_user, const title_info_t &title)
{
	return send_request_change_title_rating_imdb_ajax(site_user, title, 0);
}

bool imdb_site_info_t::authenticate(site_user_info_t &site_user)
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

bool imdb_site_info_t::authenticate_imdb_with_tesseract(site_user_info_t &site_user)
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

//#define NU_PARSE_F(a, x, f, e)	{ bool found = parse_f(doc, parser_info.title_##x, a.x, f); if(e && !found) NU_ERROR(#a "." #x " not found at " + parser_info.title_##x.xpath); }
#define NU_PARSE(a, x, e)       { bool found = parse(node, parser_info.title_##x, a.x);      if(e && !found) NU_ERROR(#a "." #x " not found:" + (parser_info.title_##x.xpath.empty() ? parser_info.title_##x.xpath : " at \"" + parser_info.title_##x.xpath + "\"") + (parser_info.title_##x.regexp.empty() ? parser_info.title_##x.regexp : " no match for \"" + parser_info.title_##x.regexp + "\"")); }

bool imdb_site_info_t::parse_title_info(pugi::xml_node &node, site_user_info_t &site_user, title_info_t &title)
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

//#undef NU_PARSE_F
#undef NU_PARSE

bool imdb_site_info_t::sync(const std::string &username, const std::string &password, site_user_info_t &site_user)
{
	uint32_t prev_added_titles_num = 0;

	std::vector<title_info_t> site_titles;

	if(!authenticate(site_user))
		return false;

	pugi::xml_document doc;

	if(!load_xhtml(doc, http->redirect_to("http://" + url, login_cookie)))
		return false;

	std::string user_page_uri;
	parse(doc, parser_entity_t("//*[@id='navUserMenu']/p[@class='navCategory singleLine']/a/@href"), user_page_uri);

	if(!authenticate(site_user))
		return false;

	if(!load_xhtml(doc, http->redirect_to(user_page_uri, login_cookie)))
		return false;

	//parse(doc, parser_entity_t("//div[contains(@class, 'userId')]/@data-userId", "ur(\\d+)"), site_user.id);
	parse(doc, parser_entity_t("//div[@class='user-profile userId']/@data-userid", "ur(\\d+)"), site_user.id);

	for(uint32_t i = 0; i < imdb_lists.size(); ++i)
	{
		if(!authenticate(site_user))
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

				if(!send_request_get_title_rating(site_user, title, user_title.rating))
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
//-----------------------------------------------------------------------------------

