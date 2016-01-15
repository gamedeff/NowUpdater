//
// NowUpdater
//
// render_d3d9.h
//
// Copyright (c) 2016, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#ifndef RENDER_D3D9_H
#define RENDER_D3D9_H
//-----------------------------------------------------------------------------------
#include "render.h"
//-----------------------------------------------------------------------------------
#include "direct3d_common.h"
#include "direct3d9_headers.h"
#include "direct3d9_data.h"
//-----------------------------------------------------------------------------------
class RenderD3D9;
//-----------------------------------------------------------------------------------
class RenderViewD3D9 : public RenderView
{
public:

	D3DPRESENT_PARAMETERS d3d_pp;
	LPDIRECT3DSWAPCHAIN9  d3d_swap_chain;
	LPDIRECT3DSURFACE9    d3d_swap_chain_back_buffer;

	RenderD3D9 *renderer;

	RenderViewD3D9(RenderD3D9 *renderer); 

	bool Init(uint32_t i, HWND hWnd, uint32_t Width = 0, uint32_t Height = 0);
	void Destroy();

	bool BeginRender();
	bool EndRender();

	bool Present();

	bool Reset(uint32_t i, uint32_t Width, uint32_t Height);
};
//-----------------------------------------------------------------------------------
class RenderD3D9 : public Render
{
public:

	LPDIRECT3D9           d3d;
	LPDIRECT3DDEVICE9     d3d_device;
	D3DPRESENT_PARAMETERS d3d_pp;

	Direct3D9TextureData render_target_d3d9;

	RenderD3D9(bool use_render_target);

	bool Init();
	bool InitDevice(HWND hWnd);
	void Destroy();

	RenderView *CreateRenderView(HWND hWnd, uint32_t Width = 0, uint32_t Height = 0);

	bool CreateRenderTarget(uint32_t Width, uint32_t Height, uint16_t BytesPerPixel);

	bool LoadImage(const void *data, uint32_t data_size, image_t &image);

	bool BeginRender();
	bool EndRender();

	bool Present();

	bool Reset(uint32_t Width, uint32_t Height);

	void NewFrame();
	void RenderFrame(RenderView *render_view);

	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
//-----------------------------------------------------------------------------------
#endif

