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

static options_t options;

#include "imgui.h"

#include "util.h"

#include "render_d3d9.h"

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(!g_render)
		return false; // DefWindowProc(hWnd, msg, wParam, lParam);

	return g_render->WndProc(hWnd, msg, wParam, lParam);
}

int main(int argc, char** argv)
{
	//if (argc != 2)
	//{
	//	Path p(argv[0]);
	//	std::cout << "usage: " << p.getBaseName() << " <uri>" << std::endl;
	//	std::cout << "       fetches the resource identified by <uri> and print it to the standard output" << std::endl;
	//	return 1;
	//}

	options.app_name = "NowUpdater";

	user_info_t userinfo = user_info_t("nowupdater", "nowupdater2015", &options);

	if(!userinfo.init())
		return 1;

	g_render = new RenderD3D9(options.no_native_windows);

	if(!g_render->Init())
		return 0;

	TCHAR NU_WNDCLASS_NAME[] = _T("Now Updater");
	TCHAR NU_APP_TITLE[] = _T("Now Updater");

	std::string window_title = userinfo.username + "'s list";

	uint32_t x = options.x, y = options.y, w = 800, h = 600;
	uint32_t popup_w = 400, popup_h = 300;

	// Create application window
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL, NU_WNDCLASS_NAME, NULL };
	RegisterClassEx(&wc);
	HWND hWnd = CreateWindow(NU_WNDCLASS_NAME, options.no_native_windows ? NU_APP_TITLE : GW_A2T(window_title.c_str()), options.no_native_windows ? WS_POPUP : WS_OVERLAPPEDWINDOW, x, y, w, h, NULL, NULL, wc.hInstance, NULL);

	if(!g_render->InitDevice(hWnd))
	{
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 0;
	}

	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);

	if(options.no_native_windows)
	{
		RECT client_rect = { 0 };
		GetClientRect(hWnd, &client_rect);

		uint32_t ClientWidth  = client_rect.right - client_rect.left;
		uint32_t ClientHeight = client_rect.bottom - client_rect.top;

		if(!g_render->CreateRenderTarget(ClientWidth, ClientHeight, 4))
			;
	}

	bool show_test_window = true;
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

		g_render->NewFrame();

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

		uint32_t window_rect_width  = userinfo.show_title_popup ? popup_w : w; //window_rect.right - window_rect.left;
		uint32_t window_rect_height = userinfo.show_title_popup ? userinfo.get_cover_height(current_title_index) + ImGui::GetStyle().FramePadding.y + ImGui::GetStyle().WindowPadding.y + ImGui::GetStyle().ItemSpacing.y/*popup_h*/ : h; //window_rect.bottom - window_rect.top;

		window_rect.left = userinfo.show_title_popup ? desktop_work_area_width - window_rect_width : x;
		window_rect.top  = userinfo.show_title_popup ? desktop_work_area_height - window_rect_height : y;
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
			SetWindowPos(hWnd, HWND_TOP, x, y, window_rect_width, window_rect_height, 0);

			ShowWindow(hWnd, SW_SHOWNORMAL);
			hiden = false;
		}
		else
		{
			x = window_rect.left + pos.x;
			y = window_rect.top + pos.y;
		}

		RECT client_rect = { 0 };
		GetClientRect(hWnd, &client_rect);

		ImGui::SetNextWindowPos(ImVec2(0,  0));
		ImGui::SetNextWindowSize(ImVec2(client_rect.right - client_rect.left, client_rect.bottom - client_rect.top));

		ImGui::Begin(window_title.c_str(), &show_another_window, options.no_native_windows ? 0 : ImGuiWindowFlags_NoTitleBar/*ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar*/);

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
		g_render->RenderFrame();
	}

	timer.stop();

	userinfo.save();

	if(g_render)
	{
		g_render->Destroy();
		delete g_render;
		g_render = 0;
	}

	UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
}

