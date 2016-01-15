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
struct options_t
{
	bool no_native_windows;

	bool animate_popup;

	uint32_t popup_delay;

	float min_rating, max_rating;

	uint32_t mediaplayer_check_delay;

	uint32_t x, y;

	std::string app_name, data_dir, xml_ext;

	options_t() : no_native_windows(false), animate_popup(false), popup_delay(3000), min_rating(1), max_rating(10), mediaplayer_check_delay(5000), x(100), y(100), data_dir("data"), xml_ext(".xml") {}
};
//-----------------------------------------------------------------------------------
#endif
