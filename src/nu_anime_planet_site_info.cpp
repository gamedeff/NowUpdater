//
// NowUpdater
//
// nu_anime_planet_site_info.cpp
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#include "nu_anime_planet_site_info.h"
//-----------------------------------------------------------------------------------
#include "nu_user_info.h"
#include "nu_json.h"
//-----------------------------------------------------------------------------------
#include <cwctype>
//-----------------------------------------------------------------------------------
anime_planet_site_info_t::anime_planet_site_info_t()
{
	name = "anime_planet";
	url = "www.anime-planet.com";
	login_uri = "/login.php?";
	login_cookie = "ap";

	site_parser_info_t ap_parser_info = 
	{
		"/html[1]/body[1]/div[1]/div[2]/div[1]/div[1]/ul[1]/li[1]/a[1]",
		"/html/body/div/div/ul[@class='nav']/li[@class='next']/a",

		"/html/body/div/table/tbody/tr/td[@class='tableTitle']/a/text()", //"/html[1]/body[1]/div[2]/table[1]/tbody[1]/tr[*]/td[1]/a",  // name
		"/html/body/div/table/tbody/tr/td[@class='tableTitle']/a/@href", //"/html[1]/body[1]/div[2]/table[1]/tbody[1]/tr[*]/td[1]/a",  // uri

		"//meta[@property='og:image']/@content",
		"//div[@class='mainEntry']/img[@class='screenshots']/@src", //"/html/body/div/div/div/div/div/div[@class='mainEntry']/img[@class='screenshots']/@src",

		"//h1[@itemprop='name']/text()", // name
		"/html/body/div/div/form/@data-id", // id
		"/html/body/div[@id='siteContainer']/div[2]/text()", //"//div[@id='siteContainer']/@itemtype", // type
		"//span[@class='iconYear']/span/a/text()", // year
		"/html/body/div[@id='siteContainer']/div/div[@class='avgRating']/span/text()", //"//meta[@itemprop='ratingValue']/@content", // average rating
		"//meta[@itemprop='bestRating']/@content", // best rating
		"//meta[@itemprop='worstRating']/@content", // worst rating
		parser_entity_t("//meta[@itemprop='ratingCount']/@content", "(?:\\d|[,\\.])+"), // votes num
		parser_entity_t("/html/body/div[@id='siteContainer']/div[6]/text()", "(?:\\d|[,\\.])+"), // rank
		"/html/body/div/div/form/select[@class='episodes']/@data-eps", // episodes num

		"/html/body/div[@id='siteContainer']/div[3]/text()", // studio name
		"/html/body/div[@id='siteContainer']/div[3]/a/@href", // studio uri

		"//form/select[@class='changeStatus']/option[@selected]/text()", //"/html/body/div/div/form/select[@class='changeStatus']/option[@selected]/text()", // status
		"//form/select[@class='episodes']/option[@selected]/text()", // episodes watched
		"//form/select[@class='timeswatched']/option[@selected]/text()", // times watched
		"/html/body/div/div/form/div/text()", // rating

		"/html/body/div/table/tbody/tr/td[@class='tableType']/text()", //"/html[1]/body[1]/div[2]/table[1]/tbody[1]/tr[*]/td[2]",    // type
		"/html/body/div/table/tbody/tr/td[@class='tableYear']/a/text()", //"/html[1]/body[1]/div[2]/table[1]/tbody[1]/tr[*]/td[3]/a",  // year
		"/html/body/div/table/tbody/tr/td[@class='tableAverage']/text()",  //"/html[1]/body[1]/div[2]/table[1]/tbody[1]/tr[*]/td[4]",    // average rating

		"/html/body/div/table/tbody/tr/td[@class='epsRating']/form/select[@class='changeStatus']/option[@selected]/text()", //"/html/body/div/table/tbody/tr/td[@class='tableStatus']/text()", //"/html[1]/body[1]/div[2]/table[1]/tbody[1]/tr[*]/td[5]",    // status
		"/html/body/div/table/tbody/tr/td[@class='epsRating']/form/select[@class='episodes']/option[@selected]/text()", //"/html/body/div/table/tbody/tr/td[@class='tableEps']/text()",  //"/html[1]/body[1]/div[2]/table[1]/tbody[1]/tr[*]/td[6]"     // episodes watched
		"/html/body/div/table/tbody/tr/td[@class='epsRating']/form/select[@class='timeswatched']/option[@selected]/text()", //"/html/body/div/table/tbody/tr/td[@class='tableTimesWatched']/text()",
		"/html/body/div/table/tbody/tr/td[@class='epsRating']/form/div/text()", //"/html/body/div/table/tbody/tr/td[@class='tableRating']/div/text()",
		"/html/body/div/table/tbody/tr/td[@class='epsRating']/form/span[@class='totalEps']/text()",

		"//ul[@class='cardDeck pure-g cd-narrow']/li/a" //"/html/body/div/div/div/table/tbody/tr/td[@class='tableTitle']/a"
	};

	parser_info = ap_parser_info;

	http = new http_session_t(url);

	const char *AP_TT[] =
	{
		"Other",
		"TV",
		"Movie",
		"Special",
		"OVA",
		"Web",
		"Music Video"
	};

	title_types.assign(AP_TT, AP_TT + countof(AP_TT));

	const int AP_TT_IDS[] =
	{
		0,
		1,
		2,
		3,
		4,
		5,
		6
	};

	title_type_ids.assign(AP_TT_IDS, AP_TT_IDS + countof(AP_TT_IDS));

	const char *AP_TS[] =
	{
		"Not added",
		"Watched",
		"Watching",
		"Want to Watch",
		"Stalled",
		"Dropped",
		"Won't Watch"
	};

	title_statuses.assign(AP_TS, AP_TS + countof(AP_TS));

	const int AP_TS_IDS[] =
	{
		0,
		1,
		2,
		4,
		5,
		3,
		6
	};

	title_status_ids.assign(AP_TS_IDS, AP_TS_IDS + countof(AP_TS_IDS));

	rating_mulcoef = 2;

	cover_image_scale_x = 0.3f;
	cover_image_scale_y = 0.3f;

	color = ImVec4(0.6f, 0.6f, 0.0f, 1.0f);
}

