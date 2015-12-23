//
// NowUpdater
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#ifndef NU_TYPES_H
#define NU_TYPES_H
//-----------------------------------------------------------------------------------
#if defined(UNICODE) || defined(_UNICODE)
#	ifdef _WIN32
#		define NU_CONFIG_UTF16
#	else
#		define NU_CONFIG_UTF8
#	endif
#endif
//-----------------------------------------------------------------------------------
#if defined(_WIN32) && !defined(__MINGW32__)

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;  // NOLINT
typedef unsigned short uint16_t;  // NOLINT
typedef int int32_t;
typedef unsigned int uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
// intptr_t and friends are defined in crtdefs.h through stdio.h.

#else

#include <stdint.h>

#endif
//-----------------------------------------------------------------------------------
class noncopyable
{
private:

	noncopyable(const noncopyable&);
	noncopyable& operator=(const noncopyable&);

public:

	noncopyable() {}
};
//-----------------------------------------------------------------------------------
#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0600		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0600	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0600 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600	// Change this to the appropriate value to target other versions of IE.
#endif

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define NOMINMAX
#include <windows.h>
#include <tchar.h>

#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <iostream>

typedef char utf8char_t;
typedef wchar_t utf16char_t;

#if defined(NU_CONFIG_UTF16)

typedef utf16char_t char_t;

//extern std::basic_ostream<char_t> &tcout = std::wcout;
#define tcout std::wcout

#elif defined(NU_CONFIG_UTF8)

typedef utf8char_t char_t;

extern std::basic_ostream<char_t> &tcout = std::cout;

#else

typedef char char_t;

extern std::basic_ostream<char_t> &tcout = std::cout;

#endif

typedef std::basic_string<char_t> string_t;

namespace std
{
#if defined(_MSC_VER) && _MSC_VER <= 1500 // MSVC 2008 or earlier

	using namespace tr1;

//#include <unordered_map>

#endif

#define _LLFMT "%I64"

	inline string to_string(int v)
	{
		char buf[2 * _MAX_INT_DIG];

		sprintf_s(buf, sizeof(buf), "%d", v);
		return (string(buf));
	}

	inline string to_string(unsigned int v)
	{
		char buf[2 * _MAX_INT_DIG];

		sprintf_s(buf, sizeof(buf), "%u", v);
		return (string(buf));
	}

	inline string to_string(long v)
	{
		char buf[2 * _MAX_INT_DIG];

		sprintf_s(buf, sizeof(buf), "%ld", v);
		return (string(buf));
	}

	inline string to_string(unsigned long v)
	{
		char buf[2 * _MAX_INT_DIG];

		sprintf_s(buf, sizeof(buf), "%lu", v);
		return (string(buf));
	}

	inline string to_string(int64_t v)
	{
		char buf[2 * _MAX_INT_DIG];

		sprintf_s(buf, sizeof(buf), _LLFMT "d", v);
		return (string(buf));
	}

	inline string to_string(uint64_t v)
	{
		char buf[2 * _MAX_INT_DIG];

		sprintf_s(buf, sizeof(buf), _LLFMT "u", v);
		return (string(buf));
	}

	inline string to_string(long double v)
	{
		typedef back_insert_iterator<string> Iter;
		typedef num_put<char, Iter> Nput;
		const Nput& Nput_fac = use_facet<Nput>(locale());
		ostream Ios((streambuf *)0);
		string Str;

		Ios.setf(ios_base::fixed);
		Nput_fac.put(Iter(Str), Ios, ' ', v);
		return (Str);
	}

	inline string to_string(double v)
	{
		return (to_string((long double)v));
	}

	inline string to_string(float v)
	{
		return (to_string((long double)v));
	}
}

struct is_whitespace_char
{
	bool operator()(char c)
	{
		return (c =='\r' || c =='\t' || c == ' ' || c == '\n');
	}
};

inline std::string::iterator replace_whitespace(std::string &str)
{
	return str.erase(std::remove_if(str.begin(), str.end(), is_whitespace_char()), str.end());
}

inline float clampf(float x, float a, float b)
{
	return x < a ? a : (x > b ? b : x);
}

//-----------------------------------------------------------------------------------
// Miscellaneous helper functions
//-----------------------------------------------------------------------------------
#ifdef _countof
#	define countof _countof
#else
#	define countof(a) (sizeof(a) / sizeof(firstof(a)))
#endif
//-----------------------------------------------------------------------------------
#define firstof(a) a[0]
#define lastof(a)  a[countof(a) - 1]
//-----------------------------------------------------------------------------------
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p) = NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete [] (p);  (p) = NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p) = NULL; } }
//-----------------------------------------------------------------------------------
#endif
