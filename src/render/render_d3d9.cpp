//
// NowUpdater
//
// render_d3d9.cpp
//
// Copyright (c) 2016, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#include "render_d3d9.h"
//-----------------------------------------------------------------------------------
#include "imgui.h"
#include "imgui_impl_dx9.h"
//-----------------------------------------------------------------------------------
extern LRESULT ImGui_ImplDX9_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//-----------------------------------------------------------------------------------
RenderD3D9::RenderD3D9(bool use_render_target) : Render(use_render_target), d3d(0), d3d_device(0)
{
	memset(&render_target_d3d9, 0, sizeof(render_target_d3d9));
}

bool RenderD3D9::Init()
{
	// Initialize Direct3D
	if((d3d = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
		return false;

	return true;
}

bool RenderD3D9::InitDevice(HWND hWnd)
{
	ZeroMemory(&d3d_pp, sizeof(d3d_pp));
	d3d_pp.Windowed = TRUE;
	d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3d_pp.BackBufferFormat = D3DFMT_X8R8G8B8; //D3DFMT_UNKNOWN;
	d3d_pp.EnableAutoDepthStencil = TRUE;
	d3d_pp.AutoDepthStencilFormat = D3DFMT_D16;
	d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	// Create the D3DDevice
	if(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING/*D3DCREATE_HARDWARE_VERTEXPROCESSING*/, &d3d_pp, &d3d_device) < 0)
	{
		d3d->Release();
		return false;
	}

	// Setup ImGui binding
	ImGui_ImplDX9_Init(hWnd, d3d_device);
	//ImGuiIO& io = ImGui::GetIO();
	//ImFont* my_font0 = io.Fonts->AddFontDefault();
	//ImFont* my_font1 = io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
	//ImFont* my_font2 = io.Fonts->AddFontFromFileTTF("../../extra_fonts/Karla-Regular.ttf", 16.0f);
	//ImFont* my_font3 = io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f); my_font3->DisplayOffset.y += 1;
	//ImFont* my_font4 = io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f); my_font4->DisplayOffset.y += 1;
	//ImFont* my_font5 = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, io.Fonts->GetGlyphRangesJapanese());
	/*ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize.x = w; //1920.0f;
	io.DisplaySize.y = h; //1280.0f;
	io.DeltaTime = 1.0f/60.0f;
	io.IniFilename = 0; //"imgui.ini";*/

	return true;
}

void RenderD3D9::Destroy()
{
	//ImGui_ImplDX9_Set(hWnd, d3d_device);
	ImGui_ImplDX9_Shutdown();

	if(render_target_d3d9.handle)
	{
		SAFE_RELEASE(render_target_d3d9.render_to_surface);
		SAFE_RELEASE(render_target_d3d9.surface_sysmem);
		SAFE_RELEASE(render_target_d3d9.surface);
		SAFE_RELEASE(render_target_d3d9.handle);
	}

	Render::Destroy();

	for(uint32_t i = 0; i < render_views.size(); ++i)
		render_views[i]->Destroy();

	for(uint32_t i = 0; i < render_views.size(); ++i)
		delete render_views[i];

	render_views.clear();

	SAFE_RELEASE(d3d_device);
	SAFE_RELEASE(d3d);
}

bool RenderD3D9::Reset(uint32_t Width, uint32_t Height)
{
	for(uint32_t i = 0; i < render_views.size(); ++i)
		render_views[i]->Destroy();

	d3d_pp.BackBufferWidth = Width;
	d3d_pp.BackBufferHeight = Height;

	HRESULT hr = d3d_device->Reset(&d3d_pp);
	if(hr == D3DERR_INVALIDCALL)
		return false;//IM_ASSERT(0);

	for(uint32_t i = 0; i < render_views.size(); ++i)
		if(!render_views[i]->Reset(i, Width, Height))
			return false;

	return true;
}

RenderView *RenderD3D9::CreateRenderView(HWND hWnd, uint32_t Width, uint32_t Height)
{
	RenderViewD3D9 *render_view = new RenderViewD3D9(this);

	if(!render_view->Init(render_views.size(), hWnd, Width, Height))
		return 0;

	render_views.push_back(render_view);
	return render_views.back();
}

bool RenderD3D9::CreateRenderTarget(uint32_t Width, uint32_t Height, uint16_t BytesPerPixel)
{
	render_target.Width = Width;
	render_target.Height = Height;
	render_target.BytesPerPixel = BytesPerPixel;
	render_target.is_render_target = true;

	if(render_target.is_render_target)
	{
		GW_D3D9_CHECK(D3DXCreateTexture(d3d_device, render_target.Width, render_target.Height, 1, D3DUSAGE_RENDERTARGET,
										d3d_pp.BackBufferFormat, D3DPOOL_DEFAULT, &render_target_d3d9.handle),
						_T("Texture creating error: can't create render target texture, D3DXCreateTexture failed"));

		GW_D3D9_CHECK(render_target_d3d9.handle->GetSurfaceLevel(0, &render_target_d3d9.surface),
						_T("Texture creating error: can't get texture surface, IDirect3DTexture9::GetSurfaceLevel failed"));

		D3DSURFACE_DESC desc;

		GW_D3D9_CHECK(render_target_d3d9.surface->GetDesc(&desc),
						_T("Texture creating error: can't get texture surface description, IDirect3DSurface9::GetDesc failed"));

		GW_D3D9_CHECK(D3DXCreateRenderToSurface(d3d_device, desc.Width, desc.Height, desc.Format, TRUE, /*D3DFMT_D24X8*/d3d_pp.AutoDepthStencilFormat, &render_target_d3d9.render_to_surface),
						_T("Texture creating error: can't create render to surface interface, D3DXCreateRenderToSurface failed"));

		GW_D3D9_CHECK(d3d_device->CreateOffscreenPlainSurface(render_target.Width, render_target.Height, d3d_pp.BackBufferFormat, D3DPOOL_SYSTEMMEM, &render_target_d3d9.surface_sysmem, NULL),
						_T("Texture creating error: can't create offscreen surface, IDirect3DDevice9::CreateOffscreenPlainSurface failed"));

		render_target_d3d9.render_to_surface->BeginScene(render_target_d3d9.surface, NULL);
		d3d_device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00FFFFFF, 1.0f, 0);
		render_target_d3d9.render_to_surface->EndScene(NULL);
	}

	return true;
}

