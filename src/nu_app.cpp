//
// NowUpdater
//
// nu_app.cpp
//
// Copyright (c) 2016, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#include "nu_app.h"
//-----------------------------------------------------------------------------------
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
//-----------------------------------------------------------------------------------
using Poco::Timer;
using Poco::TimerCallback;

#include "for_each.h"
#include "util.h"

#include "imgui.h"

#include "render_d3d9.h"
//-----------------------------------------------------------------------------------
nu_app *app = 0;
//-----------------------------------------------------------------------------------
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_TIMER:

			app->on_timer(hWnd, wParam);
			break;
	}

	if(!app->renderer)
		return DefWindowProc(hWnd, msg, wParam, lParam);

	return app->renderer->WndProc(hWnd, msg, wParam, lParam);
}
//-----------------------------------------------------------------------------------
nu_app::nu_app(const string_t &title) : title(title), renderer(0), pos(0, 0)
{
	options.app_name = GW_T2A(get_process_name());

#ifdef GW_DEBUG
	options.app_name = options.app_name.substr(0, options.app_name.size() - 1); // remove "d" ending in debug builds
#endif
}

nu_app::~nu_app()
{
}

void nu_app::add_animation(HWND hWnd, nu_animation_kind kind, nu_animation_direction direction, uint32_t distance_h, uint32_t distance_v, uint32_t time)
{
	nu_animation animation;
	animation.id = rand();
	animation.time = time;
	animation.kind = kind;
	animation.direction = direction;
	animation.distance_h = distance_h;
	animation.distance_v = distance_v;

	//windows[hWnd].animations.push_back(animation);
	windows[hWnd].animations.insert(windows[hWnd].animations.begin(), animation);
	windows[hWnd].current_animation = windows[hWnd].animations.size() - 1;
}

void nu_app::add_animation(HWND hWnd, const nu_animation_desc &animation_desc, uint32_t time)
{
	add_animation(hWnd, animation_desc.kind, animation_desc.direction, animation_desc.distance_h, animation_desc.distance_v, time);
}

void nu_app::add_animation_pause(HWND hWnd, uint32_t time)
{
	add_animation(hWnd, NU_ANIMATION_PAUSE, NU_ANIMATION_HOR_POSITIVE, 0, 0, time);
}

void nu_app::add_animation_slide_up(HWND hWnd, uint32_t dy, uint32_t time)
{
	add_animation(hWnd, NU_ANIMATION_SLIDE, NU_ANIMATION_VER_NEGATIVE, 0, dy, time);
}

void nu_app::add_animation_slide_down(HWND hWnd, uint32_t dy, uint32_t time)
{
	add_animation(hWnd, NU_ANIMATION_SLIDE, NU_ANIMATION_VER_POSITIVE, 0, dy, time);
}

void nu_app::start_animation(HWND hWnd, nu_animation &animation)
{
	SetWindowPos(hWnd, HWND_TOP, windows[hWnd].x, windows[hWnd].y, windows[hWnd].w, windows[hWnd].h, 0);

	animation.x = windows[hWnd].x;
	animation.y = windows[hWnd].y;

	animation.current_frame = 0;
	animation.frames_num = animation.time / (1000 / animation.fps);

	animation.active = true;

	SetTimer(hWnd, animation.id, 1000 / animation.fps, NULL);
}

