//===================================================================================
// GameWare Engine      
// Copyright (C) GW-Labs, 2004-2010
//===================================================================================
// Header name:         direct3d9_data.h
// Header date:         23.12.2010 - 21:18
// Header author:       Fedor Gavrilov aka Dorfe
// Header description:  
//===================================================================================
#ifndef DIRECT3D9_DATA_H
#define DIRECT3D9_DATA_H
//-----------------------------------------------------------------------------------
#include "direct3d9_headers.h"
//-----------------------------------------------------------------------------------
#ifdef GW_DEBUG_VS
#	define GW_D3DXSHADER_FLAGS_VS D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT
#else
#	define GW_D3DXSHADER_FLAGS_VS 0
#endif
//-----------------------------------------------------------------------------------
#ifdef GW_DEBUG_PS
#	define GW_D3DXSHADER_FLAGS_PS D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT
#else
#	define GW_D3DXSHADER_FLAGS_PS 0
#endif
//-----------------------------------------------------------------------------------
#if defined(GW_DEBUG_VS) || defined(GW_DEBUG_PS)
#	define GW_D3DXSHADER_FLAGS_OPTIMIZATION D3DXSHADER_SKIPOPTIMIZATION
#else
#	define GW_D3DXSHADER_FLAGS_OPTIMIZATION 0
#endif
//-----------------------------------------------------------------------------------
#if defined(GW_DEBUG_VS) || defined(GW_DEBUG_PS)
#	define GW_D3DXSHADER_FLAGS_VALIDATION D3DXSHADER_SKIPVALIDATION
#else
#	define GW_D3DXSHADER_FLAGS_VALIDATION 0
#endif
//-----------------------------------------------------------------------------------
#ifdef GW_DEBUG_D3D
#	define GW_D3DXSHADER_FLAGS_DEBUG D3DXSHADER_DEBUG
#else
#	define GW_D3DXSHADER_FLAGS_DEBUG 0
#endif
//-----------------------------------------------------------------------------------
#define GW_D3DXSHADER_ASSEMBLER_FLAGS GW_D3DXSHADER_FLAGS_DEBUG | GW_D3DXSHADER_FLAGS_VALIDATION
#define GW_D3DXSHADER_FLAGS GW_D3DXSHADER_FLAGS_DEBUG | GW_D3DXSHADER_FLAGS_OPTIMIZATION //| GW_D3DXSHADER_FLAGS_VS | GW_D3DXSHADER_FLAGS_PS
//-----------------------------------------------------------------------------------
const D3DMULTISAMPLE_TYPE GW_D3D9_MULTISAMPLETYPES_TABLE[] = 
{
	D3DMULTISAMPLE_NONE,
	D3DMULTISAMPLE_NONMASKABLE,
	D3DMULTISAMPLE_2_SAMPLES,
	D3DMULTISAMPLE_3_SAMPLES,
	D3DMULTISAMPLE_4_SAMPLES,
	D3DMULTISAMPLE_5_SAMPLES,
	D3DMULTISAMPLE_6_SAMPLES,
	D3DMULTISAMPLE_7_SAMPLES,
	D3DMULTISAMPLE_8_SAMPLES,
	D3DMULTISAMPLE_9_SAMPLES,
	D3DMULTISAMPLE_10_SAMPLES,
	D3DMULTISAMPLE_11_SAMPLES,
	D3DMULTISAMPLE_12_SAMPLES,
	D3DMULTISAMPLE_13_SAMPLES,
	D3DMULTISAMPLE_14_SAMPLES,
	D3DMULTISAMPLE_15_SAMPLES,
	D3DMULTISAMPLE_16_SAMPLES
};
const BYTE GW_D3D9_VERTEX_DECLARATION_TYPES_TABLE[] = 
{
	D3DDECLTYPE_FLOAT1,    // 1D float expanded to (value, 0., 0., 1.)
	D3DDECLTYPE_FLOAT2,    // 2D float expanded to (value, value, 0., 1.)
	D3DDECLTYPE_FLOAT3,    // 3D float expanded to (value, value, value, 1.)
	D3DDECLTYPE_FLOAT4,    // 4D float
	D3DDECLTYPE_D3DCOLOR,  // 4D packed unsigned bytes mapped to 0. to 1. range

	// Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)

	D3DDECLTYPE_UBYTE4,    // 4D unsigned byte
	D3DDECLTYPE_SHORT2,    // 2D signed short expanded to (value, value, 0., 1.)
	D3DDECLTYPE_SHORT4,    // 4D signed short

	// The following types are valid only with vertex shaders >= 2.0

	D3DDECLTYPE_UBYTE4N,   // Each of 4 bytes is normalized by dividing to 255.0
	D3DDECLTYPE_SHORT2N,   // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
	D3DDECLTYPE_SHORT4N,   // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
	D3DDECLTYPE_USHORT2N,  // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
	D3DDECLTYPE_USHORT4N,  // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
	D3DDECLTYPE_UDEC3,     // 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
	D3DDECLTYPE_DEC3N,     // 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
	D3DDECLTYPE_FLOAT16_2, // Two 16-bit floating point values, expanded to (value, value, 0, 1)
	D3DDECLTYPE_FLOAT16_4, // Four 16-bit floating point values
	D3DDECLTYPE_UNUSED,    // When the type field in a decl is unused.
};
const BYTE GW_D3D9_VERTEX_DECLARATION_USAGE_TABLE[] = 
{
	D3DDECLUSAGE_POSITION,
	D3DDECLUSAGE_BLENDWEIGHT,   // 1
	D3DDECLUSAGE_BLENDINDICES,  // 2
	D3DDECLUSAGE_NORMAL,        // 3
	D3DDECLUSAGE_PSIZE,         // 4
	D3DDECLUSAGE_TEXCOORD,      // 5
	D3DDECLUSAGE_TANGENT,       // 6
	D3DDECLUSAGE_BINORMAL,      // 7
	D3DDECLUSAGE_TESSFACTOR,    // 8
	D3DDECLUSAGE_POSITIONT,     // 9
	D3DDECLUSAGE_COLOR,         // 10
	D3DDECLUSAGE_FOG,           // 11
	D3DDECLUSAGE_DEPTH,         // 12
	D3DDECLUSAGE_SAMPLE,        // 13
};
const BYTE GW_D3D9_VERTEX_DECLARATION_USAGE_NUM_TABLE[] = 
{
	3,
	0,   // 1
	0,  // 2
	3,        // 3
	0,         // 4
	2,      // 5
	0,       // 6
	0,      // 7
	0,    // 8
	3,     // 9
	0,         // 10
	0,           // 11
	0,         // 12
	0,        // 13
};
//-----------------------------------------------------------------------------------
struct Direct3D9TextureData
{
	IDirect3DTexture9 *handle;

