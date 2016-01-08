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
#include "util.h"
//-----------------------------------------------------------------------------------
#define POCO_NO_UNWINDOWS
//-----------------------------------------------------------------------------------
#include "Poco\DateTime.h"
#include "Poco\DateTimeParser.h"
#include "Poco\DateTimeFormat.h"
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

HWND get_mediaplayer_handle(const char_t *mediaplayer);
string_t get_mediaplayer_current_time_str(const char_t *mediaplayer);
string_t get_mediaplayer_total_time_str(const char_t *mediaplayer);
bool time_str_to_seconds(const string_t &time_str, uint32_t &seconds);

enum nu_mediaplayer_state
{
	NU_MEDIAPLAYER_STATE_NOT_AVAILABLE,
	NU_MEDIAPLAYER_STATE_OPENING,
	NU_MEDIAPLAYER_STATE_BUFFERING,
	NU_MEDIAPLAYER_STATE_PLAYING,
	NU_MEDIAPLAYER_STATE_PAUSED,
	NU_MEDIAPLAYER_STATE_STOPED,
	NU_MEDIAPLAYER_STATE_ERROR
};

static const char_t *MEDIAPLAYER_STATES_STR[] =
{
	_T("N/A"),
	_T("Opening"),
	_T("Buffering"),
	_T("Playing"),
	_T("Paused"),
	_T("Stopped"),
	_T("Error")
};

string_t get_mediaplayer_state_str(const char_t *mediaplayer);
nu_mediaplayer_state get_mediaplayer_state(const char_t *mediaplayer);
bool is_mediaplayer_paused(const char_t *mediaplayer);

struct nu_mediaplayer
{
	string_t name;

	nu_mediaplayer_state state;

	std::vector<get_title_filename_proc> get_title_filename_p;
	std::vector<get_title_filename_cleanup_proc> get_title_filename_cleanup_p;
	get_title_filename_info_proc get_title_filename_info_p;

	nu_mediaplayer(const char_t *mediaplayer) : name(mediaplayer), state(NU_MEDIAPLAYER_STATE_NOT_AVAILABLE)
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

	HWND get_handle() const
	{
		return get_mediaplayer_handle(name.c_str());
	}

	void update_state()
	{
		state = get_mediaplayer_state(name.c_str());
	}

	bool get_title_filename(string_t &title_filename_str) const
	{
		char_t title_filename_buffer[MAX_PATH];
		for(std::vector<get_title_filename_proc>::const_iterator it = get_title_filename_p.begin(); it != get_title_filename_p.end(); ++it)
			if((*it)(name.c_str(), title_filename_buffer, countof(title_filename_buffer)) != 0)
			{
				title_filename_str = title_filename_buffer;
				return true;
			}

		return false;
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
