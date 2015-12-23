//
// NowUpdater
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#include "nu_mediaplayer.h"
//-----------------------------------------------------------------------------------
#include "anitomy/anitomy.h"
//-----------------------------------------------------------------------------------
#include "winwmi.h"
//-----------------------------------------------------------------------------------
#define POCO_NO_UNWINDOWS
//-----------------------------------------------------------------------------------
#include "Poco/Path.h"
#include "Poco/UnicodeConverter.h"
//-----------------------------------------------------------------------------------
#include <windows.h>
#include <shlwapi.h>
//-----------------------------------------------------------------------------------
#pragma message("Linking with shlwapi.lib...")
#pragma comment(lib, "shlwapi.lib")
//-----------------------------------------------------------------------------------
string_t GetProcessCommandLine(const char_t *process_name)
{
	return ExecWMIQuery<string_t, char_t>(bstr_t("SELECT CommandLine FROM Win32_Process WHERE Name = '") + bstr_t(process_name) + bstr_t("'"), L"CommandLine");
}
//-----------------------------------------------------------------------------------
struct mediaplayerchar_t
{
	const char_t *mediaplayer_name;
	const char_t *mediaplayer_title;
	const char_t *mediaplayer_class;
};
//-----------------------------------------------------------------------------------
bool is_mediaplayer_name(const mediaplayerchar_t mp, const char_t *mediaplayer)
{
	return _tcscmp(mp.mediaplayer_name, mediaplayer) == 0;
}
//-----------------------------------------------------------------------------------
const uint32_t MEDIAPLAYER_CHAR_T_NUM = 1;
const mediaplayerchar_t MEDIAPLAYER_CHAR_T[MEDIAPLAYER_CHAR_T_NUM] = 
{
	{ _T("mpc-hc.exe"), _T("Media Player Classic Home Cinema"), _T("MediaPlayerClassicW") }
};
//-----------------------------------------------------------------------------------
uint32_t get_mediaplayer_index(const char_t *mediaplayer)
{
	uint32_t index = std::find_if(MEDIAPLAYER_CHAR_T, MEDIAPLAYER_CHAR_T + MEDIAPLAYER_CHAR_T_NUM, std::bind2nd(std::ptr_fun(is_mediaplayer_name), mediaplayer)) - MEDIAPLAYER_CHAR_T;
	if(index >= MEDIAPLAYER_CHAR_T_NUM)
		return GW_NOT_FOUND;

	return index;
}
//-----------------------------------------------------------------------------------
HWND get_mediaplayer_handle(uint32_t index)
{
	return index != GW_NOT_FOUND ? FindWindow(MEDIAPLAYER_CHAR_T[index].mediaplayer_class, NULL) : 0;
}
//-----------------------------------------------------------------------------------
HWND get_mediaplayer_handle(const char_t *mediaplayer)
{
	uint32_t index = get_mediaplayer_index(mediaplayer);

	return get_mediaplayer_handle(index);
}
//-----------------------------------------------------------------------------------
typedef BOOL (CALLBACK* PStartHook)(HMODULE, HWND, HWND);
typedef BOOL (CALLBACK* PStopHook)();
typedef BOOL (CALLBACK* PIsHookRunning)();
//-----------------------------------------------------------------------------------
struct msg_hook
{
	PStartHook     StartHook;
	PStopHook      StopHook;
	PIsHookRunning IsHookRunning;

	HMODULE hHookModule;

	WNDCLASSEX WndClassEx;
	HWND hWnd;

