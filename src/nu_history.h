//
// NowUpdater
//
// nu_history.h
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#ifndef NU_HISTORY_H
#define NU_HISTORY_H
//-----------------------------------------------------------------------------------
#include "nu_types.h"
//-----------------------------------------------------------------------------------
enum nu_user_action_type
{
	NU_ACT_START_PLAYING_TITLE,
	NU_ACT_END_PLAYING_TITLE,
	NU_ACT_PAUSE_PLAYING_TITLE,
	NU_ACT_RESUME_PLAYING_TITLE,
	NU_ACT_START_PLAYING_TITLE_EPISODE,
	NU_ACT_END_PLAYING_TITLE_EPISODE,
	NU_ACT_PAUSE_PLAYING_TITLE_EPISODE,
	NU_ACT_RESUME_PLAYING_TITLE_EPISODE,
	NU_ACT_ADD_TITLE,
	NU_ACT_REMOVE_TITLE,
	NU_ACT_RATE_TITLE,
	NU_ACT_ADD_TITLE_REVIEW,
	NU_ACT_INC_TITLE_EPISODE,
	NU_ACT_DEC_TITLE_EPISODE,
	NU_ACT_SEARCH_TITLE,
	NU_ACT_SYNC_SITE,
	NU_ACT_SYNC_ALL,
};
//-----------------------------------------------------------------------------------
static const char_t *NU_ACT_ENG_STR[] =
{
	GW_CODE2TXT(NU_ACT_START_PLAYING_TITLE),
	GW_CODE2TXT(NU_ACT_END_PLAYING_TITLE),
	GW_CODE2TXT(NU_ACT_PAUSE_PLAYING_TITLE),
	GW_CODE2TXT(NU_ACT_RESUME_PLAYING_TITLE),
	GW_CODE2TXT(NU_ACT_START_PLAYING_TITLE_EPISODE),
	GW_CODE2TXT(NU_ACT_END_PLAYING_TITLE_EPISODE),
	GW_CODE2TXT(NU_ACT_PAUSE_PLAYING_TITLE_EPISODE),
	GW_CODE2TXT(NU_ACT_RESUME_PLAYING_TITLE_EPISODE),
	_T("add title"),
	GW_CODE2TXT(NU_ACT_REMOVE_TITLE),
	GW_CODE2TXT(NU_ACT_RATE_TITLE),
	GW_CODE2TXT(NU_ACT_ADD_TITLE_REVIEW),
	GW_CODE2TXT(NU_ACT_INC_TITLE_EPISODE),
	GW_CODE2TXT(NU_ACT_DEC_TITLE_EPISODE),
	GW_CODE2TXT(NU_ACT_SEARCH_TITLE),
	GW_CODE2TXT(NU_ACT_SYNC_SITE),
	_T("sync all"),
};
//-----------------------------------------------------------------------------------
struct nu_user_action
{
	Poco::Timestamp timestamp;

	/*nu_user_action_type*/int type;

	const char_t *to_str()
	{
		return NU_ACT_ENG_STR[type];
	}

	PUGI_SERIALIZATION_START
	{
		PUGI_SERIALIZE_ATTRIB(/**(int *)&*/type);
		PUGI_SERIALIZE_ATTRIB(timestamp);

		return true;
	}
	PUGI_SERIALIZATION_END
};
//-----------------------------------------------------------------------------------
struct nu_user_action_history
{
	std::vector<nu_user_action> user_actions;

	void add(nu_user_action_type user_action_type)
	{
		nu_user_action user_action = { Poco::Timestamp(), user_action_type };

		user_actions.push_back(user_action);
	}

	PUGI_SERIALIZATION_START
	{
		PUGI_SERIALIZE_ARRAY(user_actions, "user_action");

		return true;
	}
	PUGI_SERIALIZATION_END
};
//-----------------------------------------------------------------------------------
#endif

