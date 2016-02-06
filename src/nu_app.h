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
enum nu_animation_kind
{
	NU_ANIMATION_PAUSE,
	NU_ANIMATION_SLIDE,
};
//-----------------------------------------------------------------------------------
enum nu_animation_direction
{
	NU_ANIMATION_HOR_POSITIVE, // Animates the window from left to right
	NU_ANIMATION_HOR_NEGATIVE, // Animates the window from right to left
	NU_ANIMATION_VER_POSITIVE, // Animates the window from top to bottom
	NU_ANIMATION_VER_NEGATIVE  // Animates the window from bottom to top
};
//-----------------------------------------------------------------------------------
struct nu_animation_desc
{
	nu_animation_kind kind;
	nu_animation_direction direction;
	uint32_t distance_h, distance_v;

	nu_animation_desc() {}
	nu_animation_desc(nu_animation_kind kind, nu_animation_direction direction, uint32_t distance_h, uint32_t distance_v) : kind(kind), direction(direction), distance_h(distance_h), distance_v(distance_v) {}
};
//-----------------------------------------------------------------------------------
struct nu_animation
{
	bool active;
	uint32_t id;
	uint32_t time, fps, current_frame, frames_num;
	nu_animation_kind kind;
	nu_animation_direction direction;
	uint32_t distance_h, distance_v;

	float x, y;

	nu_animation() : active(false), id(0), time(1000), fps(30), current_frame(0), frames_num(0), distance_h(0), distance_v(0), x(0), y(0) {}
};
//-----------------------------------------------------------------------------------
enum nu_window_popup_kind
{
	NU_POPUP_BOTTOM_RIGHT,
	NU_POPUP_BOTTOM_LEFT,
	NU_POPUP_TOP_LEFT,
	NU_POPUP_TOP_RIGHT
};
//-----------------------------------------------------------------------------------
struct nu_window
{
	WNDCLASSEX wc;

	string_t title, classname;
	int x, y;
	uint32_t w, h;

	RenderView *render_view;

	std::vector<nu_animation> animations;
	uint32_t current_animation;

	Closure<bool(nu_window *)> on_update;
	Closure<bool(nu_window *)> on_gui;

	nu_window() : render_view(0), current_animation(0) {}
	//nu_window(Closure<bool(nu_window *)> on_idle) : render_view(0), on_idle(on_idle) {}
};
//-----------------------------------------------------------------------------------
struct nu_app
{
	string_t title;

	options_t options;

	std::map<HWND, nu_window> windows;

	typedef std::pair<const HWND, nu_window> WindowByHandle;

	Render *renderer;

	ImVec2 pos;

	Poco::FastMutex mutex;

	nu_app(const string_t &title);
	virtual ~nu_app();

	string_t get_process_name();

	virtual bool init();

	virtual void destroy();

	WNDCLASSEX register_window_class(const char_t *wndclass);

	HWND create_window(const string_t &window_title, const string_t &window_class, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const Closure<bool(nu_window *)> &on_gui);

	HWND create_window(const string_t &window_title, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const Closure<bool(nu_window *)> &on_gui);

	HWND create_and_show_window(const string_t &window_title, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const Closure<bool(nu_window *)> &on_gui);

	HWND create_and_show_window_center(const string_t &window_title, uint32_t w, uint32_t h, const Closure<bool(nu_window *)> &on_gui);

	HWND create_and_show_window_popup(const string_t &window_title, uint32_t w, uint32_t h, const Closure<bool(nu_window *)> &on_gui, uint32_t time = 1000, uint32_t slide_time = 1000, nu_window_popup_kind popup_kind = NU_POPUP_BOTTOM_RIGHT);

	void destroy_window(HWND hWnd);

	void destroy_window(nu_window *window);

	void handle_messages();

	void get_desktop_size(uint32_t &desktop_width, uint32_t &desktop_height);

	HWND get_window_handle(nu_window *window);

	void add_animation(HWND hWnd, nu_animation_kind kind, nu_animation_direction direction, uint32_t distance_h, uint32_t distance_v, uint32_t time = 1000);

	void add_animation(HWND hWnd, const nu_animation_desc &animation_desc, uint32_t time = 1000);

	void add_animation_pause(HWND hWnd, uint32_t time = 1000);

	void add_animation_slide_up(HWND hWnd, uint32_t dy, uint32_t time = 1000);

	void add_animation_slide_down(HWND hWnd, uint32_t dy, uint32_t time = 1000);

	void start_animation(HWND hWnd, nu_animation &animation);

	void on_timer(HWND hWnd, UINT_PTR nIDEvent);
};
//-----------------------------------------------------------------------------------
extern nu_app *app;
//-----------------------------------------------------------------------------------
#endif

