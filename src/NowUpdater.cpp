//
// NowUpdater.cpp
//
// Copyright (c) 2015, Fedor Gavrilov
// and Contributors.
//
//===================================================================================
#include "NowUpdater.h"

#include "nu_user_info.h"

using Poco::Timer;
using Poco::TimerCallback;

#include "nu_options.h"

static options_t options;

#include "imgui.h"
#include "imgui_impl_dx9.h"

#include "image.h"
#include "util.h"

#include "direct3d_common.h"
#include "direct3d9_headers.h"
#include "direct3d9_data.h"

static Direct3D9TextureData render_target_d3d9 = { 0 };

class Texture
{
public:

	unsigned int ID;

	unsigned int   Width, Height;
	unsigned short BytesPerPixel;

	unsigned char *Data;
	unsigned int DataSize;

	bool is_render_target;

	BitmapImage image;
};

static Texture render_target = { 0 };

// Data
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp;

extern LRESULT ImGui_ImplDX9_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool ImplDX9_LoadImage(const void *data, uint32_t data_size, title_image_t &image)
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

	GW_D3D9_CHECK(D3DXCreateTextureFromFileInMemory(g_pd3dDevice, data, data_size, (LPDIRECT3DTEXTURE9 *) &pTexture), _T("Failed to load graphics file: FROM BUFFER"));

	image.handle = (void *)pTexture;

	return true;
}

