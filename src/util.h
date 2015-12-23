//===================================================================================
// GameWare Engine      
// Copyright (C) GW-Labs, 2004-2009
//===================================================================================
// Header name:         util.h
// Header date:         23.7.2009 - 16:28
// Header author:       Fedor Gavrilov aka Dorfe
// Header description:  
//===================================================================================
#ifndef UTIL_H
#define UTIL_H
//-----------------------------------------------------------------------------------
#include "config.h"
#include "nu_types.h"
//-----------------------------------------------------------------------------------
// Convert char* string to wchar_t* string
//-----------------------------------------------------------------------------------
GAMEWARE_API const wchar_t* gwToWChar(const char *src);
//-----------------------------------------------------------------------------------
inline const wchar_t* gwToWChar(const std::string &src) { return gwToWChar(src.c_str()); }
//-----------------------------------------------------------------------------------
// Convert wchar_t* string to char* string
//-----------------------------------------------------------------------------------
GAMEWARE_API const char* gwToChar(const wchar_t *src);
//-----------------------------------------------------------------------------------
inline const char* gwToChar(const std::wstring &src) { return gwToChar(src.c_str()); }
//-----------------------------------------------------------------------------------
#define GW_A2W gwToWChar
#define GW_W2A gwToChar
//-----------------------------------------------------------------------------------
#ifdef GW_UNICODE
#	define GW_A2T(x) GW_A2W(x)
#	define GW_T2A(x) GW_W2A(x)
#	define GW_W2T(x) (x)
#	define GW_T2W(x) (x)
#	define _FS _FS_wide
#else
#	define GW_A2T(x) (x)
#	define GW_T2A(x) (x)
#	define GW_W2T(x) GW_W2A(x)
#	define GW_T2W(x) GW_A2W(x)
#	define _FS _FS_narrow
#endif
//-----------------------------------------------------------------------------------
//GAMEWARE_API char_t *_FS(const char_t *format, ...);
GAMEWARE_API char *_FS_narrow(const char *format, ...);
GAMEWARE_API wchar_t *_FS_wide(const wchar_t *format, ...);
//-----------------------------------------------------------------------------------
GAMEWARE_API char_t *string_replace(const char_t *s, const char_t *orig, char_t *rep);

#include <string>
#include <vector>
#include <sstream>

inline std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems)
{
	std::stringstream ss(s);
	std::string item;
	while(std::getline(ss, item, delim))
	{
		elems.push_back(item);
	}
	return elems;
}

inline std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

inline std::string substr_after_first_of(std::string const& s, char delim)
{
	std::string::size_type pos = s.find_first_of(delim);

	if(pos != std::string::npos)
		return s.substr(pos + 1, s.length());
	else
		return s;
}
//-----------------------------------------------------------------------------------
#endif
//===================================================================================
// Copyright (C) GW-Labs, 2004-2009
//===================================================================================