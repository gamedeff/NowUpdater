//
// NowUpdater
//
// nu_myanimelist_site_info.cpp
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#include "nu_myanimelist_site_info.h"
//-----------------------------------------------------------------------------------
#include "nu_user_info.h"
//-----------------------------------------------------------------------------------
#include "Poco/DateTimeParser.h"
//-----------------------------------------------------------------------------------
myanimelist_site_info_t::myanimelist_site_info_t()
{
	name = "myanimelist";
	url = "myanimelist.net";
	login_uri = "";
	login_cookie = "";

	site_parser_info_t mal_parser_info = 
	{
		"/html[1]/body[1]/div[1]/div[2]/div[1]/div[1]/ul[1]/li[1]/a[1]",
		"/html/body/div/div/ul[@class='nav']/li[@class='next']/a",

		"/a[@class='hovertitle']",  // name
		"/a[@class='hovertitle']",  // uri

		"//meta[@property='og:image']/@content",
		"//div[@class='mainEntry']/img[@class='screenshots']/@src", //"/html/body/div/div/div/div/div/div[@class='mainEntry']/img[@class='screenshots']/@src",

		parser_entity_t("//a[@class='hovertitle']/text()", "(.*) \\(\\d{4}\\)"), // name
		"/html/body/div/div/form/@data-id", // id
		"/html/body/div[@id='siteContainer']/div[2]/text()", //"//div[@id='siteContainer']/@itemtype", // type
		parser_entity_t("//a[@class='hovertitle']/text()", "\\((\\d{4})\\)"), // year
		parser_entity_t("", "Score:\\s((?:\\d|[,\\.])+)"), // average rating
		"//meta[@itemprop='bestRating']/@content", // best rating
		"//meta[@itemprop='worstRating']/@content", // worst rating
		parser_entity_t("", "scored by\\s((?:\\d|[,\\.])+)"), // votes num
		parser_entity_t("", "Ranked:\\s#((?:\\d|[,\\.])+)"), // rank
		parser_entity_t("", "Episodes:\\s(\\d+)"), // episodes num

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

		"//ul[@class='cardDeck pure-g cd-narrow']/li/a", //"/html/body/div/div/div/table/tbody/tr/td[@class='tableTitle']/a",

		// Since October 2013, third-party applications need to identify themselves
		// with a unique user-agent string that is whitelisted by MAL. Using a generic
		// value (e.g. "Mozilla/5.0") or an arbitrary one (e.g. "NowUpdater/1.0") will
		// result in invalid text/html responses, courtesy of Incapsula.
		// To get your own whitelisted user-agent string, follow the registration link
		// at the official MAL API club page (http://myanimelist.net/forum/?topicid=692311).
		// If, for any reason, you'd like to use NowUpdater's instead, I will appreciate it if you ask beforehand.
		"api-taiga-32864c09ef538453b4d8110734ee355b"
	};

	parser_info = mal_parser_info;

	http = new http_session_t(url);

	const char *MAL_TT[] =
	{
		"Unknown",
		"TV",
		"Movie",
		"Special",
		"OVA",
		"ONA",
		"Music"
	};

	title_types.assign(MAL_TT, MAL_TT + countof(MAL_TT));

	const int MAL_TT_IDS[] =
	{
		0,
		1,
		3,
		4,
		2,
		5,
		6
	};

	title_type_ids.assign(MAL_TT_IDS, MAL_TT_IDS + countof(MAL_TT_IDS));

	const char *MAL_TS[] =
	{
		"Not added",
		"Completed",
		"Watching",
		"Plan to Watch",
		"On Hold",
		"Dropped",
		"Won't Watch"
	};

	title_statuses.assign(MAL_TS, MAL_TS + countof(MAL_TS));

	const int MAL_TS_IDS[] =
	{
		0,
		2,
		1,
		6,
		3,
		4,
		4 // "Won't Watch"
	};

	title_status_ids.assign(MAL_TS_IDS, MAL_TS_IDS + countof(MAL_TS_IDS));

	rating_mulcoef = 1;

	cover_image_scale_x = 0.3f;
	cover_image_scale_y = 0.3f;

	color = ImVec4(0.0f, 0.6f, 0.6f, 1.0f);
}

