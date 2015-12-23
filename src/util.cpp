//===================================================================================
// GameWare Engine      
// Copyright (C) GW-Labs, 2004-2009
//===================================================================================
// Module name:         util.cpp
// Module date:         23.7.2009 - 16:29
// Module author:       Fedor Gavrilov aka Dorfe
// Module description:  
//===================================================================================
// Precompiled header:
//-----------------------------------------------------------------------------------
//#include "precompiled.h"
//#pragma hdrstop
//-----------------------------------------------------------------------------------
#include "util.h"
//-----------------------------------------------------------------------------------
#include <assert.h>
#include <locale.h>
//-----------------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------------
const int WCHAR_MAX_COUNT = 16;                                 // max number of string buffers
const int WCHAR_MAX_LENGTH = 1024;                              // max string length per buffer
//-----------------------------------------------------------------------------------
static wchar_t wchar_wideStr[WCHAR_MAX_COUNT][WCHAR_MAX_LENGTH];// circular buffer for wchar_t*
static char wchar_str[WCHAR_MAX_COUNT][WCHAR_MAX_LENGTH];       // circular buffer for char*
static int wchar_indexWchar = 0;                                // current index of circular buffer
static int wchar_indexChar = 0;                                 // current index of circular buffer
//-----------------------------------------------------------------------------------
// Convert char* string to wchar_t* string
//-----------------------------------------------------------------------------------
GAMEWARE_API const wchar_t* gwToWChar(const char *src)
{
	wchar_indexWchar = (++wchar_indexWchar) % WCHAR_MAX_COUNT;  // circulate index

	setlocale(LC_ALL, "");

	mbstowcs(wchar_wideStr[wchar_indexWchar], src, WCHAR_MAX_LENGTH); // copy string as wide char
	wchar_wideStr[wchar_indexWchar][WCHAR_MAX_LENGTH-1] = L'\0';      // in case when source exceeded max length

	return wchar_wideStr[wchar_indexWchar];                     // return string as wide char
}
//-----------------------------------------------------------------------------------
// Convert wchar_t* string to char* string
//-----------------------------------------------------------------------------------
GAMEWARE_API const char* gwToChar(const wchar_t* src)
{
	wchar_indexChar = (++wchar_indexChar) % WCHAR_MAX_COUNT;    // circulate index

	wcstombs(wchar_str[wchar_indexChar], src, WCHAR_MAX_LENGTH);// copy string as char
	wchar_str[wchar_indexChar][WCHAR_MAX_LENGTH-1] = '\0';      // in case when source exceeded max length

	return wchar_str[wchar_indexChar];                          // return string as char
}
//-----------------------------------------------------------------------------------
//char_t _FS_buffer[GW_MAX_STACK_FS_COUNT][GW_MAX_STACK_ERROR_STR_LEN];
//uint32_t _FS_index = 0;
//-----------------------------------------------------------------------------------
//GAMEWARE_API char_t *_FS(const char_t *format, ...)
//{
//	_FS_index = (++_FS_index) % GW_MAX_STACK_FS_COUNT;  // circulate index
//
//	va_list args;
//	int len;
//
//	va_start(args, format);
//		len = _vsctprintf(format, args) + 1; // _vsctprintf doesn't count terminating '\0'
//		assert(len <= GW_MAX_STACK_ERROR_STR_LEN);
//		_vstprintf(_FS_buffer[_FS_index], format, args);
//	va_end(args);
//
//	return _FS_buffer[_FS_index];
//}
//-----------------------------------------------------------------------------------
char _FS_buffer_narrow[GW_MAX_STACK_FS_COUNT][GW_MAX_STACK_ERROR_STR_LEN];
uint32_t _FS_index_narrow = 0;
//-----------------------------------------------------------------------------------
GAMEWARE_API char *_FS_narrow(const char *format, ...)
{
	_FS_index_narrow = (++_FS_index_narrow) % GW_MAX_STACK_FS_COUNT;  // circulate index

	va_list args;
	int len;

	va_start(args, format);
		len = _vscprintf(format, args) + 1; // _vscprintf doesn't count terminating '\0'
		assert(len <= GW_MAX_STACK_ERROR_STR_LEN);
		vsprintf(_FS_buffer_narrow[_FS_index_narrow], format, args);
	va_end(args);

	return _FS_buffer_narrow[_FS_index_narrow];
}
//-----------------------------------------------------------------------------------
wchar_t _FS_buffer_wide[GW_MAX_STACK_FS_COUNT][GW_MAX_STACK_ERROR_STR_LEN];
uint32_t _FS_index_wide = 0;
//-----------------------------------------------------------------------------------
GAMEWARE_API wchar_t *_FS_wide(const wchar_t *format, ...)
{
	_FS_index_wide = (++_FS_index_wide) % GW_MAX_STACK_FS_COUNT;  // circulate index

	va_list args;
	int len;

	va_start(args, format);
		len = _vscwprintf(format, args) + 1; // _vscwprintf doesn't count terminating L'\0'
		assert(len <= GW_MAX_STACK_ERROR_STR_LEN);
		_vswprintf(_FS_buffer_wide[_FS_index_wide], format, args);
	va_end(args);

	return _FS_buffer_wide[_FS_index_wide];
}
//-----------------------------------------------------------------------------------
GAMEWARE_API char_t *string_replace(const char_t *s, const char_t *orig, char_t *rep)
{
	static char_t str[GW_MAX_STACK_FORMAT_STR_LEN];

	_tcscpy(str, s);

	char_t *ch;

	while((ch = (char_t *) _tcsstr(str, orig)) != NULL)
	{
		char_t buffer1[GW_MAX_STACK_FORMAT_STR_LEN];
		char_t buffer2[GW_MAX_STACK_FORMAT_STR_LEN];

		_tcsncpy(buffer1, str, ch - str);
		_tcsncpy(buffer2, str, ch - str + _tcslen(orig));

		buffer1[ch - str]                 = _T('\0');
		buffer2[ch - str + _tcslen(orig)] = _T('\0');

		if(_tcslen(orig) >= _tcslen(rep))
		{
			_stprintf(buffer1 + (ch - str), _T("%s%s"), rep, ch + _tcslen(orig));
		}
		else
		{
			_stprintf(buffer2 + (ch - str), _T("%s%s"), rep, ch + _tcslen(orig));
			_tcscpy(buffer1, buffer2/*, _tcslen(buffer1)*/);
		}

		_tcscpy(str, buffer1);
	}

	return str;
}
//-----------------------------------------------------------------------------------

//===================================================================================
// Copyright (C) GW-Labs, 2004-2009
//===================================================================================