bool anime_planet_site_info_t::authenticate(site_user_info_t &site_user)
{
	if(http->cookies.find(login_cookie) == http->cookies.end())
		http->send_form(HTTPRequest::HTTP_POST, login_uri, site_user.username, site_user.password, login_cookie);

	return http->cookies.find(login_cookie) != http->cookies.end();
}

bool anime_planet_site_info_t::send_request_change_title_episodes_watched_num(site_user_info_t &site_user, const title_info_t &title, uint32_t episodes_watched_num)
{
	if(!authenticate(site_user))
		return false;

	std::string json_request_str = make_json_request_str(std::string("ajax_set_episodes"),
														 title.id,
														 std::to_string(episodes_watched_num));

	return http->send_json_request(title.uri, json_request_str, login_cookie);
}

bool anime_planet_site_info_t::send_request_change_title_status(site_user_info_t &site_user, const title_info_t &title, uint32_t status)
{
	if(!authenticate(site_user))
		return false;

	std::string json_request_str = make_json_request_str(std::string("ajax_set_status"),
														 title.id,
														 std::string("anime"),
														 std::to_string(title_status_ids[status]));

	return http->send_json_request(title.uri, json_request_str, login_cookie);
}

bool anime_planet_site_info_t::send_request_change_title_rating(site_user_info_t &site_user, const title_info_t &title, float rating)
{
	if(!authenticate(site_user))
		return false;

	std::string json_request_str = make_json_request_str(std::string("ajax_set_rating"),
														 title.id,
														 std::string("anime"),
														 std::to_string(rating / rating_mulcoef));

	return http->send_json_request(title.uri, json_request_str, login_cookie);
}

bool anime_planet_site_info_t::send_request_add_title(site_user_info_t &site_user, const title_info_t &title, uint32_t status)
{
	return send_request_change_title_status(site_user, title, status);
}

bool anime_planet_site_info_t::send_request_delete_title(site_user_info_t &site_user, const title_info_t &title)
{
	return send_request_change_title_status(site_user, title, NU_TITLE_STATUS_NOT_ADDED);
}

bool anime_planet_site_info_t::send_request_search_title(site_user_info_t &site_user, const std::string &title_name, std::vector<title_info_t> &found_titles)
{
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

		if(site_info_t::parse_title_info(site_user, node.attribute("href").value(), title))
			found_titles.push_back(title);
	}

	return !found_titles.empty();
}

bool anime_planet_site_info_t::sync(const std::string &username, const std::string &password, site_user_info_t &site_user)
{
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

//#define NU_PARSE_F(a, x, f, e)	{ bool found = parse_f(doc, parser_info.title_##x, a.x, f); if(e && !found) NU_ERROR(#a "." #x " not found at " + parser_info.title_##x.xpath); }
#define NU_PARSE(a, x, e)       { bool found = parse(node, parser_info.title_##x, a.x);      if(e && !found) NU_ERROR(#a "." #x " not found:" + (parser_info.title_##x.xpath.empty() ? parser_info.title_##x.xpath : " at \"" + parser_info.title_##x.xpath + "\"") + (parser_info.title_##x.regexp.empty() ? parser_info.title_##x.regexp : " no match for \"" + parser_info.title_##x.regexp + "\"")); }

bool anime_planet_site_info_t::parse_title_info(pugi::xml_node &node, site_user_info_t &site_user, title_info_t &title)
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
		std::replace_if(title.name.begin(), title.name.end(), std::iswcntrl, ' ');

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

//#undef NU_PARSE_F
#undef NU_PARSE
//-----------------------------------------------------------------------------------
