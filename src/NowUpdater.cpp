//
// NowUpdater.cpp
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#include "NowUpdater.h"

#include "nu_app.h"

int main(int argc, char** argv)
{
	int exit_code = 0;

	app = new nu_app("nowupdater", "nowupdater2015");

	if(!app->init())
	{
		exit_code = 1;
		goto end;
	}

	uint32_t popup_w = 400, popup_h = 300;

	uint32_t x = app->options.x, y = app->options.y, w = 800, h = 600;

	uint32_t n = 1;

	for(int i = 0; i < n; ++i)
	{
		TCHAR NU_APP_TITLE[] = _T("Now Updater");

		string_t window_title = app->options.no_native_windows ? NU_APP_TITLE : GW_A2T(app->userinfo.username + "'s list ") + string_t(GW_A2T(std::to_string(i)));

		HWND hWnd = app->create_window(window_title, x, y, w, h);
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

