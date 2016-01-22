//===================================================================================
// GameWare Engine      
// Copyright (C) GW-Labs, 2004-2009
//===================================================================================
// Header name:         direct3d_common.h
// Header date:         13.9.2009 - 20:37
// Header author:       Fedor Gavrilov aka Dorfe
// Header description:  
//===================================================================================
#ifndef DIRECT3D_COMMON_H
#define DIRECT3D_COMMON_H
//-----------------------------------------------------------------------------------
#include "config.h"
#include "nu_types.h"
//-----------------------------------------------------------------------------------
#include <d3d9types.h>
//-----------------------------------------------------------------------------------
#include <dxerr.h>
//-----------------------------------------------------------------------------------
#pragma message("Linking with dxerr.lib...")
#pragma comment(lib, "dxerr.lib")
//-----------------------------------------------------------------------------------
//#include <dxerr9.h>
//-----------------------------------------------------------------------------------
//#pragma message("Linking with dxerr9.lib...")
//#pragma comment(lib, "dxerr9.lib")
//-----------------------------------------------------------------------------------
#define DXGetErrorString9      DXGetErrorString
#define DXGetErrorDescription9 DXGetErrorDescription
//-----------------------------------------------------------------------------------
static HRESULT Direct3DLastError = 0;
static char_t Direct3DLastErrorDescription[GW_MAX_STACK_ERROR_STR_LEN];
//-----------------------------------------------------------------------------------
inline char_t *Direct3D_GetErrorString(const char_t *ErrorFormat, const char_t *ErrorString, const char_t *ErrorDescription)
{
	_sntprintf(Direct3DLastErrorDescription, countof(Direct3DLastErrorDescription) - 1,
			   ErrorFormat, ErrorString, Direct3DLastError, ErrorDescription);

	return Direct3DLastErrorDescription;
}
//-----------------------------------------------------------------------------------
#define Direct3D9_GetErrorString()  Direct3D_GetErrorString(_T("Direct3D 9 error: \"%s (%d)\", error description: \"%s\"."),  DXGetErrorString9(Direct3DLastError), DXGetErrorDescription9(Direct3DLastError))
#define Direct3D10_GetErrorString() Direct3D_GetErrorString(_T("Direct3D 10 error: \"%s (%d)\", error description: \"%s\"."), DXGetErrorString(Direct3DLastError),  DXGetErrorDescription(Direct3DLastError))
//-----------------------------------------------------------------------------------
#define gwThrow_ErrorDirect3D9(x)         gwThrow2(x, Direct3D9_GetErrorString())
#define gwThrow_ErrorDirect3D9NoBreak(x)  gwThrow3(x, Direct3D9_GetErrorString())
#define gwThrow_ErrorDirect3D10(x)        gwThrow2(x, Direct3D10_GetErrorString())
//-----------------------------------------------------------------------------------
#define GW_D3D_CHECK_(x, error_msg, error_func, onerror) \
	/*gwOutputStringPrint(GW_TFILE_LINE_STR); \
	gwOutputStringPrint(_T(#x GW_TOTSTRING("\n")));*/ \
	if(FAILED(Direct3DLastError = (x))) \
	{ \
		error_func(error_msg); \
		onerror; \
	}
//-----------------------------------------------------------------------------------
#define GW_D3D9_CHECK_(x, error_msg, onerror)                 GW_D3D_CHECK_(x, error_msg, gwThrow_ErrorDirect3D9,        onerror)
#define GW_D3D9_CHECK_NO_DEBUG_BREAK_(x, error_msg, onerror)  GW_D3D_CHECK_(x, error_msg, gwThrow_ErrorDirect3D9NoBreak, onerror)
#define GW_D3D10_CHECK_(x, error_msg, onerror)                GW_D3D_CHECK_(x, error_msg, gwThrow_ErrorDirect3D10,       onerror)
//-----------------------------------------------------------------------------------
#define GW_D3D9_CHECK(x, error_msg)                 GW_D3D9_CHECK_(x, error_msg, return false)
#define GW_D3D9_CHECK_NO_DEBUG_BREAK(x, error_msg)  GW_D3D9_CHECK_NO_DEBUG_BREAK_(x, error_msg, return false)
#define GW_D3D10_CHECK(x, error_msg)                GW_D3D10_CHECK_(x, error_msg, return false)
//-----------------------------------------------------------------------------------
struct DepthStencilSize { unsigned char Depth, Stencil; };
//-----------------------------------------------------------------------------------
#endif
//===================================================================================
// Copyright (C) GW-Labs, 2004-2009
//===================================================================================