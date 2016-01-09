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

	bool CreateRenderTarget(uint32_t Width, uint32_t Height, uint16_t BytesPerPixel);

	bool LoadImage(const void *data, uint32_t data_size, image_t &image);

	bool BeginRender();
	bool EndRender();

	void NewFrame();
	void RenderFrame();

	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
//-----------------------------------------------------------------------------------
#endif

