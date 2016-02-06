//
// nu_options.h
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#ifndef NU_OPTIONS_H
#define NU_OPTIONS_H
//-----------------------------------------------------------------------------------
#include "nu_types.h"
//-----------------------------------------------------------------------------------
#include "Poco/File.h"
#include "Poco/Path.h"

using Poco::Path;
//-----------------------------------------------------------------------------------
struct options_t
{
	bool no_native_windows;

	bool animate_popup;

	uint32_t popup_slide_time, popup_display_time;

	float min_rating, max_rating;

	uint32_t mediaplayer_check_delay;

	bool preload_images;

	int x, y;

	std::string app_name, data_dir, xml_ext;

	options_t() : no_native_windows(false),
				  animate_popup(false),
				  popup_slide_time(3000),
				  popup_display_time(5000),
				  min_rating(1),
				  max_rating(10),
				  mediaplayer_check_delay(5000),
				  preload_images(false),
				  x(100),
				  y(100),
				  data_dir("data"),
				  xml_ext(".xml")
	{}

	std::string get_data_path(bool createdirs = false)
	{
		std::string datapath = Path::dataHome() + app_name + Path::separator() + data_dir;

		if(createdirs)
			Poco::File(datapath).createDirectories();

		return datapath;
	}

	std::string get_data_path(const std::string &username, const std::string &dataname, bool createdirs = false)
	{
		return get_data_path(createdirs) + Path::separator() + "users" + Path::separator() + username + Path::separator() + dataname + xml_ext;
	}
};
//-----------------------------------------------------------------------------------
#endif
