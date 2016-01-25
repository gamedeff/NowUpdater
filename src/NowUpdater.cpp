//
// NowUpdater.cpp
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#include "NowUpdater.h"
//-----------------------------------------------------------------------------------
#include "Poco/DirectoryIterator.h"

using Poco::DirectoryIterator;
//-----------------------------------------------------------------------------------
template<class T> bool ImGui_Items_StringGetter(void* data, int idx, const char** out_text)
{
	static char str_fmt[64] = "%s";

	T::const_iterator *pitems_it = (T::const_iterator *) data;
	if(out_text)
	{
		T::const_iterator items_it = (*pitems_it) + idx;

		//*out_text = items_it->name.c_str();
		*out_text = _FS_narrow(str_fmt, items_it->c_str());
	}
	return true;
}

#define ImGui_Edit(a, b, s) \
	if(b.empty()) \
		b.resize(s); \
\
	ImGui::Text(a); \
	ImGui::SameLine(); \
	ImGui::InputText("##" GW_TOSTRING(b), &b[0], b.size());
//-----------------------------------------------------------------------------------
NowUpdater::NowUpdater() : nu_app(_T("Now Updater")), current_user(0), userinfo(0), choose_user_window(0), new_user_window(0), options_window(0), main_window(0),
														timer(0, options.mediaplayer_check_delay), timer_started(false), download_images_thread_adapter(*this, &NowUpdater::download_images)
{
}

bool NowUpdater::choose_user_ui(nu_window *window)
{
	ImGui::PushItemWidth(-1);

	if(!users.empty())
		ImGui::ListBox("##empty", (int *)&current_user, ImGui_Items_StringGetter< std::vector<std::string> >, &users.begin(), users.size(), 4);

	static std::string username;

	if(!users.empty())
		username = users[current_user];

	ImGui_Edit("User: ", username, 256);

	static std::string password = "nowupdater2015";
	ImGui_Edit("Password: ", password, 256);

	if(ImGui::Button("OK") && username != "" && password != "")
	{
		if(create_user(username, password))
		{
			assert(&windows[choose_user_window] == window);

			destroy_window(window);
			choose_user_window = 0;

			return true;
		}
	}

	ImGui::SameLine();

	if(ImGui::Button("New user"))
	{
		uint32_t w = 400, h = 300;

		new_user_window = create_and_show_window_center(_T("New user"), w, h, CLOSURE(this, &NowUpdater::new_user_ui));
		if(new_user_window != 0)
		{
			assert(&windows[choose_user_window] == window);

			destroy_window(window);
			choose_user_window = 0;

			return true;
		}
	}

	ImGui::PopItemWidth();

	return false;
}

bool NowUpdater::new_user_ui(nu_window *window)
{
	static std::string username;
	ImGui_Edit("User: ", username, 256);

	static std::string password = "nowupdater2015";
	ImGui_Edit("Password: ", password, 256);

	if(ImGui::Button("OK") && username != "" && password != "")
	{
		if(create_user(username, password))
		{
			assert(&windows[new_user_window] == window);

			destroy_window(window);
			new_user_window = 0;

			return true;
		}
	}

	ImGui::SameLine();

	if(ImGui::Button("Cancel"))
	{
		assert(&windows[new_user_window] == window);

		destroy_window(window);
		new_user_window = 0;

		return false;
	}

	return false;
}

bool NowUpdater::create_user(std::string username, std::string password)
{
	std::vector<std::string>::const_iterator where_it = std::find(users.begin(), users.end(), username);
	if(where_it != users.end())
		current_user = where_it - users.begin();
	else
	{
		users.push_back(username);
		current_user = users.size() - 1;
	}

	userinfo = new user_info_t(users[current_user], password, &options);
	if(!userinfo->init())
		return false;

	uint32_t w = 800, h = 600;

	string_t window_title = options.no_native_windows ? title : GW_A2T(userinfo->username + "'s list ");

	main_window = create_and_show_window(window_title, options.x, options.y, w, h, CLOSURE(this, &NowUpdater::main_ui));
	if(!main_window)
		return false;

	windows[main_window].on_update = CLOSURE(this, &NowUpdater::main_update);

	if(options.preload_images)
	{
		download_images();

		load_images();
	}
	else if(!download_images_thread.isRunning())
	{
		download_images_thread.start(download_images_thread_adapter);
	}

	return main_window != 0;
}

