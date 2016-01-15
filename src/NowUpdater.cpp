//
// NowUpdater.cpp
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#include "NowUpdater.h"

#include "nu_user_info.h"

using Poco::Timer;
using Poco::TimerCallback;

#include "nu_options.h"

#include "imgui.h"

#include "util.h"

#include "render_d3d9.h"

#include "for_each.h"

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct nu_window
{
	WNDCLASSEX wc;

	string_t title, classname;
	uint32_t x, y, w, h;

	RenderView *render_view;

	nu_window() : render_view(0) {}
};

struct nu_app
{
	options_t options;
	user_info_t userinfo;

	std::map<HWND, nu_window> windows;

	typedef std::pair<const HWND, nu_window> WindowByHandle;

	Render *renderer;

	nu_app() : userinfo("nowupdater", "nowupdater2015", &options), renderer(0)
	{
		options.app_name = "NowUpdater";
	}

	bool init()
	{
		if(!userinfo.init())
			return false;

		return true;
	}

	void destroy()
	{
		userinfo.save();

		FOR_EACH(WindowByHandle &window_by_handle, windows)
		{
			HWND hWnd = window_by_handle.first;
			nu_window &window = window_by_handle.second;

			//window.renderer->Destroy();
			//delete window.renderer;

			destroy_window(hWnd);
		}
	}

	WNDCLASSEX register_window_class(const char_t *wndclass)
	{
		WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL, wndclass, NULL };
		RegisterClassEx(&wc);
		return wc;
	}

	HWND create_window(const string_t &window_title, const string_t window_class, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
	{
		nu_window window;

		window.title = window_title;
		window.classname = window_class;
		window.wc = register_window_class(window.classname.c_str());
		window.x = x;
		window.y = y;
		window.w = w;
		window.h = h;

		// Create application window
		HWND hWnd = CreateWindow(window.classname.c_str(), window.title.c_str(), options.no_native_windows ? WS_POPUP : WS_OVERLAPPEDWINDOW,
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

	void destroy_window(HWND hWnd)
	{
		DestroyWindow(hWnd);

		UnregisterClass(windows[hWnd].classname.c_str(), windows[hWnd].wc.hInstance);
	}

	void handle_messages(uint32_t popup_w, uint32_t popup_h)
	{
		bool show_another_window = false;
		ImVec2 pos = ImVec2(0, 0);

		Timer timer(0, options.mediaplayer_check_delay); // fire immediately, repeat every 5000 ms
		timer.start(TimerCallback<user_info_t>(userinfo, &user_info_t::on_timer));
		//Thread::sleep(5000);
		//timer->stop();

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
					renderer->NewFrame();

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

					int current_title_index = 0;

					handle_popup(hWnd, popup_w, current_title_index, pos);

					RECT client_rect = { 0 };
					GetClientRect(hWnd, &client_rect);

					ImGui::SetNextWindowPos(ImVec2(0,  0));
					ImGui::SetNextWindowSize(ImVec2(client_rect.right - client_rect.left, client_rect.bottom - client_rect.top));

					ImGui::Begin(GW_T2A(window.title.c_str()), &show_another_window, options.no_native_windows ? 0 : ImGuiWindowFlags_NoTitleBar/*ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar*/);

					if(userinfo.show_title_popup)
					{
						static int current_title_status = NU_TITLE_STATUS_NOT_ADDED;

						userinfo.title_ui(current_title_status);
					}
					else
						current_title_index = userinfo.main();

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
					//renderer->Present();
				}
				//if(window.render_view)
				//	window.render_view->Present();
			}
		}

		timer.stop();
	}

	void handle_popup(HWND hWnd, uint32_t popup_w, int current_title_index, ImVec2 &pos);
};

