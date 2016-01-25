//
// NowUpdater.h
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#ifndef NOWUPDATER_H
#define NOWUPDATER_H
//-----------------------------------------------------------------------------------
#include "config.h"
//-----------------------------------------------------------------------------------
#include "nu_app.h"
//-----------------------------------------------------------------------------------
#include "Poco\Thread.h"
#include "Poco\RunnableAdapter.h"
//-----------------------------------------------------------------------------------
struct NowUpdater : public nu_app
{
	std::vector<std::string> users;

	uint32_t current_user;

	user_info_t *userinfo;

	HWND choose_user_window, new_user_window, options_window, main_window;

	Poco::Thread download_images_thread;
	Poco::RunnableAdapter<NowUpdater> download_images_thread_adapter;

	NowUpdater();

	bool init();

	void destroy();

	bool choose_user_ui(nu_window *window);

	bool new_user_ui(nu_window *window);

	bool create_user(std::string username, std::string password);

	void load_images();
	void download_images();

	bool main_ui(nu_window *window);
};
//-----------------------------------------------------------------------------------
#endif