	msg_hook(UINT message, HWND hTargetWnd, WNDPROC pWndProc, const char_t *hook_filename, const char_t *class_name = _T("msg_hook_class"))
	{
		HINSTANCE hCurrentInstance = GetModuleHandle(NULL);

		ZeroMemory(&WndClassEx, sizeof(WndClassEx));
		WndClassEx.cbSize = sizeof(WNDCLASSEX);
		WndClassEx.lpfnWndProc = pWndProc;
		WndClassEx.hInstance = hCurrentInstance;
		WndClassEx.lpszClassName = class_name;
		if((RegisterClassEx(&WndClassEx)) == NULL)
		{
			throw std::exception("Failed to register message only window class");
		}
		hWnd = CreateWindow(WndClassEx.lpszClassName, NULL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, NULL, NULL, NULL);
		if(hWnd == NULL)
		{
			throw std::exception("Failed to create message only window");
		}

		//const char_t lpcszClassName[] = _T("messageClass");
		//WNDCLASSEX  WindowClassEx;

		//ZeroMemory(&WindowClassEx, sizeof(WNDCLASSEX));
		//WindowClassEx.cbSize        = sizeof(WNDCLASSEX);
		//WindowClassEx.lpfnWndProc   = pWndProc;
		//WindowClassEx.hInstance     = hCurrentInstance;
		//WindowClassEx.lpszClassName = lpcszClassName;

		//if (RegisterClassEx(&WindowClassEx) != 0)
		//{
		//	// Create a message-only window
		//	hWnd = CreateWindowEx(0, lpcszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, WindowClassEx.hInstance, NULL);
		//}

		//hWnd = g_hWnd;

		StartHook = NULL;
		StopHook = NULL;
		IsHookRunning = NULL;

		char_t hook_path[MAX_PATH] = {0};

		GetModuleFileName(NULL, hook_path, MAX_PATH);
		PathRemoveFileSpec(hook_path);
		PathAppend(hook_path, hook_filename);

		hHookModule = LoadLibrary(hook_path);
		if(hHookModule != NULL)
		{
			StartHook     = (PStartHook)GetProcAddress(hHookModule, "StartHook");
			StopHook      = (PStopHook)GetProcAddress(hHookModule, "StopHook");
			IsHookRunning = (PIsHookRunning)GetProcAddress(hHookModule, "IsHookRunning");
		}

		ChangeWindowMessageFilter(message, MSGFLT_ADD);

		BOOL isRunning = IsHookRunning && IsHookRunning();

		StartHook(hHookModule, hWnd, hTargetWnd);

		//BOOL isRunning = IsHookRunning();
	}

	bool check_msg()
	{
		bool got_it = false;

		MSG msg;
		while(PeekMessage(&msg, hWnd,  0, 0, PM_REMOVE)) 
		//while(GetMessage(&msg, hWnd, 0, 0)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			got_it = true;
		}

