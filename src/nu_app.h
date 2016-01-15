//
// NowUpdater
//
// nu_app.h
//
// Copyright (c) 2016, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#ifndef NU_APP_H
#define NU_APP_H
//-----------------------------------------------------------------------------------
#include "nu_options.h"
#include "nu_user_info.h"

#include "render.h"
//-----------------------------------------------------------------------------------
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//-----------------------------------------------------------------------------------
struct nu_window
{
	WNDCLASSEX wc;

	string_t title, classname;
	uint32_t x, y, w, h;

	RenderView *render_view;

	nu_window() : render_view(0) {}
};
//-----------------------------------------------------------------------------------
struct nu_app
{
	options_t options;

	user_info_t userinfo;

	std::map<HWND, nu_window> windows;

	typedef std::pair<const HWND, nu_window> WindowByHandle;

	Render *renderer;

	nu_app(const std::string &username, const std::string &password);

	bool init();

	void destroy();

	WNDCLASSEX register_window_class(const char_t *wndclass);

	HWND create_window(const string_t &window_title, const string_t &window_class, uint32_t x, uint32_t y, uint32_t w, uint32_t h);

	HWND create_window(const string_t &window_title, uint32_t x, uint32_t y, uint32_t w, uint32_t h);

	void destroy_window(HWND hWnd);

	void handle_messages(uint32_t popup_w, uint32_t popup_h);

	void handle_popup(HWND hWnd, uint32_t popup_w, int current_title_index, ImVec2 &pos);
};
//-----------------------------------------------------------------------------------
extern nu_app *app;
//-----------------------------------------------------------------------------------
#endif

