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

	app = new nu_app(_T("Now Updater"));

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

