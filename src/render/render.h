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
#include "for_each.h"
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
class Render;
//-----------------------------------------------------------------------------------
class RenderView
{
public:

	uint32_t n;

	RenderView(Render *renderer) : n(0) {}

	virtual ~RenderView() {}

	virtual bool Init(uint32_t i, HWND hWnd, uint32_t Width = 0, uint32_t Height = 0) = 0;
	virtual void Destroy() = 0;

	virtual bool InitUI() = 0;
	virtual void DestroyUI() = 0;

	virtual bool BeginRender() = 0;
	virtual bool EndRender() = 0;

	virtual bool Present() = 0;

	virtual bool Reset(uint32_t Width, uint32_t Height) = 0;
};
//-----------------------------------------------------------------------------------
class Render
{
public:

	Texture render_target;
	bool use_render_target;

	std::map<HWND, RenderView *> render_views;

	typedef std::pair<const HWND, RenderView *> RenderViewByHandle;

	Render(bool use_render_target) : use_render_target(use_render_target)
	{
		memset(&render_target, 0, sizeof(render_target));
	}

	virtual ~Render() {}

	virtual bool Init() = 0;
	virtual bool InitDevice(HWND hWnd) = 0;
	virtual void Destroy()
	{
		FOR_EACH(RenderViewByHandle &render_view_by_handle, render_views)
		{
			RenderView *render_view = render_view_by_handle.second;

			render_view->Destroy();
			delete render_view;
		}

		render_views.clear();
	}

	virtual RenderView *CreateRenderView(HWND hWnd, uint32_t Width = 0, uint32_t Height = 0) = 0;
	virtual void DestroyRenderView(HWND hWnd)
	{
		RenderView *render_view = render_views[hWnd];
		if(render_view)
		{
			render_view->Destroy();
			delete render_view;

			render_views.erase(hWnd);
		}
	}

	virtual bool CreateRenderTarget(uint32_t Width, uint32_t Height, uint16_t BytesPerPixel) = 0;

	virtual bool LoadImage(const void *data, uint32_t data_size, image_t &image) = 0;

	virtual bool BeginRender() = 0;
	virtual bool EndRender() = 0;

	virtual bool Present() = 0;

	virtual bool Reset(uint32_t Width, uint32_t Height) = 0;

	virtual void NewFrame(HWND hWnd) = 0;
	virtual void RenderFrame(RenderView *render_view) = 0;

	virtual LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
};
//-----------------------------------------------------------------------------------
//static Render *g_render = 0;
//-----------------------------------------------------------------------------------
#endif