bool RenderD3D9::LoadImage(const void *data, uint32_t data_size, image_t &image)
{
	LPDIRECT3DTEXTURE9 pTexture = NULL;
	D3DXIMAGE_INFO image_info;
	GW_D3D9_CHECK(D3DXGetImageInfoFromFileInMemory(data, data_size, &image_info), _T("Failed to load graphics file: INFO BUFFER"));

	//result = D3DXCreateTextureFromFileInMemoryEx(
	//	g_pd3dDevice,
	//	(LPCVOID) sites[current_site].titles[current_title_index].cover_texture_data.c_str(), //&graphic_buffer[0],
	//	sites[current_site].titles[current_title_index].cover_texture_data.size(),
	//	image_info.Width,
	//	image_info.Height,
	//	image_info.MipLevels,                     //mip-map levels (1 for no chain)
	//	D3DPOOL_DEFAULT,       //the type of surface (standard)
	//	image_info.Format/*D3DFMT_UNKNOWN*/,        //surface format (default)
	//	D3DPOOL_DEFAULT,       //memory class for the texture
	//	D3DX_DEFAULT,          //image filter
	//	D3DX_DEFAULT,          //mip filter
	//	0,					   //color key for transparency
	//	&image_info,           //bitmap file info (from loaded file)
	//	NULL,                  //color palette
	//	&pTexture);            //destination texture

	image.w = image_info.Width;
	image.h = image_info.Height;

	GW_D3D9_CHECK(D3DXCreateTextureFromFileInMemory(d3d_device, data, data_size, (LPDIRECT3DTEXTURE9 *) &pTexture), _T("Failed to load graphics file: FROM BUFFER"));

	image.handle = (void *)pTexture;

	return true;
}

bool RenderD3D9::BeginRender()
{
	if(use_render_target && render_target_d3d9.render_to_surface) //if(render_target_d3d9.render_to_surface) //if(render->render_target)
	{
		//Direct3D9_SetViewport(render, 0, 0, render->render_target->Width, render->render_target->Height);

		GW_D3D9_CHECK(render_target_d3d9.render_to_surface->BeginScene(render_target_d3d9.surface, NULL),
						_T("ID3DXRenderToSurface::BeginScene() failed - second call before End()?.."));
	}
	else
	{
		GW_D3D9_CHECK(d3d_device->BeginScene(),
						_T("IDirect3DDevice9::BeginScene() failed - second call before End()?.."));
	}

	//ImVec4 clear_col = ImColor(114, 144, 154);

	d3d_device->SetRenderState(D3DRS_ZENABLE, false);
	d3d_device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	d3d_device->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
	//D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_col.x*255.0f), (int)(clear_col.y*255.0f), (int)(clear_col.z*255.0f), (int)(clear_col.w*255.0f));
	//d3d_device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);

	return true;
}

