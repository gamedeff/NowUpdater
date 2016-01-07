//
// NowUpdater
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#ifndef NU_USER_INFO_H
#define NU_USER_INFO_H
//-----------------------------------------------------------------------------------
#include "config.h"
#include "util.h"
#include "nu_types.h"
#include "nu_options.h"

#include "nu_mediaplayer.h"

#include "nu_xml.h"

#include "Poco/Timer.h"
#include "Poco/Thread.h"

#include "Poco/Timestamp.h"

#include "nu_site_info.h"

#include "nu_history.h"

struct user_title_info_t
{
	uint32_t index;

	uint32_t status;
	uint32_t episodes_watched_num;
	uint32_t times_watched_num;
	float rating;

	Poco::Timestamp started;
	Poco::Timestamp ended;

	Poco::Timestamp last_updated;

	//user_title_info_t() : last_updated(Poco::Timestamp::TIMEVAL_MIN) {}

	PUGI_SERIALIZATION_START
	{
		//PUGI_SERIALIZE_ATTRIB(site_index);
		PUGI_SERIALIZE_ATTRIB(index);
		PUGI_SERIALIZE(status);
		PUGI_SERIALIZE(episodes_watched_num);
		PUGI_SERIALIZE(times_watched_num);
		PUGI_SERIALIZE(rating);

		last_updated = Poco::Timestamp::TIMEVAL_MIN;

		return true;
	}
	PUGI_SERIALIZATION_END
};

struct site_user_info_t
{
	uint32_t site_index;

	bool enabled;

	std::string username, password;
	std::map<std::string, std::string> login_cookies;

	uint32_t id;

	std::vector<user_title_info_t> user_titles;

	PUGI_SERIALIZATION_START
	{
		PUGI_SERIALIZE_ATTRIB(site_index);

		PUGI_SERIALIZE_ATTRIB(enabled);
		//enabled = true;

		PUGI_SERIALIZE(username);
		PUGI_SERIALIZE(password);
		PUGI_SERIALIZE_NR(login_cookies);

		PUGI_SERIALIZE_NR(id);

		//for(std::map<std::string, std::string>::iterator login_cookies_it = login_cookies.begin(); login_cookies_it != login_cookies.end(); ++login_cookies_it)
		//{
		//	PUGI_SERIALIZE(login_cookies_it->first);
		//	PUGI_SERIALIZE(login_cookies_it->second);
		//}

		PUGI_SERIALIZE_ARRAY_NR(user_titles, "title");

		return true;
	}
	PUGI_SERIALIZATION_END
};

struct user_info_t
{
	std::string username, password;

	options_t *options;

	//std::vector<user_title_info_t> user_titles;
	std::vector<site_user_info_t> site_users;

	std::vector<site_info_t *> sites;
	uint32_t current_site;

	std::vector<nu_mediaplayer> mediaplayers;

	std::vector<title_info_t> last_found_titles;

	title_filename_info_t last_title;

	bool show_title_popup;

	uint32_t current_title_index;

	nu_user_action_history action_history;

	Poco::FastMutex mutex;

	user_info_t(const std::string &username, const std::string &password, options_t *options);

	~user_info_t();

	void on_timer(Poco::Timer& timer);

	bool load();
	bool save();

	bool init();

	bool authenticate();

	bool sync(uint32_t i, uint32_t k);

	bool set_title_episodes_watched_num(uint32_t si, uint32_t i, uint32_t episodes_watched_num);

	bool set_title_episodes_watched_num(uint32_t i, uint32_t episodes_watched_num);

	bool set_title_rating(uint32_t si, uint32_t i, float rating);

	bool set_title_rating(uint32_t i, float rating);

	bool add_title(uint32_t si, title_info_t &title, uint32_t status = NU_TITLE_STATUS_PLAN_TO_WATCH);

	bool add_title(title_info_t &title, uint32_t status = NU_TITLE_STATUS_PLAN_TO_WATCH);

	bool search_title(uint32_t si, const std::string &title_name, std::vector<title_info_t> &found_titles);

	bool search_title(const std::string &title_name, std::vector<title_info_t> &found_titles);

	float get_cover_width(uint32_t i);
	float get_cover_height(uint32_t i);

	uint32_t get_user_title_index(uint32_t si, const std::string &title_name);
	uint32_t get_user_title_index_parse(uint32_t si, const std::string &title_name);
	uint32_t find_and_add_title(uint32_t si, const std::string &title_name);

	int main();

	void title_ui(int &current_title_status);

	int titlelist_ui();

	void set_current_site_next_to(uint32_t si);
	void search_ui();

	bool has_title(uint32_t si, uint32_t site_user_index, uint32_t user_title_index);

	void sync_all();

	void add_to_history(nu_user_action_type user_action_type, const char_t *desc = _T(""));

	PUGI_SERIALIZATION_START
	{
		PUGI_SERIALIZE_ATTRIB(username);

		//PUGI_SERIALIZE_ARRAY(user_titles, "title");

		PUGI_SERIALIZE_ARRAY(site_users, "site");

		//PUGI_SERIALIZE(action_history);

		if(is_write)
			action_history.write(node);
		else
			action_history.read(node);

		return true;
	}
	PUGI_SERIALIZATION_END
};
//-----------------------------------------------------------------------------------
#endif
