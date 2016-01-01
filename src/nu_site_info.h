//
// NowUpdater
//
// nu_site_info.h
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#ifndef NU_SITE_INFO_H
#define NU_SITE_INFO_H
//-----------------------------------------------------------------------------------
#include "nu_http_session.h"

#include "nu_xml.h"

#include "nu_site_parser.h"

#include "Poco//Timestamp.h"

#include "imgui.h"
//-----------------------------------------------------------------------------------
enum title_status_t
{
	NU_TITLE_STATUS_NOT_ADDED,
	NU_TITLE_STATUS_WATCHED,
	NU_TITLE_STATUS_WATCHING,
	NU_TITLE_STATUS_PLAN_TO_WATCH,
	NU_TITLE_STATUS_STALLED,
	NU_TITLE_STATUS_DROPPED,
	NU_TITLE_STATUS_WONT_WATCH
};

enum title_type_t
{
	NU_TITLE_TYPE_UNKNOWN,
	NU_TITLE_TYPE_SERIES,
	NU_TITLE_TYPE_MOVIE,
	NU_TITLE_TYPE_SPECIAL,
	NU_TITLE_TYPE_OVA,
	NU_TITLE_TYPE_WEB_ONA,
	NU_TITLE_TYPE_MUSIC_VIDEO
};

struct title_info_t;

struct episode_info_t
{
	uint32_t number;

	Poco::Timestamp airdate;

	std::string name, uri;

	std::vector<uint32_t> title_indexes;
};

struct season_info_t
{
	std::vector<episode_info_t> episodes; 
};

struct title_image_t
{
	uint32_t w, h;

	void *handle;

	title_image_t() : handle(0) {}
};

struct title_info_t
{
	std::string name, uri;

	std::string cover_thumb_uri, cover_uri;

	std::string cover_texture_data;

	title_image_t cover_texture;

	uint32_t id;
	uint32_t type;
	uint32_t year;
	float average_rating;
	uint32_t best_rating, worst_rating, votes_num, rank;
	uint32_t episodes_num;
	uint32_t seasons_num;
	std::vector<season_info_t> seasons;

	std::string studio_name, studio_uri;

	std::string description;
	std::vector<std::string> tags;

	Poco::Timestamp started;
	Poco::Timestamp ended;

	Poco::Timestamp last_updated;

	title_info_t() : id(0), type(0), year(0), average_rating(0.0f), best_rating(0), worst_rating(0), votes_num(0), rank(0), episodes_num(0) {}

	PUGI_SERIALIZATION_START
	{
		PUGI_SERIALIZE_ATTRIB(name);
		PUGI_SERIALIZE(uri);
		PUGI_SERIALIZE(cover_thumb_uri);
		PUGI_SERIALIZE(cover_uri);
		PUGI_SERIALIZE(id);
		PUGI_SERIALIZE(type);
		PUGI_SERIALIZE(year);
		PUGI_SERIALIZE(average_rating);
		PUGI_SERIALIZE(best_rating);
		PUGI_SERIALIZE(worst_rating);
		PUGI_SERIALIZE(votes_num);
		PUGI_SERIALIZE(rank);
		PUGI_SERIALIZE(episodes_num);
		PUGI_SERIALIZE(studio_name);
		PUGI_SERIALIZE(studio_uri);
		PUGI_SERIALIZE(description);
		//PUGI_SERIALIZE(tags);

		return true;
	}
	PUGI_SERIALIZATION_END
};

struct site_parser_info_t
{
	parser_entity_t titlelist_uri, titlelist_next_page_uri, titlelist_title_name, titlelist_title_uri, title_cover_thumb_uri, title_cover_uri;
	parser_entity_t title_name, title_id, title_type, title_year, title_average_rating;
	parser_entity_t title_best_rating, title_worst_rating, title_votes_num, title_rank, title_episodes_num;
	parser_entity_t title_studio_name, title_studio_uri;
	parser_entity_t title_status, title_episodes_watched_num, title_times_watched_num, title_rating;

	parser_entity_t titlelist_type, titlelist_year, titlelist_average_rating;
	parser_entity_t titlelist_status, titlelist_episodes_watched, titlelist_times_watched, titlelist_rating, titlelist_episodes_num;

	parser_entity_t titlesearch_titles;

	std::string useragent;

	parser_entity_t title_seasons_num, titleseasonlist_title_name, titleseasonlist_title_uri;