bool RenderD3D9::EndRender()
{
	if(use_render_target && render_target_d3d9.render_to_surface) //if(render_target_d3d9.render_to_surface) //if(render->render_target)
	{
		GW_D3D9_CHECK(render_target_d3d9.render_to_surface->EndScene(NULL),
						_T("ID3DXRenderToSurface::EndScene() failed - Begin() called twice?.."));

		GW_D3D9_CHECK(d3d_device->GetRenderTargetData(render_target_d3d9.surface, render_target_d3d9.surface_sysmem),
						_T("Can't get render target data, IDirect3DDevice9::GetRenderTargetData failed"));

		//render->window->Redraw();
	}
	else
	{
		GW_D3D9_CHECK(d3d_device->EndScene(),
						_T("IDirect3DDevice9::EndScene() failed - Begin() called twice?.."));
	}

	return true;
}

bool RenderD3D9::Present()
{
	GW_D3D9_CHECK_NO_DEBUG_BREAK(d3d_device->Present(NULL, NULL, NULL, NULL),
									_T("IDirect3DDevice9::Present() failed."));

	return true;
}


LRESULT WINAPI RenderD3D9::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(ImGui_ImplDX9_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch(msg)
	{
		case WM_SIZE:

			if(d3d_device != NULL && wParam != SIZE_MINIMIZED)
			{
				ImGui_ImplDX9_Set(hWnd, d3d_device);
				ImGui_ImplDX9_InvalidateDeviceObjects();

				Reset(LOWORD(lParam), HIWORD(lParam));

				ImGui_ImplDX9_CreateDeviceObjects();
			}
			return 0;

		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return 0;
			break;
#if 0
		case WM_PRINTCLIENT:
			{
				////if(userinfo.show_title_popup)
				//{
				//	ImGui_ImplDX9_NewFrame();
				//	Render();
				//}
				//RedrawWindow(hWnd, NULL, NULL, RDW_UPDATENOW);
				//HDC hdc = (HDC) wParam;
				//HDC TargetDC = GetDC(hWnd);

				//RECT rect;
				//GetWindowRect(hWnd, &rect);

				//BitBlt(hdc,0,0,rect.right-rect.left,rect.bottom-rect.top,TargetDC,0,0,SRCCOPY);
				RECT client_rect = {0};
				::GetClientRect(GetNativeView(), &client_rect);
				HDC dest_dc = reinterpret_cast<HDC>(w_param);
				DCHECK(dest_dc);
				HDC src_dc = ::GetDC(GetNativeView());
				::BitBlt(dest_dc, 0, 0, client_rect.right - client_rect.left,
						 client_rect.bottom - client_rect.top, src_dc, 0, 0,
						 SRCCOPY);
				::ReleaseDC(GetNativeView(), src_dc);
			}

			break;
#endif
		case WM_PAINT:
		{
			//Window *window = GetWindowByHandle(hWnd);

			//if(window != NULL && window->OnPaint)
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hWnd, &ps);

				//window->OnPaint(window);
				//if(userinfo.show_title_popup)
				{
/*					NewFrame();
					RenderFrame();
					//window.render_view->Present();*/
				}

				if(use_render_target && render_target_d3d9.render_to_surface) //if(render_target_d3d9.surface_sysmem) //if(window->render && window->render->render_target)
				{
					//HBITMAP hBitmap = NULL;

					HDC hRenderTargetDC = NULL; // = window->render->GetRenderTargetDC();
					GW_D3D9_CHECK(render_target_d3d9.surface_sysmem->GetDC(&hRenderTargetDC),
									_T("Can't get render target DC, IDirect3DSurface9::GetDC failed"));

					BitmapImage rt;

					if(!hRenderTargetDC)
					{
/*						// create a DC for our bitmap
						hRenderTargetDC = CreateCompatibleDC(hdc);

						BITMAPINFO bmi;

						// zero the memory for the bitmap info
						ZeroMemory(&bmi, sizeof(BITMAPINFO));

						// setup bitmap info 
						bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
						bmi.bmiHeader.biWidth       = render_target.Width;
						bmi.bmiHeader.biHeight      = render_target.Height;
						bmi.bmiHeader.biPlanes      = 1;
						bmi.bmiHeader.biBitCount    = render_target.BytesPerPixel * 8; // four 8-bit components
						bmi.bmiHeader.biCompression = BI_RGB;
						bmi.bmiHeader.biSizeImage   = render_target.Width  * 
													  render_target.Height * 
													  render_target.BytesPerPixel;

						// create our DIB section and select the bitmap into the dc
						void *bits;
						hBitmap = CreateDIBSection(hRenderTargetDC, &bmi, DIB_RGB_COLORS, &bits, NULL, 0x0);
*/
						Image_Create(&rt, hdc, render_target.Width, render_target.Height, render_target.BytesPerPixel * 8);

						SelectObject(rt.hdc, rt.hBitmap);

						memcpy(rt.pPixels, render_target.Data, render_target.DataSize);

						//SetDIBitsToDevice(hdc, 0, 0, render_target.Width, render_target.Height, 0, 0, 0, render_target.Height, bits, &bmi, DIB_RGB_COLORS);

						//BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

						//POINT p = { 0 };
						//SIZE  s = { render_target.Width, render_target.Height };

						//BOOL res = UpdateLayeredWindow(hWnd, windows_data[window->Index].hDC, 0, &s, hRenderTargetDC, &p, RGB(0, 0, 0), &blend, /*LWA_COLORKEY | */ULW_ALPHA);
					}
					else
					{
						rt.hdc = hRenderTargetDC;
					}

					BitBlt(hdc, 0, 0, render_target.Width, render_target.Height, rt.hdc, 0, 0, SRCCOPY);

					// do cleanup
					if(!hRenderTargetDC)
					{
						Image_Destroy(&rt);
					}
					else
					{
						//window->render->ReleaseRenderTargetDC(hRenderTargetDC);
						GW_D3D9_CHECK(render_target_d3d9.surface_sysmem->ReleaseDC(hRenderTargetDC),
										_T("Can't release render target DC, IDirect3DSurface9::ReleaseDC failed"));
					}
				}

				EndPaint(hWnd, &ps);
			}

			break;
		}

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void RenderD3D9::NewFrame()
{
	//ImGui_ImplDX9_Set(hWnd, d3d_device);
	ImGui_ImplDX9_NewFrame();
}

