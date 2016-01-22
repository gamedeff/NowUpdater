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

#include "closure.h"
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

	Closure<bool(nu_window *)> on_idle;

	nu_window() {}
	nu_window(Closure<bool(nu_window *)> on_idle) : render_view(0), on_idle(on_idle) {}
};
//-----------------------------------------------------------------------------------
struct nu_app
{
	string_t title;

	options_t options;

	std::map<HWND, nu_window> windows;

	typedef std::pair<const HWND, nu_window> WindowByHandle;

	Render *renderer;

	Poco::FastMutex mutex;

	nu_app(const string_t &title);
	virtual ~nu_app();

	string_t get_process_name();

	virtual bool init();

	virtual void destroy();

	WNDCLASSEX register_window_class(const char_t *wndclass);

	HWND create_window(const string_t &window_title, const string_t &window_class, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const Closure<bool(nu_window *)> &on_idle);

	HWND create_window(const string_t &window_title, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const Closure<bool(nu_window *)> &on_idle);

	HWND create_and_show_window(const string_t &window_title, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const Closure<bool(nu_window *)> &on_idle);

	HWND create_and_show_window_center(const string_t &window_title, uint32_t w, uint32_t h, const Closure<bool(nu_window *)> &on_idle);

	void destroy_window(HWND hWnd);

	void destroy_window(nu_window *window);

	void handle_messages(uint32_t popup_w, uint32_t popup_h);

	void handle_popup(HWND hWnd, uint32_t popup_w, int current_title_index, ImVec2 &pos);

	void get_desktop_size(uint32_t &desktop_width, uint32_t &desktop_height);
};
//-----------------------------------------------------------------------------------
extern nu_app *app;
//-----------------------------------------------------------------------------------
#endif

