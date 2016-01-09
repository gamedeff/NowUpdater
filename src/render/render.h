//
// NowUpdater
//
// render.h
//
// Copyright (c) 2016, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#ifndef RENDER_H
#define RENDER_H
//-----------------------------------------------------------------------------------
#include "nu_types.h"
#include "image.h"
//-----------------------------------------------------------------------------------
class Texture
{
public:

	uint32_t ID;

	uint32_t Width, Height;
	uint16_t BytesPerPixel;

	unsigned char *Data;
	uint32_t DataSize;

	bool is_render_target;

	BitmapImage image;
};
//-----------------------------------------------------------------------------------
class Render
{
public:

	Texture render_target;
	bool use_render_target;

	Render(bool use_render_target) : use_render_target(use_render_target)
	{
		memset(&render_target, 0, sizeof(render_target));
	}

	virtual ~Render() {}

	virtual bool Init() = 0;
	virtual bool InitDevice(HWND hWnd) = 0;
	virtual void Destroy() = 0;

	virtual bool CreateRenderTarget(uint32_t Width, uint32_t Height, uint16_t BytesPerPixel) = 0;

	virtual bool LoadImage(const void *data, uint32_t data_size, image_t &image) = 0;

	virtual bool BeginRender() = 0;
	virtual bool EndRender() = 0;

	virtual void NewFrame() = 0;
	virtual void RenderFrame() = 0;

	virtual LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
};
//-----------------------------------------------------------------------------------
static Render *g_render = 0;
//-----------------------------------------------------------------------------------
#endif