void RenderD3D9::RenderFrame(RenderView *render_view)
{
	render_view ? render_view->BeginRender() : BeginRender();
	{
		//ImGui_ImplDX9_Set(hWnd, d3d_device);

		ImGui::Render();
	}
	render_view ? render_view->EndRender() : EndRender();

	render_view ? render_view->Present() : Present();
}
//-----------------------------------------------------------------------------------
RenderViewD3D9::RenderViewD3D9(RenderD3D9 *renderer) : RenderView(renderer), renderer(renderer), d3d_swap_chain(0), d3d_swap_chain_back_buffer(0)
{
}

bool RenderViewD3D9::Init(uint32_t i, HWND hWnd, uint32_t Width, uint32_t Height)
{
	ZeroMemory(&d3d_pp, sizeof(d3d_pp));
	d3d_pp.hDeviceWindow = hWnd;
	d3d_pp.Windowed = TRUE;
	d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3d_pp.BackBufferFormat = D3DFMT_X8R8G8B8; //D3DFMT_UNKNOWN;
	d3d_pp.BackBufferWidth = Width;
	d3d_pp.BackBufferHeight = Height;
	d3d_pp.EnableAutoDepthStencil = TRUE;
	d3d_pp.AutoDepthStencilFormat = D3DFMT_D16;
	d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	if(i > 0)
	{
		GW_D3D9_CHECK(renderer->d3d_device->CreateAdditionalSwapChain(&d3d_pp, &d3d_swap_chain), _T("Failed to CreateAdditionalSwapChain"));
	}
	else
	{
		GW_D3D9_CHECK(renderer->d3d_device->GetSwapChain(0, &d3d_swap_chain), _T("Failed to GetSwapChain"));
	}

	return true;
}

void RenderViewD3D9::Destroy()
{
	SAFE_RELEASE(d3d_swap_chain_back_buffer);
	SAFE_RELEASE(d3d_swap_chain);
}

bool RenderViewD3D9::BeginRender()
{
	// Tell the Direct3D device to render to the swap chain’s back buffer
	GW_D3D9_CHECK(d3d_swap_chain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &d3d_swap_chain_back_buffer),
								_T("IDirect3DSwapChain9::GetBackBuffer() failed."));
	GW_D3D9_CHECK(renderer->d3d_device->SetRenderTarget(0, d3d_swap_chain_back_buffer),
								_T("IDirect3DDevice9::SetRenderTarget() failed."));

	D3DCOLOR clrBlack = D3DCOLOR_ARGB(0xFF, 0x00, 0x00, 0x00);

	// Clear the target buffer...
	GW_D3D9_CHECK(renderer->d3d_device->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		clrBlack, 1.0f, 0 ),
		_T("IDirect3DDevice9::Clear() failed."));

	return renderer->BeginRender();
}

bool RenderViewD3D9::EndRender()
{
	return renderer->EndRender();
}

bool RenderViewD3D9::Present()
{
	GW_D3D9_CHECK_NO_DEBUG_BREAK(d3d_swap_chain->Present(NULL, NULL, d3d_pp.hDeviceWindow, NULL, 0),
						_T("IDirect3DSwapChain9::Present() failed."));

	SAFE_RELEASE(d3d_swap_chain_back_buffer);

	return true;
}

bool RenderViewD3D9::Reset(uint32_t i, uint32_t Width, uint32_t Height)
{
	return Init(i, d3d_pp.hDeviceWindow, Width, Height);
}
//-----------------------------------------------------------------------------------
