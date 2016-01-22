//===================================================================================
// GameWare Engine      
// Copyright (C) GW-Labs, 2004-2015
//===================================================================================
// Header name:         config.h
// Header date:         21.5.2015 - 21:26
// Header author:       Fedor Gavrilov aka Dorfe
// Header description:  
//===================================================================================
#ifndef CONFIG_H
#define CONFIG_H
//-----------------------------------------------------------------------------------
#define GW_COMPILER_MSVC8_OR_HIGHER
//-----------------------------------------------------------------------------------
// Deprecated CRT and checked iterators
//-----------------------------------------------------------------------------------
#ifdef GW_COMPILER_MSVC8_OR_HIGHER
#	define _CRT_NON_CONFORMING_SWPRINTFS
//#	define _SECURE_SCL 0
//#	ifndef _HAS_ITERATOR_DEBUGGING
//#		define _HAS_ITERATOR_DEBUGGING 0
//#	endif
//#	ifndef _SCL_SECURE_NO_WARNINGS
//#		define _SCL_SECURE_NO_WARNINGS
//#	endif
#	ifndef _CRT_SECURE_NO_DEPRECATE
#		define _CRT_SECURE_NO_DEPRECATE
#	endif
//#	define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
//#	define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT 1
//#	define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 0
//-----------------------------------------------------------------------------------
// OK, so here it is again. The dreaded for scope. It is standard conformant - though it seems so
// But, well, sometimes it fails. So...
//-----------------------------------------------------------------------------------
//#	define for if(false) ; else for
//#	pragma warning(disable: 4127)
#endif
//-----------------------------------------------------------------------------------
#if (defined(_WIN32) || defined(WIN32))
#	define GW_OS_WINDOWS
#	define GW_PLATFORM_PC
#endif
//-----------------------------------------------------------------------------------
#if (defined(_CONSOLE) || defined(CONSOLE))
#	define GW_CONSOLE_APP
#endif
//-----------------------------------------------------------------------------------
#if (defined(_DEBUG) || defined(DEBUG))
#	define GW_DEBUG
#endif
//-----------------------------------------------------------------------------------
#if (defined(_MT) || defined(MT))
#	define GW_MULTITHREADER
#endif
//-----------------------------------------------------------------------------------
#if (defined(_UNICODE) || defined(UNICODE))
#	define GW_UNICODE
#endif
//-----------------------------------------------------------------------------------
#define GW_TOSTRING2(x) #x
#define GW_TOSTRING(x) GW_TOSTRING2(x)
//-----------------------------------------------------------------------------------
#define GW_WIDEN2(x) L ## x
#define GW_WIDEN(x) GW_WIDEN2(x)
//-----------------------------------------------------------------------------------
#define GW_CODE2TXT(x) _T(#x)
//-----------------------------------------------------------------------------------
#define GW_LINE_NUMBER __LINE__
//-----------------------------------------------------------------------------------
#ifdef GW_UNICODE
#	define GW_TOTSTRING GW_WIDEN
#else
#	define GW_TOTSTRING
#endif
//-----------------------------------------------------------------------------------
#	define GW_TFIlE__  GW_TOTSTRING(__FILE__)
#	define GW_LINE     GW_TOTSTRING(GW_TOSTRING(GW_LINE_NUMBER))
#	define GW_DATE     GW_TOTSTRING(__DATE__)
#	define GW_TIME     GW_TOTSTRING(__TIME__)
#	define GW_DATETIME GW_TOTSTRING(__TIMESTAMP__)
#	define GW_FUNCTION GW_TOTSTRING(__FUNCTION__)
//-----------------------------------------------------------------------------------
//#ifdef GW_COMPILER_MSVC6_OR_HIGHER
#	define GW_TFILE_LINE_STR          (GW_TFIlE__ _T("(") GW_LINE _T("): "))
#	define GW_TFILE_LINE_FUNCTION_STR (GW_TFIlE__ _T("(") GW_LINE _T("): ") GW_FUNCTION _T(" "))
//#endif
//-----------------------------------------------------------------------------------
//#define GW_NO_DEBUG_PRINT
//-----------------------------------------------------------------------------------
#ifdef GW_UNICODE
#	define gwOutputStringPrint OutputDebugStringW
#else
#	define gwOutputStringPrint OutputDebugStringA
#endif
//-----------------------------------------------------------------------------------
#define gwThrow1(x) { gwOutputStringPrint(GW_TFILE_LINE_STR); gwOutputStringPrint(x); gwOutputStringPrint(_T("\n")); }
#define gwThrow3(x, y) { gwThrow1(x); gwOutputStringPrint(y); gwOutputStringPrint(_T("\n")); }
#define gwThrow4(x) { gwThrow1(x); }
//-----------------------------------------------------------------------------------
#if defined(GW_NO_DEBUG_BREAK)
#	define gwThrow2(x, y) gwThrow3(x, y);
#	define gwThrow(x) gwThrow4(x);
#else
#	define gwThrow2(x, y) { gwThrow3(x, y); DebugBreak(); }
#	define gwThrow(x) { gwThrow4(x); DebugBreak(); }
#endif
//-----------------------------------------------------------------------------------
#define GW_DEFAULT ((uint32_t) -1)
#define GW_NOT_FOUND GW_DEFAULT
//-----------------------------------------------------------------------------------
#define GW_MINIMAL_STACK_STR_LEN						16
#define GW_MINI_STACK_STR_LEN							64
#define GW_DEFAULT_STACK_STR_LEN						256
#define GW_MAX_STACK_STR_LEN							GW_DEFAULT_STACK_STR_LEN * 3
#define GW_MAX_STACK_ERROR_STR_LEN						2048
#define GW_MAX_STACK_FORMAT_STR_LEN						4096
#define GW_MAX_STACK_FS_COUNT							16
//-----------------------------------------------------------------------------------
#define GAMEWARE_API
//-----------------------------------------------------------------------------------
#endif
//===================================================================================
// Copyright (C) GW-Labs, 2004-2015
//===================================================================================