void nu_app::on_timer(HWND hWnd, UINT_PTR nIDEvent)
{
	if(windows[hWnd].animations.empty())
		return;

	nu_animation &animation = windows[hWnd].animations[windows[hWnd].current_animation];

	if(nIDEvent == animation.id)
	{
		if(++animation.current_frame > animation.frames_num)
		{
			animation.active = false;
			KillTimer(hWnd, animation.id);

			windows[hWnd].animations.pop_back();

			if(!windows[hWnd].animations.empty())
			{
				//gwOutputStringPrint(_FS(_T("current_animation = %d\n"), windows[hWnd].current_animation));

				--windows[hWnd].current_animation;
				start_animation(hWnd, windows[hWnd].animations[windows[hWnd].current_animation]);
			}
		}
		else if(animation.kind == NU_ANIMATION_SLIDE)
		{
			//RECT window_rect = { 0 };
			//GetWindowRect(hWnd, &window_rect);

			switch(animation.direction)
			{
				case NU_ANIMATION_HOR_POSITIVE: animation.x += ((float) animation.distance_h) / animation.frames_num; break;
				case NU_ANIMATION_HOR_NEGATIVE: animation.x -= ((float) animation.distance_h) / animation.frames_num; break;
				case NU_ANIMATION_VER_POSITIVE: animation.y += ((float) animation.distance_v) / animation.frames_num; break;
				case NU_ANIMATION_VER_NEGATIVE: animation.y -= ((float) animation.distance_v) / animation.frames_num; break;
			}

			windows[hWnd].x = floorf(animation.x + 0.5f);
			windows[hWnd].y = floorf(animation.y + 0.5f);

			//gwOutputStringPrint(_FS(_T("x = %d, y = %d\n"), windows[hWnd].x, windows[hWnd].y));

			SetWindowPos(hWnd, HWND_TOP, windows[hWnd].x, windows[hWnd].y, windows[hWnd].w, windows[hWnd].h, 0);

			ShowWindow(hWnd, SW_SHOWNORMAL);
		}   
	}
}

string_t nu_app::get_process_name()
{
	string_t buffer;
	buffer.resize(MAX_PATH);
	do
	{
		uint32_t len = GetModuleFileName(NULL, &buffer[0], buffer.size());
		if(len < buffer.size())
		{
			buffer.resize(len);
			break;
		}

		buffer.resize(buffer.size() * 2);
	}
	while(buffer.size() < USHRT_MAX + 1);
	// now buffer = "c:\whatever\yourexecutable.exe"

	// Go to the beginning of the file name
	char_t *process_name = PathFindFileName(buffer.c_str());
	// now process_name = "yourexecutable.exe"

	// Set the dot before the extension to 0 (terminate the string there)
	*(PathFindExtension(process_name)) = 0;
	// now process_name = "yourexecutable"

	return string_t(process_name);
}

bool nu_app::init()
{
	return true;
}

void nu_app::destroy()
{
	if(renderer)
	{
		renderer->Destroy();

		delete renderer;
		renderer = 0;
	}

	FOR_EACH(WindowByHandle &window_by_handle, windows)
	{
		HWND hWnd = window_by_handle.first;
		nu_window &window = window_by_handle.second;

		destroy_window(hWnd);
	}

	windows.clear();
}

WNDCLASSEX nu_app::register_window_class(const char_t *wndclass)
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL, wndclass, NULL };
	RegisterClassEx(&wc);
	return wc;
}

HWND nu_app::create_window(const string_t &window_title, const string_t &window_class, bool popup, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const Closure<bool(nu_window *)> &on_gui)
{
	nu_window window;

	window.title = window_title;
	window.classname = window_class;
	window.wc = register_window_class(window.classname.c_str());
	window.x = x;
	window.y = y;
	window.w = w;
	window.h = h;
	window.on_gui = on_gui;

	// Create application window
	HWND hWnd = CreateWindow(window.classname.c_str(), window.title.c_str(), popup ? WS_POPUP : WS_OVERLAPPEDWINDOW,
							 window.x, window.y, window.w, window.h, NULL, NULL, window.wc.hInstance, NULL);

	if(!hWnd)
	{
		UnregisterClass(window.classname.c_str(), window.wc.hInstance);
		return 0;
	}

	if(!renderer)
	{
		renderer = new RenderD3D9(options.no_native_windows);

		if(!renderer || !renderer->Init() || !renderer->InitDevice(hWnd))
		{
			destroy_window(hWnd);
			return 0;
		}
	}

	windows[hWnd] = window;

	return hWnd;
}

HWND nu_app::create_window(const string_t &window_title, const string_t &window_class, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const Closure<bool(nu_window *)> &on_gui)
{
	return create_window(window_title, window_class, options.no_native_windows, x, y, w, h, on_gui);
}

HWND nu_app::create_window(const string_t &window_title, bool popup, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const Closure<bool(nu_window *)> &on_gui)
{
	string_t window_class = string_t(GW_A2T(options.app_name)) +

#ifdef GW_UNICODE
		_T("WindowW") + 
#else
		_T("Window") + 
#endif
		string_t(GW_A2T(std::to_string(windows.size() + 1)));

	return create_window(window_title, window_class, popup, x, y, w, h, on_gui);
}

