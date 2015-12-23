//
// NowUpdater
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#ifndef NU_MEDIAPLAYER_H
#define NU_MEDIAPLAYER_H
//-----------------------------------------------------------------------------------
#include "config.h"
#include "nu_types.h"
//-----------------------------------------------------------------------------------
typedef int (*get_title_filename_proc)(const char_t *mediaplayer, char_t *filename, int filename_size);
typedef int (*get_title_filename_cleanup_proc)(const char_t *mediaplayer);

struct title_filename_info_t
{
	string_t title;

	int episode_number;
	int year;

	title_filename_info_t() : episode_number(-1), year(-1) {}
};

typedef int (*get_title_filename_info_proc)(const char_t *filename, title_filename_info_t *title_filename_info);

int get_title_filename_from_windowtitle(const char_t *mediaplayer, char_t *filename, int filename_size);
int get_title_filename_from_windowtitle_cleanup(const char_t *mediaplayer);
int get_title_filename_from_cmdline(const char_t *mediaplayer, char_t *filename, int filename_size);
int get_title_filename_from_cmdline_cleanup(const char_t *mediaplayer);
int get_title_filename_from_WM_COPYDATA(const char_t *mediaplayer, char_t *filename, int filename_size);
int get_title_filename_from_WM_COPYDATA_cleanup(const char_t *mediaplayer);

int get_title_filename_info_anitomy(const char_t *filename, title_filename_info_t *title_filename_info);
int get_title_filename_info_guessit_local(const char_t *filename, title_filename_info_t *title_filename_info);
int get_title_filename_info_guessit_online(const char_t *filename, title_filename_info_t *title_filename_info);

struct nu_mediaplayer
{
	string_t name;

	std::vector<get_title_filename_proc> get_title_filename_p;
	std::vector<get_title_filename_cleanup_proc> get_title_filename_cleanup_p;
	get_title_filename_info_proc get_title_filename_info_p;

	nu_mediaplayer(const char_t *mediaplayer) : name(mediaplayer)
	{
		get_title_filename_p.push_back(get_title_filename_from_cmdline);
		get_title_filename_p.push_back(get_title_filename_from_WM_COPYDATA);
		get_title_filename_p.push_back(get_title_filename_from_windowtitle);

		get_title_filename_cleanup_p.push_back(get_title_filename_from_windowtitle_cleanup);
		get_title_filename_cleanup_p.push_back(get_title_filename_from_WM_COPYDATA_cleanup);
		get_title_filename_cleanup_p.push_back(get_title_filename_from_cmdline_cleanup);

		get_title_filename_info_p = get_title_filename_info_anitomy;
	}

	~nu_mediaplayer()
	{
		for(std::vector<get_title_filename_cleanup_proc>::const_iterator it = get_title_filename_cleanup_p.begin(); it != get_title_filename_cleanup_p.end(); ++it)
			(*it)(name.c_str());
	}

	string_t get_title_filename() const
	{
		char_t title_filename_buffer[MAX_PATH];
		for(std::vector<get_title_filename_proc>::const_iterator it = get_title_filename_p.begin(); it != get_title_filename_p.end(); ++it)
			if((*it)(name.c_str(), title_filename_buffer, countof(title_filename_buffer)) != 0)
			{
				string_t title_filename_str(title_filename_buffer);
				return title_filename_str;
			}

		return _T("");
	}

	title_filename_info_t get_title_filename_info() const
	{
		char_t title_filename_buffer[MAX_PATH];
		for(std::vector<get_title_filename_proc>::const_iterator it = get_title_filename_p.begin(); it != get_title_filename_p.end(); ++it)
			if((*it)(name.c_str(), title_filename_buffer, countof(title_filename_buffer)) != 0)
			{
				title_filename_info_t title_filename_info;
				if(get_title_filename_info_p(title_filename_buffer, &title_filename_info) != 0)
					return title_filename_info;
			}

		return title_filename_info_t();
	}

	title_filename_info_t get_title_filename_info(const string_t &title_filename) const
	{
		title_filename_info_t title_filename_info;
		if(get_title_filename_info_p(title_filename.c_str(), &title_filename_info) != 0)
			return title_filename_info;

		return title_filename_info_t();
	}
};
//-----------------------------------------------------------------------------------
#endif