void NowUpdater::download_images()
{
	for(uint32_t i = 0; i < userinfo->site_users.size(); ++i)
	{
		uint32_t si = userinfo->site_users[i].site_index;

		for(uint32_t k = 0; k < userinfo->site_users[i].user_titles.size(); ++k)
			userinfo->sites[si]->download_title_images(userinfo->site_users[i].user_titles[k].index);
	}
}

void NowUpdater::load_images()
{
	for(uint32_t i = 0; i < userinfo->site_users.size(); ++i)
	{
		uint32_t si = userinfo->site_users[i].site_index;

		for(uint32_t k = 0; k < userinfo->site_users[i].user_titles.size(); ++k)
			if(!userinfo->sites[si]->titles[userinfo->site_users[i].user_titles[k].index].cover_texture_data.empty() && !userinfo->sites[si]->titles[userinfo->site_users[i].user_titles[k].index].cover_texture.handle)
			{
				renderer->LoadImage(userinfo->sites[si]->titles[userinfo->site_users[i].user_titles[k].index].cover_texture_data.c_str(), userinfo->sites[si]->titles[userinfo->site_users[i].user_titles[k].index].cover_texture_data.size(), userinfo->sites[si]->titles[userinfo->site_users[i].user_titles[k].index].cover_texture);
			}
	}
}

bool NowUpdater::main_ui(nu_window *window)
{
	assert(&windows[main_window] == window);

	userinfo->ui();

	return false;
}

bool NowUpdater::main_update(nu_window *window)
{
	assert(&windows[main_window] == window);

	load_images();

	if(userinfo)
	{
		if(!timer_started)
		{
			timer.start(Poco::TimerCallback<user_info_t>(*userinfo, &user_info_t::on_timer));
			//Thread::sleep(5000);
			//timer->stop();

			timer_started = true;
		}

		HWND hWnd = get_window_handle(window);
		if(hWnd)
			handle_popup(hWnd);
	}

	return false;
}

void NowUpdater::handle_popup(HWND hWnd)
{
	RECT window_rect = { 0 };
	GetWindowRect(hWnd, &window_rect);

	uint32_t desktop_work_area_width, desktop_work_area_height;
	get_desktop_size(desktop_work_area_width, desktop_work_area_height);

	uint32_t window_rect_width  = userinfo->show_title_popup ? popup_w : windows[hWnd].w; //window_rect.right - window_rect.left;
	uint32_t window_rect_height = userinfo->show_title_popup ? userinfo->get_cover_height(userinfo->current_title_index) + ImGui::GetStyle().FramePadding.y + ImGui::GetStyle().WindowPadding.y + ImGui::GetStyle().ItemSpacing.y/*popup_h*/ : windows[hWnd].h; //window_rect.bottom - window_rect.top;

	window_rect.left = userinfo->show_title_popup ? desktop_work_area_width - window_rect_width : windows[hWnd].x;
	window_rect.top  = userinfo->show_title_popup ? desktop_work_area_height - window_rect_height : windows[hWnd].y;
	window_rect.right = window_rect.left + window_rect_width;
	window_rect.bottom = window_rect.top + window_rect_height;

	static bool hiden = false;

	if(userinfo->show_title_popup)
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

bool NowUpdater::init()
{
	users.clear();

	std::string users_path = options.get_data_path() + Path::separator() + "users";

	DirectoryIterator user_dir_it(users_path);
	DirectoryIterator user_dir_end;
	while(user_dir_it != user_dir_end)
	{
		if(user_dir_it->exists() && user_dir_it->isDirectory() && user_dir_it->canRead())
		{
			users.push_back(user_dir_it.name());
		}

		++user_dir_it;
	}

	if(users.empty() || users.size() > 1)
	{
		uint32_t w = 400, h = 300;

		choose_user_window = create_and_show_window_center(_T("Login"), w, h, CLOSURE(this, &NowUpdater::choose_user_ui));
		return choose_user_window != 0;
	}
	else
	{
		std::string username;

		if(!users.empty())
			username = users[current_user];

		std::string password = "nowupdater2015";

		return create_user(username, password);
	}
}

void NowUpdater::destroy()
{
	//FOR_EACH(user_info_t *user, users)
	//{
	//	user->save();

	//	delete user;
	//	user = 0;
	//}

	if(timer_started)
		timer.stop();

	users.clear();

	delete userinfo;

	nu_app::destroy();
}

int main(int argc, char** argv)
{
	int exit_code = 0;

	app = new NowUpdater();

	if(!app->init())
	{
		exit_code = 1;
		goto end;
	}

	app->handle_messages();

end:

	app->destroy();

	delete app;

	return exit_code;
}

