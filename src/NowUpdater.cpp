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
NowUpdater::NowUpdater() : nu_app(_T("Now Updater")), current_user(0), userinfo(0), choose_user_window(0), new_user_window(0), options_window(0), main_window(0)
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
	return main_window != 0;
}

bool NowUpdater::main_ui(nu_window *window)
{
	assert(&windows[main_window] == window);

	userinfo->ui();

	return false;
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

	uint32_t w = 400, h = 300;

	choose_user_window = create_and_show_window_center(_T("Login"), w, h, CLOSURE(this, &NowUpdater::choose_user_ui));
	return choose_user_window != 0;
}

void NowUpdater::destroy()
{
	//FOR_EACH(user_info_t *user, users)
	//{
	//	user->save();

	//	delete user;
	//	user = 0;
	//}

	users.clear();

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

	uint32_t popup_w = 400, popup_h = 300;

	app->handle_messages(popup_w, popup_h);

end:

	app->destroy();

	delete app;

	return exit_code;
}

