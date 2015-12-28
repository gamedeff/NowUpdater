//
// NowUpdater
//
// nu_imdb_site_info.h
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#ifndef NU_IMDB_SITE_INFO_H
#define NU_IMDB_SITE_INFO_H
//-----------------------------------------------------------------------------------
#include "nu_site_info.h"
//-----------------------------------------------------------------------------------
struct imdb_list_t
{
	std::string name;
	std::string list_id, list_class;

	uint32_t status;
};
//-----------------------------------------------------------------------------------
struct imdb_site_info_t : site_info_t
{
	std::vector<imdb_list_t> imdb_lists;

	imdb_site_info_t();

	std::string imdb_get_title_id_str(const title_info_t &title);

	std::string imdb_get_auth_token(const std::string &imdb_title_id);

	bool send_request_get_title_rating(site_user_info_t &site_user, const title_info_t &title, float &rating);

	bool send_request_change_title_rating_imdb_ajax(site_user_info_t &site_user, const title_info_t &title, float rating);

	bool send_request_get_list_item_id_imdb_list_ajax(site_user_info_t &site_user, const title_info_t &title, imdb_list_t &imdb_list, std::string &imdb_list_item_id, std::string &imdb_hidden_key_name, std::string &imdb_hidden_key);
	bool send_request_delete_title_imdb_list_ajax(site_user_info_t &site_user, const title_info_t &title, imdb_list_t &imdb_list);

	bool authenticate_imdb_with_tesseract(site_user_info_t &site_user);

	virtual bool authenticate(site_user_info_t &site_user);

	virtual bool sync(const std::string &username, const std::string &password, site_user_info_t &site_user);

	virtual bool send_request_change_title_episodes_watched_num(site_user_info_t &site_user, const title_info_t &title, uint32_t episodes_watched_num);

	virtual bool send_request_change_title_status(site_user_info_t &site_user, const title_info_t &title, uint32_t status);

	virtual bool send_request_change_title_rating(site_user_info_t &site_user, const title_info_t &title, float rating);

	virtual bool send_request_add_title(site_user_info_t &site_user, const title_info_t &title, uint32_t status = NU_TITLE_STATUS_PLAN_TO_WATCH);

	virtual bool send_request_delete_title(site_user_info_t &site_user, const title_info_t &title);

	virtual bool send_request_search_title(site_user_info_t &site_user, const std::string &title_name, std::vector<title_info_t> &found_titles);

	virtual bool parse_title_info(pugi::xml_node &node, site_user_info_t &site_user, title_info_t &title);
};
//-----------------------------------------------------------------------------------
#endif