void nu_app::handle_popup(HWND hWnd, uint32_t popup_w, int current_title_index, ImVec2 &pos)
{
	RECT window_rect = { 0 };
	GetWindowRect(hWnd, &window_rect);

	//unsigned int nScreenWidth;
	//unsigned int nScreenHeight;

	//nScreenWidth  = ::GetSystemMetrics(SM_CXSCREEN);
	//nScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);

	RECT desktop_work_area_rect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &desktop_work_area_rect, 0);

	uint32_t desktop_work_area_width  = desktop_work_area_rect.right - desktop_work_area_rect.left;
	uint32_t desktop_work_area_height = desktop_work_area_rect.bottom - desktop_work_area_rect.top;

	uint32_t window_rect_width  = userinfo.show_title_popup ? popup_w : windows[hWnd].w; //window_rect.right - window_rect.left;
	uint32_t window_rect_height = userinfo.show_title_popup ? userinfo.get_cover_height(current_title_index) + ImGui::GetStyle().FramePadding.y + ImGui::GetStyle().WindowPadding.y + ImGui::GetStyle().ItemSpacing.y/*popup_h*/ : windows[hWnd].h; //window_rect.bottom - window_rect.top;

	window_rect.left = userinfo.show_title_popup ? desktop_work_area_width - window_rect_width : windows[hWnd].x;
	window_rect.top  = userinfo.show_title_popup ? desktop_work_area_height - window_rect_height : windows[hWnd].y;
	window_rect.right = window_rect.left + window_rect_width;
	window_rect.bottom = window_rect.top + window_rect_height;

	static bool hiden = false;

	if(userinfo.show_title_popup)
	{
		AdjustWindowRectEx(&window_rect, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE));

		window_rect_width  = window_rect.right - window_rect.left;
		window_rect_height = window_rect.bottom - window_rect.top;

		if(!hiden)
		{
			ShowWindow(hWnd, SW_HIDE);
			hiden = true;
		}

		SetWindowPos(hWnd, /*HWND_TOPMOST*/HWND_TOP, window_rect.left, window_rect.top, window_rect_width, window_rect_height, 0/*SWP_NOREDRAW*//*SWP_HIDEWINDOW*/);
		if(options.animate_popup)
			AnimateWindow(hWnd, options.popup_delay, AW_SLIDE | AW_ACTIVATE | AW_VER_NEGATIVE);
		else
		{
			ShowWindow(hWnd, SW_SHOWNORMAL);
			//Sleep(options.popup_delay);
		}
		//UpdateWindow(hWnd);
	}
	else if(hiden)
	{
		SetWindowPos(hWnd, HWND_TOP, windows[hWnd].x, windows[hWnd].y, window_rect_width, window_rect_height, 0);

		ShowWindow(hWnd, SW_SHOWNORMAL);
		hiden = false;
	}
	else
	{
		windows[hWnd].x = window_rect.left + pos.x;
		windows[hWnd].y = window_rect.top + pos.y;
	}
}

nu_app *app;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(!app->renderer)
		return DefWindowProc(hWnd, msg, wParam, lParam);

	return app->renderer->WndProc(hWnd, msg, wParam, lParam);
}

int main(int argc, char** argv)
{
	int exit_code = 0;

	app = new nu_app();

	if(!app->init())
	{
		exit_code = 1;
		goto end;
	}

	uint32_t popup_w = 400, popup_h = 300;

	uint32_t x = app->options.x, y = app->options.y, w = 800, h = 600;

	uint32_t n = 4;

	for(int i = 0; i < n; ++i)
	{
		TCHAR NU_WNDCLASS_NAME[] = _T("NowUpdaterWindow");
		TCHAR NU_APP_TITLE[] = _T("Now Updater");

		string_t wndclass = NU_WNDCLASS_NAME + string_t(GW_A2T(std::to_string(i)));
		string_t window_title = app->options.no_native_windows ? NU_APP_TITLE : GW_A2T(app->userinfo.username + "'s list ") + string_t(GW_A2T(std::to_string(i)));

		HWND hWnd = app->create_window(window_title, wndclass.c_str(), x, y, w, h);
	//}

	//app->renderer->Reset(w, h);

	//FOR_EACH(nu_app::WindowByHandle &window_by_handle, app->windows)
	//{
	//	HWND hWnd = window_by_handle.first;
	//	nu_window &window = window_by_handle.second;

		ShowWindow(hWnd, SW_SHOWDEFAULT);
		UpdateWindow(hWnd);

		if(app->options.no_native_windows)
		{
			RECT client_rect = { 0 };
			GetClientRect(hWnd, &client_rect);

			uint32_t ClientWidth  = client_rect.right - client_rect.left;
			uint32_t ClientHeight = client_rect.bottom - client_rect.top;

			if(!app->renderer->CreateRenderTarget(ClientWidth, ClientHeight, 4))
				goto end;
		}

		app->windows[hWnd].render_view = app->renderer->CreateRenderView(hWnd);
		if(!app->windows[hWnd].render_view)
			goto end;
	}

	app->handle_messages(popup_w, popup_h);

end:

	app->destroy();

	delete app;

	return exit_code;
}