		return got_it;
	}

	~msg_hook()
	{
		StopHook();

		DestroyWindow(hWnd);
		UnregisterClass(WndClassEx.lpszClassName, WndClassEx.hInstance);
	}
};
//-----------------------------------------------------------------------------------
static msg_hook *copydata_msg_hook = 0;
HWND hLastMediaPlayer = 0;
std::vector<string_t> cmdln;
//-----------------------------------------------------------------------------------
LRESULT CALLBACK CopyDataWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	if(nMsg == WM_COPYDATA)
	{
		cmdln.clear();

		COPYDATASTRUCT *pCDS = (COPYDATASTRUCT *) lParam;
		DWORD len = *((DWORD*)pCDS->lpData);
		TCHAR* pBuff = (TCHAR*)((DWORD*)pCDS->lpData + 1);
		TCHAR* pBuffEnd = (TCHAR*)((BYTE*)pBuff + pCDS->cbData - sizeof(DWORD));

		while(len-- > 0 && pBuff < pBuffEnd)
		{
			string_t str(pBuff);
			pBuff += str.length() + 1;

			cmdln.push_back(str);
		}
	}

	return DefWindowProc(hWnd, nMsg, wParam, lParam);
}
//-----------------------------------------------------------------------------------
#ifdef _DEBUG
#	define COPYDATA_MSG_HOOK_DLL _T("CopyDataMsgHook_d.dll")
#else
#	define COPYDATA_MSG_HOOK_DLL _T("CopyDataMsgHook.dll")
#endif
//-----------------------------------------------------------------------------------
int get_title_filename_from_WM_COPYDATA(const char_t *mediaplayer, char_t *filename, int filename_size)
{
	if(!filename || !filename_size)
		return 0;

	HWND hMediaPlayer = get_mediaplayer_handle(mediaplayer);

	if(hLastMediaPlayer != hMediaPlayer)
	{
		get_title_filename_from_WM_COPYDATA_cleanup(mediaplayer);
		hLastMediaPlayer = hMediaPlayer;
	}

	if(!hMediaPlayer)
		return 0;

	if(!copydata_msg_hook)
		copydata_msg_hook = new msg_hook(WM_COPYDATA, hMediaPlayer, CopyDataWndProc, COPYDATA_MSG_HOOK_DLL);

	if(!copydata_msg_hook->check_msg())
		return 0;

	if(!filename_size)
	{
		filename_size = cmdln.front().size() + 1;
		return filename_size;
	}

	_tcsncpy(filename, &cmdln.front()[0], filename_size - 1);
	return filename_size - 1;
}
//-----------------------------------------------------------------------------------
int get_title_filename_from_WM_COPYDATA_cleanup(const char_t *mediaplayer)
{
	if(copydata_msg_hook)
	{
		delete copydata_msg_hook;
		copydata_msg_hook = 0;
	}

	return 0;
}
//-----------------------------------------------------------------------------------
int get_title_filename_from_windowtitle(const char_t *mediaplayer, char_t *filename, int filename_size)
{
	if(!filename || !filename_size)
		return 0;

	uint32_t index = get_mediaplayer_index(mediaplayer);

	HWND hMediaPlayer = get_mediaplayer_handle(index);

	if(!hMediaPlayer)
		return 0;

	if(!filename_size)
	{
		filename_size = GetWindowTextLength(hMediaPlayer) + 1;
		return filename_size;
	}

	int size = GetWindowText(hMediaPlayer, filename, filename_size - 1);

	if(_tcscmp(MEDIAPLAYER_CHAR_T[index].mediaplayer_title, filename) == 0)
		return 0;

	return size;
}
//-----------------------------------------------------------------------------------
int get_title_filename_from_windowtitle_cleanup(const char_t *mediaplayer)
{
	return 0;
}
//-----------------------------------------------------------------------------------
int get_title_filename_from_cmdline(const char_t *mediaplayer, char_t *filename, int filename_size)
{
	if(!filename || !filename_size)
		return 0;

	HWND hMediaPlayer = get_mediaplayer_handle(mediaplayer);

	if(hLastMediaPlayer != hMediaPlayer)
	{
		get_title_filename_from_cmdline_cleanup(mediaplayer);
		hLastMediaPlayer = hMediaPlayer;
	}
	else
		return 0; // one time per mediaplayer start

	if(!hMediaPlayer)
		return 0;

	string_t cmd = GetProcessCommandLine(mediaplayer);

	const string_t space = _T(" ");
	string_t::size_type space_pos = cmd.find(space);
	if(space_pos == string_t::npos)
		return 0;

	const string_t quote = _T("\"");
	string_t::size_type start = 0;
	string_t::size_type quote_2nd_pos = string_t::npos;
	string_t::size_type quote_3rd_pos = string_t::npos;
	string_t::size_type quote_4th_pos = string_t::npos;
	uint32_t occurrences = 0;
	while((start = cmd.find(quote, start)) != string_t::npos)
	{
		++occurrences;
		if(occurrences == 2)
			quote_2nd_pos = start;
		else if(occurrences == 3)
			quote_3rd_pos = start;
		else if(occurrences == 4)
		{
			quote_4th_pos = start;
			break;
		}
		start += quote.length();
	}
	string_t title_filename_path_str;
	if(occurrences >= 4 && space_pos > quote_2nd_pos && space_pos < quote_3rd_pos)
		title_filename_path_str = cmd.substr(quote_3rd_pos + quote.length(), quote_4th_pos - quote_3rd_pos - quote.length());
	else if(occurrences >= 2 && space_pos > quote_2nd_pos)
		title_filename_path_str = cmd.substr(space_pos + space.length());

#if !defined(NU_CONFIG_UTF8) && defined(POCO_WIN32_UTF8)
	std::string title_filename_path_str_utf8;
	Poco::UnicodeConverter::toUTF8(title_filename_path_str, title_filename_path_str_utf8);

	Poco::Path title_filename_path(title_filename_path_str_utf8);
#else
	Poco::Path title_filename_path(title_filename_path_str);
#endif

#if !defined(NU_CONFIG_UTF8) && defined(POCO_WIN32_UTF8)
	std::wstring title_filename_str;
	std::string title_filename_str_utf8 = title_filename_path.getFileName();
	Poco::UnicodeConverter::toUTF16(title_filename_str_utf8, title_filename_str);
#else
	std::string title_filename_str = title_filename_path.getFileName();
#endif

	if(!filename_size)
	{
		filename_size = title_filename_str.size() + 1;
		return filename_size;
	}

	_tcsncpy(filename, &title_filename_str[0], filename_size - 1);
	return filename_size - 1;
}
//-----------------------------------------------------------------------------------
int get_title_filename_from_cmdline_cleanup(const char_t *mediaplayer)
{
	return 0;
}
//-----------------------------------------------------------------------------------