HWND nu_app::create_window(const string_t &window_title, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const Closure<bool(nu_window *)> &on_gui)
{
	return create_window(window_title, options.no_native_windows, x, y, w, h, on_gui);
}

HWND nu_app::create_and_show_window(const string_t &window_title, bool popup, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const Closure<bool(nu_window *)> &on_gui)
{
	HWND hWnd = create_window(window_title, popup, x, y, w, h, on_gui);
	if(!hWnd)
		return hWnd;

	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);

	if(options.no_native_windows)
	{
		RECT client_rect = { 0 };
		GetClientRect(hWnd, &client_rect);

		uint32_t ClientWidth  = client_rect.right - client_rect.left;
		uint32_t ClientHeight = client_rect.bottom - client_rect.top;

		if(!renderer->CreateRenderTarget(ClientWidth, ClientHeight, 4))
		{
			destroy_window(hWnd);
			return 0;
		}
	}

	windows[hWnd].render_view = renderer->CreateRenderView(hWnd, windows[hWnd].w, windows[hWnd].h);
	if(!windows[hWnd].render_view)
	{
		destroy_window(hWnd);
		return 0;
	}

	return hWnd;
}

HWND nu_app::create_and_show_window(const string_t &window_title, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const Closure<bool(nu_window *)> &on_gui)
{
	return create_and_show_window(window_title, options.no_native_windows, x, y, w, h, on_gui);
}

HWND nu_app::create_and_show_window_center(const string_t &window_title, uint32_t w, uint32_t h, const Closure<bool(nu_window *)> &on_gui)
{
	uint32_t desktop_width, desktop_height;
	get_desktop_size(desktop_width, desktop_height);

	return create_and_show_window(window_title.find(title) == 0 ? window_title : title + _T(": ") + window_title, desktop_width / 2 - w / 2, desktop_height / 2 - h / 2, w, h, on_gui);
}

HWND nu_app::create_and_show_window_popup(const string_t &window_title, uint32_t w, uint32_t h, const Closure<bool(nu_window *)> &on_gui, uint32_t time, uint32_t slide_time, nu_window_popup_kind popup_kind)
{
	RECT desktop_work_area_rect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &desktop_work_area_rect, 0);

	uint32_t desktop_work_area_width  = desktop_work_area_rect.right - desktop_work_area_rect.left;
	uint32_t desktop_work_area_height = desktop_work_area_rect.bottom - desktop_work_area_rect.top;

	uint32_t desktop_width  = GetSystemMetrics(SM_CXSCREEN);
	uint32_t desktop_height = GetSystemMetrics(SM_CYSCREEN);

	int x = 0, y = 0;
	uint32_t dy = 0;
	bool from_top = false;

	switch(popup_kind)
	{
		case NU_POPUP_BOTTOM_RIGHT:	x = desktop_work_area_width - w; y = desktop_height; dy = h + desktop_height - desktop_work_area_height; break;
		case NU_POPUP_BOTTOM_LEFT:	x = desktop_work_area_rect.left; y = desktop_height; dy = h + desktop_height - desktop_work_area_height; break;
		case NU_POPUP_TOP_LEFT:		x = desktop_work_area_rect.left; y = desktop_work_area_rect.top - h; dy = h; from_top = true; break;
		case NU_POPUP_TOP_RIGHT:	x = desktop_work_area_width - w; y = desktop_work_area_rect.top - h; dy = h; from_top = true; break;
	}

	HWND hWnd = create_and_show_window(window_title, true, x, y, w, h, on_gui);
	if(!hWnd)
		return 0;

	(this->*(from_top ? &nu_app::add_animation_slide_down : &nu_app::add_animation_slide_up))(hWnd, dy, slide_time);
	add_animation_pause(hWnd, time);
	(this->*(from_top ? &nu_app::add_animation_slide_up : &nu_app::add_animation_slide_down))(hWnd, dy, slide_time);

	start_animation(hWnd, windows[hWnd].animations[windows[hWnd].current_animation]);

	return 0;
}