bool Render()
{
	if(options.no_native_windows && render_target_d3d9.render_to_surface) //if(render_target_d3d9.render_to_surface) //if(render->render_target)
	{
		//Direct3D9_SetViewport(render, 0, 0, render->render_target->Width, render->render_target->Height);

		GW_D3D9_CHECK(render_target_d3d9.render_to_surface->BeginScene(render_target_d3d9.surface, NULL),
						_T("ID3DXRenderToSurface::BeginScene() failed - second call before End()?.."));
	}
	else
	{
		GW_D3D9_CHECK(g_pd3dDevice->BeginScene(),
						_T("IDirect3DDevice9::BeginScene() failed - second call before End()?.."));
	}

	ImVec4 clear_col = ImColor(114, 144, 154);

	g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, false);
	g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
	D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_col.x*255.0f), (int)(clear_col.y*255.0f), (int)(clear_col.z*255.0f), (int)(clear_col.w*255.0f));
	//g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);

	ImGui::Render();

	if(options.no_native_windows && render_target_d3d9.render_to_surface) //if(render_target_d3d9.render_to_surface) //if(render->render_target)
	{
		GW_D3D9_CHECK(render_target_d3d9.render_to_surface->EndScene(NULL),
						_T("ID3DXRenderToSurface::EndScene() failed - Begin() called twice?.."));

		GW_D3D9_CHECK(g_pd3dDevice->GetRenderTargetData(render_target_d3d9.surface, render_target_d3d9.surface_sysmem),
						_T("Can't get render target data, IDirect3DDevice9::GetRenderTargetData failed"));

		//render->window->Redraw();
	}
	else
	{
		GW_D3D9_CHECK(g_pd3dDevice->EndScene(),
						_T("IDirect3DDevice9::EndScene() failed - Begin() called twice?.."));

		GW_D3D9_CHECK_NO_DEBUG_BREAK(g_pd3dDevice->Present(NULL, NULL, NULL, NULL),
										_T("IDirect3DDevice9::Present() failed."));
	}

	return true;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(ImGui_ImplDX9_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch(msg)
	{
		case WM_SIZE:
			if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
			{
				ImGui_ImplDX9_InvalidateDeviceObjects();
				g_d3dpp.BackBufferWidth  = LOWORD(lParam);
				g_d3dpp.BackBufferHeight = HIWORD(lParam);
				HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
				if (hr == D3DERR_INVALIDCALL)
					IM_ASSERT(0);
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
					ImGui_ImplDX9_NewFrame();
					Render();
				}

				if(options.no_native_windows && render_target_d3d9.render_to_surface) //if(render_target_d3d9.surface_sysmem) //if(window->render && window->render->render_target)
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

int main(int argc, char** argv)
{
	//if (argc != 2)
	//{
	//	Path p(argv[0]);
	//	std::cout << "usage: " << p.getBaseName() << " <uri>" << std::endl;
	//	std::cout << "       fetches the resource identified by <uri> and print it to the standard output" << std::endl;
	//	return 1;
	//}

	options.app_name = "NowUpdater";

	user_info_t userinfo = user_info_t("nowupdater", "nowupdater2015", &options);

	if(!userinfo.init())
		return 1;

	// Initialize Direct3D
	LPDIRECT3D9 pD3D;
	if((pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
	{
		//UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 0;
	}

	TCHAR NU_WNDCLASS_NAME[] = _T("Now Updater");
	TCHAR NU_APP_TITLE[] = _T("Now Updater");

	std::string window_title = userinfo.username + "'s anime list";

	uint32_t x = options.x, y = options.y, w = 800, h = 600;
	uint32_t popup_w = 400, popup_h = 300;

	// Create application window
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL, NU_WNDCLASS_NAME, NULL };
	RegisterClassEx(&wc);
	HWND hWnd = CreateWindow(NU_WNDCLASS_NAME, options.no_native_windows ? NU_APP_TITLE : GW_A2T(window_title.c_str()), options.no_native_windows ? WS_POPUP : WS_OVERLAPPEDWINDOW, x, y, w, h, NULL, NULL, wc.hInstance, NULL);

	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8; //D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	// Create the D3DDevice
	if (pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
	{
		pD3D->Release();
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 0;
	}

	// Setup ImGui binding
	ImGui_ImplDX9_Init(hWnd, g_pd3dDevice);
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

	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);

	if(options.no_native_windows)
	{
		RECT client_rect = { 0 };
		GetClientRect(hWnd, &client_rect);

		uint32_t ClientWidth  = client_rect.right - client_rect.left;
		uint32_t ClientHeight = client_rect.bottom - client_rect.top;

		render_target.Width = ClientWidth;
		render_target.Height = ClientHeight;
		render_target.BytesPerPixel = 4;
		render_target.is_render_target = true;

		if(render_target.is_render_target)
		{
			GW_D3D9_CHECK(D3DXCreateTexture(g_pd3dDevice, render_target.Width, render_target.Height, 1, D3DUSAGE_RENDERTARGET,
											g_d3dpp.BackBufferFormat, D3DPOOL_DEFAULT, &render_target_d3d9.handle),
							_T("Texture creating error: can't create render target texture, D3DXCreateTexture failed"));

			GW_D3D9_CHECK(render_target_d3d9.handle->GetSurfaceLevel(0, &render_target_d3d9.surface),
							_T("Texture creating error: can't get texture surface, IDirect3DTexture9::GetSurfaceLevel failed"));

			D3DSURFACE_DESC desc;

			GW_D3D9_CHECK(render_target_d3d9.surface->GetDesc(&desc),
							_T("Texture creating error: can't get texture surface description, IDirect3DSurface9::GetDesc failed"));

			GW_D3D9_CHECK(D3DXCreateRenderToSurface(g_pd3dDevice, desc.Width, desc.Height, desc.Format, TRUE, /*D3DFMT_D24X8*/g_d3dpp.AutoDepthStencilFormat, &render_target_d3d9.render_to_surface),
							_T("Texture creating error: can't create render to surface interface, D3DXCreateRenderToSurface failed"));

			GW_D3D9_CHECK(g_pd3dDevice->CreateOffscreenPlainSurface(render_target.Width, render_target.Height, g_d3dpp.BackBufferFormat, D3DPOOL_SYSTEMMEM, &render_target_d3d9.surface_sysmem, NULL),
							_T("Texture creating error: can't create offscreen surface, IDirect3DDevice9::CreateOffscreenPlainSurface failed"));

			render_target_d3d9.render_to_surface->BeginScene(render_target_d3d9.surface, NULL);
			g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00FFFFFF, 1.0f, 0);
			render_target_d3d9.render_to_surface->EndScene(NULL);
		}
	}

	bool show_test_window = true;
	bool show_another_window = false;

	ImVec2 pos = ImVec2(0, 0);

	Timer timer(0, options.mediaplayer_check_delay); // fire immediately, repeat every 5000 ms
	timer.start(TimerCallback<user_info_t>(userinfo, &user_info_t::on_timer));
	//Thread::sleep(5000);
	//timer->stop();

	// Main loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while(msg.message != WM_QUIT)
	{
		if(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}

		ImGui_ImplDX9_NewFrame();

#if 0
		// 1. Show a simple window
		// Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
		{
			static float f = 0.0f;
			ImGui::Text("Hello, world!");
			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
			ImGui::ColorEdit3("clear color", (float*)&clear_col);
			if (ImGui::Button("Test Window")) show_test_window ^= 1;
			if (ImGui::Button("Another Window")) show_another_window ^= 1;
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		}

		// 2. Show another simple window, this time using an explicit Begin/End pair
		if (show_another_window)
		{
			ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
			ImGui::Begin("Another Window", &show_another_window);
			ImGui::Text("Hello");
			ImGui::End();
		}

		// 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
		if (show_test_window)
		{
			ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
			ImGui::ShowTestWindow(&show_test_window);
		}
#endif

		int current_title_index = 0;

		RECT window_rect = { 0 };
		GetWindowRect(hWnd, &window_rect);

		//unsigned int nScreenWidth;
		//unsigned int nScreenHeight;

		//nScreenWidth  = ::GetSystemMetrics(SM_CXSCREEN);
		//nScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);

		RECT desktop_work_area_rect;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &desktop_work_area_rect, 0);

		uint32_t desktop_work_area_width  = desktop_work_area_rect.right - desktop_work_area_rect.left;
		uint32_t desktop_work_area_height = desktop_work_area_rect.bottom - desktop_work_area_rect.top;

		uint32_t window_rect_width  = userinfo.show_title_popup ? popup_w : w; //window_rect.right - window_rect.left;
		uint32_t window_rect_height = userinfo.show_title_popup ? userinfo.get_cover_height(current_title_index) + ImGui::GetStyle().FramePadding.y + ImGui::GetStyle().WindowPadding.y + ImGui::GetStyle().ItemSpacing.y/*popup_h*/ : h; //window_rect.bottom - window_rect.top;

		window_rect.left = userinfo.show_title_popup ? desktop_work_area_width - window_rect_width : x;
		window_rect.top  = userinfo.show_title_popup ? desktop_work_area_height - window_rect_height : y;
		window_rect.right = window_rect.left + window_rect_width;
		window_rect.bottom = window_rect.top + window_rect_height;

		static bool hiden = false;

		if(userinfo.show_title_popup)
		{
			AdjustWindowRectEx(&window_rect, GetWindowLong(hWnd, GWL_STYLE), FALSE, GetWindowLong(hWnd, GWL_EXSTYLE));

			window_rect_width  = window_rect.right - window_rect.left;
			window_rect_height = window_rect.bottom - window_rect.top;

			if(!hiden)
			{
				ShowWindow(hWnd, SW_HIDE);
				hiden = true;
			}

			SetWindowPos(hWnd, HWND_TOPMOST, window_rect.left, window_rect.top, window_rect_width, window_rect_height, 0/*SWP_NOREDRAW*//*SWP_HIDEWINDOW*/);
			if(options.animate_popup)
				AnimateWindow(hWnd, options.popup_delay, AW_SLIDE | AW_ACTIVATE | AW_VER_NEGATIVE);
			else
			{
				ShowWindow(hWnd, SW_SHOWNORMAL);
				//Sleep(options.popup_delay);
			}
			//UpdateWindow(hWnd);
		}
		else if(hiden)
		{
			SetWindowPos(hWnd, HWND_TOP, x, y, window_rect_width, window_rect_height, 0);

			ShowWindow(hWnd, SW_SHOWNORMAL);
			hiden = false;
		}
		else
		{
			x = window_rect.left + pos.x;
			y = window_rect.top + pos.y;
		}

		RECT client_rect = { 0 };
		GetClientRect(hWnd, &client_rect);

		ImGui::SetNextWindowPos(ImVec2(0,  0));
		ImGui::SetNextWindowSize(ImVec2(client_rect.right - client_rect.left, client_rect.bottom - client_rect.top));

		ImGui::Begin(window_title.c_str(), &show_another_window, options.no_native_windows ? 0 : ImGuiWindowFlags_NoTitleBar/*ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar*/);

		if(userinfo.show_title_popup)
		{
			static int current_title_status = NU_TITLE_STATUS_NOT_ADDED;

			userinfo.title_ui(current_title_status);
		}
		else
			current_title_index = userinfo.main();

		pos = ImGui::GetWindowPos();

		ImGui::End();

		//GetWindowRect(hWnd, &window_rect);

		////MoveWindow(hwnd, pos.x, pos.y, , );
		//SetWindowPos(hWnd, 
		//			 HWND_TOP, 
		//			 window_rect.left + pos.x, 
		//			 window_rect.top + pos.y, 
		//			 0, 0,          // Ignores size arguments. 
		//			 SWP_NOSIZE);

		//std::vector<user_title_info_t>::iterator current_title_it = userinfo.titles.begin() + current_title_index;

		// Rendering
		Render();
	}

	timer.stop();

	userinfo.save();

	ImGui_ImplDX9_Shutdown();

	if(render_target_d3d9.handle)
	{
		SAFE_RELEASE(render_target_d3d9.render_to_surface);
		SAFE_RELEASE(render_target_d3d9.surface_sysmem);
		SAFE_RELEASE(render_target_d3d9.surface);
		SAFE_RELEASE(render_target_d3d9.handle);
	}

	if (g_pd3dDevice) g_pd3dDevice->Release();
	if (pD3D) pD3D->Release();

	UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
}