	IDirect3DSurface9 *surface;
	IDirect3DSurface9 *surface_sysmem;

	ID3DXRenderToSurface *render_to_surface;

	DWORD   usage;
	D3DPOOL pool;
};
//-----------------------------------------------------------------------------------
struct Direct3D9ShaderData
{
	IDirect3DVertexShader9 *vertex_shader_handle;
	IDirect3DPixelShader9  *pixel_shader_handle;

	const char *profile;

	ID3DXBuffer        *compiled_code;
	ID3DXConstantTable *constant_table;
};
//-----------------------------------------------------------------------------------
struct Direct3D9StreamParamData
{
	D3DVERTEXELEMENT9 decl;
	unsigned int size;

	IDirect3DVertexBuffer9 *handle;
};
//-----------------------------------------------------------------------------------
struct Direct3D9VertexBufferData
{
	D3DFORMAT format;

	IDirect3DVertexBuffer9 *handle;
};
//-----------------------------------------------------------------------------------
struct Direct3D9IndexBufferData
{
	D3DFORMAT format;
	unsigned int size, num;

	IDirect3DIndexBuffer9 *handle;
};
//-----------------------------------------------------------------------------------
/*struct Direct3D9EffectParamData
{
	D3DXHANDLE index;
	Shader::ShaderType shader_type;
	unsigned int hash;
};
//-----------------------------------------------------------------------------------
struct Direct3D9EffectData
{
	IDirect3DVertexDeclaration9 *handle;

	Direct3D9ShaderData *ps;
	Direct3D9ShaderData *vs;

	Direct3D9StreamParamData cached_attrib_index[GW_MAX_EFFECT_ATTRIBS_NUM];
	unsigned int cached_attrib_index_num;

	unsigned int attrib_hash[countof(GW_D3D9_VERTEX_DECLARATION_USAGE_TABLE)];

	Direct3D9EffectParamData cached_uniform_index[GW_MAX_EFFECT_UNIFORMS_NUM];
	unsigned int cached_uniform_index_num;
};
//-----------------------------------------------------------------------------------
struct Direct3D9FontData
{
	ID3DXFont *handle;

	ID3DXSprite *sprite_handle;
};
//-----------------------------------------------------------------------------------
struct Direct3D9RenderData
{
	IDirect3D9       *d3d;
	IDirect3DDevice9 *d3d_device;

	D3DPRESENT_PARAMETERS  present_params;
	D3DCAPS9               device_caps;
	D3DDEVTYPE             device_driver_type;

	UINT adapter;

	DWORD behavior_flags;
	D3DFORMAT texture_format;

	Direct3D9TextureData textures[GW_MAX_TEXTURES_NUM];
	unsigned int textures_num, textures_index;

	Direct3D9ShaderData shaders[GW_MAX_SHADERS_NUM];
	unsigned int shaders_num, shaders_index;

	Direct3D9EffectData effects[GW_MAX_EFFECTS_NUM];
	unsigned int effects_num, effects_index;

	Direct3D9FontData fonts[GW_MAX_FONTS_NUM];
	unsigned int fonts_num, fonts_index;


	Direct3D9VertexBufferData vertex_buffers[GW_MAX_VERTEX_BUFFERS_NUM];
	unsigned int vertex_buffers_num, vertex_buffers_index;

	Direct3D9IndexBufferData index_buffers[GW_MAX_INDEX_BUFFERS_NUM];
	unsigned int index_buffers_num, index_buffers_index;
};
//-----------------------------------------------------------------------------------
extern Direct3D9RenderData direct3d9_render_data[GW_MAX_WINDOWS_NUM];
extern unsigned int direct3d9_renders_num;*/
//-----------------------------------------------------------------------------------
#endif
//===================================================================================
// Copyright (C) GW-Labs, 2004-2010
//===================================================================================