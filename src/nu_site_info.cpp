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
//-----------------------------------------------------------------------------------
site_info_t::site_info_t()
{
	color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	text_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
}
site_info_t::~site_info_t()
{
	delete http;
}

bool site_info_t::parse_title_info_by_id(site_user_info_t & site_user, title_info_t & title)
{
	throw std::exception("The method or operation is not implemented.");
}

bool site_info_t::parse_title_info(pugi::xml_document &doc, site_user_info_t &site_user, const std::string &title_uri, title_info_t &title)
{
	title.uri = title_uri;

	if(title.uri.empty())
	{
		if(!parse_title_info_by_id(site_user, title))
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

//#define NU_PARSE_F(a, x, f, e)	{ bool found = parse_f(doc, parser_info.title_##x, a.x, f); if(e && !found) NU_ERROR(#a "." #x " not found at " + parser_info.title_##x.xpath); }
#define NU_PARSE(a, x, e)       { bool found = parse(node, parser_info.title_##x, a.x);      if(e && !found) NU_ERROR(#a "." #x " not found:" + (parser_info.title_##x.xpath.empty() ? parser_info.title_##x.xpath : " at \"" + parser_info.title_##x.xpath + "\"") + (parser_info.title_##x.regexp.empty() ? parser_info.title_##x.regexp : " no match for \"" + parser_info.title_##x.regexp + "\"")); }

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

void site_info_t::download_title_images(uint32_t i)
{
	if(!titles[i].cover_thumb_uri.empty())
	{
		if(titles[i].cover_texture_data.empty())
		{
			titles[i].cover_texture_data = http->go_to(titles[i].cover_thumb_uri, login_cookie);
		}
	}
}

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

void site_info_t::remove_titles_removed_from_other_clients(site_user_info_t &site_user, std::vector<title_info_t> &site_titles)
{
	// Remove titles removed not from NowUpdater (from web or other client):
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