int get_title_filename_info_anitomy(const char_t *filename, title_filename_info_t *title_filename_info)
{
	if(!filename || !title_filename_info)
		return 0;

	anitomy::Anitomy anitomy;
	anitomy.Parse(filename);
	anitomy::Elements& elements = anitomy.elements();

	title_filename_info->title = elements.get(anitomy::kElementAnimeTitle);
	title_filename_info->episode_number = _wtoi(elements.get(anitomy::kElementEpisodeNumber).c_str());
	title_filename_info->year = _wtoi(elements.get(anitomy::kElementAnimeYear).c_str());

	return 1;
}


//#include <commctrl.h>
//
//HWND hListView;
//
//BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
//{
//	hListView = FindWindowEx(hwnd, NULL, "SysListView32", NULL);
//
//	if(!hListView)
//		return TRUE;
//
//	//MessageBox(NULL, "FOUND SysListView32", "found", MB_ICONINFORMATION);
//	return FALSE;
//}

//int pllist()
//{
//	HWND hMediaPlayerMain, hPlaylist;
//	int list = 0;
//
//	hMediaPlayerMain = FindWindow(MPCHC_WINDOW_CLASS, NULL);
//
//	if(!hMediaPlayerMain)
//	{
//		MessageBox(NULL, "not running program", "error", MB_ICONINFORMATION);
//		return 0;
//	}
//
//	//hChild = FindWindowEx(hMain, NULL, "TopWindow", NULL);
//	//hChild = FindWindowEx(hChild, NULL, "TabListManager", NULL);
//	hPlaylist = FindWindowEx(hMediaPlayerMain, NULL, NULL, "Playlist");
//
//	EnumChildWindows(hPlaylist, &EnumChildProc, NULL); 
//
//	list = ListView_GetItemCount(hListView);
//
//	LVITEM item = {0};
//	char buffer[256] = { '\0' };
//	for(int i = list; i > 0; i--)
//	{
//		//ListView_GetItemText(hListView, i - 1, 0, buffer, 255);
//		ListView_GetItem(hListView, &item);
//		printf("item: %s, state: %d\n", buffer, item.state);
//	}
//
//	// Get the first selected item
//	int iPos = ListView_GetNextItem(hListView, -1, 0/*LVNI_SELECTED*/);
//	while(iPos != -1)
//	{
//		// iPos is the index of a selected item
//		// do whatever you want with it
//
//		ListView_GetItemText(hListView, iPos, 0, buffer, 255);
//		printf("contact: %s\n", buffer);
//
//		// Get the next selected item
//		iPos = ListView_GetNextItem(hListView, iPos, LVNI_SELECTED);
//	}
//
//	printf("Number of contacts: %d\n", list);
//
//	return 0;
//}