//#define NU_PARSE_F(a, x, f, e)	{ bool found = parse_f(doc, parser_info.title_##x, a.x, f); if(e && !found) NU_ERROR(#a "." #x " not found at " + parser_info.title_##x.xpath); }
#define NU_PARSE(a, x, e)       { bool found = parse(node, parser_info.title_##x, a.x);      if(e && !found) NU_ERROR(#a "." #x " not found:" + (parser_info.title_##x.xpath.empty() ? parser_info.title_##x.xpath : " at \"" + parser_info.title_##x.xpath + "\"") + (parser_info.title_##x.regexp.empty() ? parser_info.title_##x.regexp : " no match for \"" + parser_info.title_##x.regexp + "\"")); }

bool myanimelist_site_info_t::parse_title_info_by_id(site_user_info_t &site_user, title_info_t &title)
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

//#undef NU_PARSE_F
#undef NU_PARSE

#define NU_PARSE_XML(a, x, name, e) { pugi::xml_node node_child = node.child(name); if(node_child) pugi::serialization::read(node_child, a.x); if(e && !node) NU_ERROR(#a "." #x " not found at " #name); }

Poco::DateTime parse_date_mal(pugi::xml_node &node, const pugi::char_t *name)
{
	const pugi::char_t *node_value = (node.type() != pugi::node_pcdata) ? node.child_value() : node.value();

	int tzd;

	//return Poco::DateTimeParser::parse(Poco::DateTimeFormat::SORTABLE_FORMAT, node.child_value(name), tzd);
	return node_value && pugi::string_t(node_value).size() > 0 ? Poco::DateTimeParser::parse(node_value, tzd) : Poco::Timestamp(Poco::Timestamp::TIMEVAL_MIN);
}

bool myanimelist_site_info_t::parse_title_info(pugi::xml_node &node, site_user_info_t &site_user, title_info_t &title)
{
	NU_PARSE_XML(title, cover_thumb_uri, "series_image", true);
	//NU_PARSE(title, cover_uri, true);

	//title.cover_texture.handle = 0;

	if(title.name.empty())
		NU_PARSE_XML(title, name, "series_title", true);

	//anime_item.SetSynonyms(XmlReadStrValue(node, L"series_synonyms"));

	NU_PARSE_XML(title, id, "series_animedb_id", true);

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

	//NU_PARSE_XML(title, year, "series_start", true);
	title.year = series_start.year();

	//	NU_PARSE(title, average_rating, true);

	//	NU_PARSE(title, best_rating, true);
	//	NU_PARSE(title, worst_rating, true);
	//	NU_PARSE(title, votes_num, true);
	//	NU_PARSE(title, rank, true);

	NU_PARSE_XML(title, episodes_num, "series_episodes", true);

	//anime_item.AddtoUserList();

	if(!parse_title_info_by_id(site_user, title))
		return false;

	if(title.cover_thumb_uri.empty() && !title.cover_texture.handle)
	{
		std::vector<title_info_t> last_found_titles;

		if(send_request_search_title(site_user, title.name, last_found_titles))
			for(uint32_t i = 0; i <  last_found_titles.size(); ++i)
				if(last_found_titles[i].name == title.name) // exact match
				{
					title.cover_thumb_uri = last_found_titles[i].cover_thumb_uri;

					break;
				}
	}

	return true;
}

bool myanimelist_site_info_t::parse_title_info_search_entry(pugi::xml_node &node, title_info_t &title)
{
	NU_PARSE_XML(title, cover_thumb_uri, "image", true);
	//NU_PARSE(title, cover_uri, true);

	//title.cover_texture.handle = 0;

	if(title.name.empty())
		NU_PARSE_XML(title, name, "title", true);

	//anime_item.SetSynonyms(XmlReadStrValue(node, L"series_synonyms"));

	NU_PARSE_XML(title, id, "id", true);

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

	NU_PARSE_XML(title, episodes_num, "episodes", true);

	return true;
}

