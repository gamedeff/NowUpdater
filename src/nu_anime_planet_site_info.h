//
// NowUpdater
//
// nu_anime_planet_site_info.h
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#ifndef NU_ANIME_PLANET_SITE_INFO_T_H
#define NU_ANIME_PLANET_SITE_INFO_T_H
//-----------------------------------------------------------------------------------
#include "nu_site_info.h"
//-----------------------------------------------------------------------------------
struct anime_planet_site_info_t : site_info_t
{
	anime_planet_site_info_t();

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