	//site_parser_info_t() {}
};
//-----------------------------------------------------------------------------------
struct user_title_info_t;
struct site_user_info_t;
//-----------------------------------------------------------------------------------
struct site_info_t
{
	std::string name, url, login_uri, login_cookie;

	site_parser_info_t parser_info;

	std::vector<std::string> title_types;
	std::vector<int> title_type_ids;

	std::vector<const char *> title_statuses;
	std::vector<int> title_status_ids;

	std::vector<title_info_t> titles;

	float rating_mulcoef;

	float cover_image_scale_x, cover_image_scale_y;

	ImVec4 color, text_color;

	http_session_t *http;

	site_info_t();
	virtual ~site_info_t();

	virtual bool parse_title_info_by_id(site_user_info_t &site_user, title_info_t &title);


	virtual bool authenticate(site_user_info_t &site_user) = 0;

	virtual bool sync(const std::string &username, const std::string &password, site_user_info_t &site_user) = 0;

	virtual bool send_request_change_title_episodes_watched_num(site_user_info_t &site_user, const title_info_t &title, uint32_t episodes_watched_num) = 0;

	virtual bool send_request_change_title_status(site_user_info_t &site_user, const title_info_t &title, uint32_t status) = 0;

	virtual bool send_request_change_title_rating(site_user_info_t &site_user, const title_info_t &title, float rating) = 0;

	virtual bool send_request_add_title(site_user_info_t &site_user, const title_info_t &title, uint32_t status = NU_TITLE_STATUS_PLAN_TO_WATCH) = 0;

	virtual bool send_request_delete_title(site_user_info_t &site_user, const title_info_t &title) = 0;

	virtual bool send_request_search_title(site_user_info_t &site_user, const std::string &title_name, std::vector<title_info_t> &found_titles) = 0;

	virtual bool parse_title_info(pugi::xml_node &node, site_user_info_t &site_user, title_info_t &title) = 0;

	bool send_request_change_title_episodes_watched_num(site_user_info_t &site_user, uint32_t i, uint32_t episodes_watched_num);

	bool send_request_change_title_status(site_user_info_t &site_user, uint32_t i, uint32_t status);

	bool send_request_change_title_rating(site_user_info_t &site_user, uint32_t i, float rating);

	bool send_request_add_title(site_user_info_t &site_user, uint32_t i, uint32_t status = NU_TITLE_STATUS_PLAN_TO_WATCH);

	bool send_request_delete_title(site_user_info_t &site_user, uint32_t i);

	void remove_titles_removed_from_other_clients(site_user_info_t &site_user, std::vector<title_info_t> &site_titles);

	uint32_t remove_title(site_user_info_t &site_user, uint32_t i);

	bool parse_title_info(pugi::xml_document &doc, site_user_info_t &site_user, const std::string &title_uri, title_info_t &title);

	//bool parse_title_info(pugi::xml_document &doc, const std::string &title_name, const std::string &title_uri, title_info_t &title);

	//bool parse_title_info(const std::string &title_name, const std::string &title_uri, title_info_t &title);

	bool parse_title_info(site_user_info_t &site_user, const std::string &title_uri, title_info_t &title);

	bool parse_title_and_user_title_info(pugi::xml_document &doc, site_user_info_t &site_user, const std::string &title_uri, title_info_t &title, user_title_info_t &user_title);

	bool parse_title_and_user_title_info(site_user_info_t &site_user, const std::string &title_uri, title_info_t &title, user_title_info_t &user_title);

	bool parse_user_title_info(pugi::xml_node &node, user_title_info_t &user_title);

	bool parse_user_title_info(site_user_info_t &site_user, const title_info_t &title, user_title_info_t &user_title);

	bool parse_user_title_info(site_user_info_t &site_user, user_title_info_t &user_title);

	PUGI_SERIALIZATION_START
	{
		PUGI_SERIALIZE_ATTRIB(name);

		//if(is_write)
		//{
		//	for(std::vector<title_info_t>::iterator it = titles.begin(); it != titles.end(); ++it)
		//		if(!it->write(node.append_child("title")))
		//			return false;
		//}
		//else
		//{
		//	title_info_t title;

		//	node = Node.first
		//	if(title.read(node))
		//}
		PUGI_SERIALIZE_ARRAY_NR(titles, "title");

		//PUGI_SERIALIZE_ARRAY(user_titles, "user_title");

		PUGI_SERIALIZE(rating_mulcoef);

		return true;
	}
	PUGI_SERIALIZATION_END
};
//-----------------------------------------------------------------------------------
#endif