bool myanimelist_site_info_t::parse_user_title_info(pugi::xml_node &node, user_title_info_t &user_title)
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

	NU_PARSE_XML(user_title, episodes_watched_num, "my_watched_episodes", false/*user_title.status != NU_TITLE_STATUS_NOT_ADDED && user_title.status != NU_TITLE_STATUS_PLAN_TO_WATCH*/);
	NU_PARSE_XML(user_title, times_watched_num,    "my_rewatching",       user_title.status == NU_TITLE_STATUS_WATCHED);
	NU_PARSE_XML(user_title, rating,               "my_score",            false/*user_title.status != NU_TITLE_STATUS_NOT_ADDED && user_title.status != NU_TITLE_STATUS_PLAN_TO_WATCH*/);

	Poco::DateTime my_start_date = parse_date_mal(node, "my_start_date");
	user_title.started = my_start_date.timestamp();

	Poco::DateTime my_finish_date = parse_date_mal(node, "my_finish_date");
	user_title.ended = my_finish_date.timestamp();

	//anime_item.SetMyRewatchingEp(XmlReadIntValue(node, L"my_rewatching_ep"));
	NU_PARSE_XML(user_title, last_updated,    "my_last_updated",       false);
	//anime_item.SetMyTags(XmlReadStrValue(node, L"my_tags"));

	//AnimeDatabase.UpdateItem(anime_item);

	return true;
}

#undef NU_PARSE_XML

bool myanimelist_site_info_t::send_request_search_title(site_user_info_t &site_user, const std::string &title_name, std::vector<title_info_t> &found_titles)
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

bool myanimelist_site_info_t::authenticate(site_user_info_t &site_user)
{
	//if(http->cookies.find(login_cookie) == http->cookies.end())
	//	http->send_form(HTTPRequest::HTTP_POST, login_uri, username, password, login_cookie);

	//return http->cookies.find(login_cookie) != http->cookies.end();
	return true;
}

bool myanimelist_site_info_t::sync(const std::string &username, const std::string &password, site_user_info_t &site_user)
{
	return import(http->send_request(HTTPRequest::HTTP_GET, parser_info.useragent, "/malappinfo.php?u=" + username + "&status=all", "", username, password), site_user);
}

bool myanimelist_site_info_t::import(const std::string &xml_str, site_user_info_t &site_user)
{
	pugi::xml_document doc;

	if(!load_xml(doc, xml_str))
		return false;

	return import(doc, site_user);
}

bool myanimelist_site_info_t::import(pugi::xml_node &xml_doc_node, site_user_info_t &site_user)
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

			if(!parse_title_info(node, site_user, title))
				return false;

			if(!parse_user_title_info(node, user_title))
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

bool myanimelist_site_info_t::send_request_change_title_episodes_watched_num(site_user_info_t &site_user, const title_info_t &title, uint32_t episodes_watched_num)
{
	std::string response_str = send_request_mal(http, "update", parser_info.useragent, site_user, title, "episode", episodes_watched_num);

	return response_str == "Updated";
}

bool myanimelist_site_info_t::send_request_change_title_status(site_user_info_t &site_user, const title_info_t &title, uint32_t status)
{
	if(!status)
		return send_request_delete_title(site_user, title);

	std::string response_str = send_request_mal(http, "update", parser_info.useragent, site_user, title, "status", title_status_ids[status]);

	return response_str == "Updated";
}

bool myanimelist_site_info_t::send_request_change_title_rating(site_user_info_t &site_user, const title_info_t &title, float rating)
{
	std::string response_str = send_request_mal(http, "update", parser_info.useragent, site_user, title, "score", rating / rating_mulcoef);

	return response_str == "Updated";
}

bool myanimelist_site_info_t::send_request_add_title(site_user_info_t &site_user, const title_info_t &title, uint32_t status)
{
	std::string response_str = send_request_mal(http, "add", parser_info.useragent, site_user, title, "status", title_status_ids[status]);

	return response_str.find("<title>201 Created</title>") != std::string::npos || 0 != get_int_number(response_str.c_str());
}

bool myanimelist_site_info_t::send_request_delete_title(site_user_info_t &site_user, const title_info_t &title)
{
	std::string response_str = send_request_mal(http, "delete", parser_info.useragent, site_user, title, "", 0);

	return response_str == "Deleted";
}

//-----------------------------------------------------------------------------------