void nu_app::destroy_window(HWND hWnd)
{
	if(renderer && renderer->render_views[hWnd])
	{
		renderer->DestroyRenderView(hWnd);

		if(renderer->render_views.empty())
		{
			renderer->Destroy();

			delete renderer;
			renderer = 0;
		}
	}

	DestroyWindow(hWnd);

	UnregisterClass(windows[hWnd].classname.c_str(), windows[hWnd].wc.hInstance);

	windows.erase(hWnd);

	FOR_EACH(WindowByHandle &window_by_handle, windows)
	{
		HWND window_hWnd = window_by_handle.first;
		if(window_hWnd != hWnd)
			renderer->DestroyRenderView(window_hWnd);
	}

	FOR_EACH(WindowByHandle &window_by_handle, windows)
	{
		HWND window_hWnd = window_by_handle.first;
		if(window_hWnd != hWnd)
			windows[window_hWnd].render_view = renderer->CreateRenderView(window_hWnd, windows[window_hWnd].w, windows[window_hWnd].h);
	}
}

HWND nu_app::get_window_handle(nu_window *window)
{
	for(std::map<HWND, nu_window>::iterator window_it = windows.begin(); window_it != windows.end(); ++window_it)
		if(window_it->second.classname == window->classname)
			return window_it->first;

	return 0;
}

void nu_app::destroy_window(nu_window *window)
{
	HWND hWnd = get_window_handle(window);
	if(hWnd)
		destroy_window(hWnd);
}

void nu_app::handle_messages()
{
	Poco::FastMutex::ScopedLock lock(mutex);

	bool show_another_window = false;

	// Main loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while(msg.message != WM_QUIT)
	{
		if(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}

		FOR_EACH(WindowByHandle &window_by_handle, windows)
		{
			HWND hWnd = window_by_handle.first;
			nu_window &window = window_by_handle.second;

			if(renderer)
			{
				renderer->NewFrame(window_by_handle.first);

		#if 0
				// 1. Show a simple window
				// Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
				{
					static float f = 0.0f;
					ImGui::Text("Hello, world!");
					ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
					ImGui::ColorEdit3("clear color", (float*)&clear_col);
					if (ImGui::Button("Test Window")) show_test_window ^= 1;
					if (ImGui::Button("Another Window")) show_another_window ^= 1;
					ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
				}

				// 2. Show another simple window, this time using an explicit Begin/End pair
				if (show_another_window)
				{
					ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
					ImGui::Begin("Another Window", &show_another_window);
					ImGui::Text("Hello");
					ImGui::End();
				}

				// 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
				if (show_test_window)
				{
					ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
					ImGui::ShowTestWindow(&show_test_window);
				}
		#endif

				bool window_closed = window.on_update ? window.on_update(&window) : false;

				if(!window_closed)
				{
					RECT client_rect = { 0 };
					GetClientRect(hWnd, &client_rect);

					ImGui::SetNextWindowPos(ImVec2(0,  0));
					ImGui::SetNextWindowSize(ImVec2(client_rect.right - client_rect.left, client_rect.bottom - client_rect.top));

					ImGui::Begin(GW_T2A(window.title.c_str()), &show_another_window, options.no_native_windows ? 0 : ImGuiWindowFlags_NoTitleBar/*ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar*/);
				}

				if(window.on_gui)
					window_closed = window.on_gui(&window);

				if(!window_closed)
				{
					pos = ImGui::GetWindowPos();

					ImGui::End();

					//GetWindowRect(hWnd, &window_rect);

					////MoveWindow(hwnd, pos.x, pos.y, , );
					//SetWindowPos(hWnd, 
					//			 HWND_TOP, 
					//			 window_rect.left + pos.x, 
					//			 window_rect.top + pos.y, 
					//			 0, 0,          // Ignores size arguments. 
					//			 SWP_NOSIZE);

					//std::vector<user_title_info_t>::iterator current_title_it = userinfo.titles.begin() + current_title_index;

					// Rendering
					renderer->RenderFrame(window.render_view);
				}
			}
		}
	}
}

void nu_app::get_desktop_size(uint32_t &desktop_width, uint32_t &desktop_height)
{
	//unsigned int nScreenWidth;
	//unsigned int nScreenHeight;

	//nScreenWidth  = ::GetSystemMetrics(SM_CXSCREEN);
	//nScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);

	RECT desktop_work_area_rect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &desktop_work_area_rect, 0);

	desktop_width  = desktop_work_area_rect.right - desktop_work_area_rect.left;
	desktop_height = desktop_work_area_rect.bottom - desktop_work_area_rect.top;
}

