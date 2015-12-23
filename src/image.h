//===================================================================================
// GameWare Engine      
// Copyright (C) GW-Labs, 2004-2009
//===================================================================================
// Header name:         image.h
// Header date:         6.8.2009 - 2:22
// Header author:       Fedor Gavrilov aka Dorfe
// Header description:  
//===================================================================================
#ifndef IMAGE_H
#define IMAGE_H
//-----------------------------------------------------------------------------------
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
//-----------------------------------------------------------------------------------
// Windows Header Files:
//-----------------------------------------------------------------------------------
#include <windows.h>
//-----------------------------------------------------------------------------------
// Generic wrapper around a DIB with a 32-bit color depth.
//-----------------------------------------------------------------------------------
struct BitmapImage
{
	int width;
	int height;
	int pitch;
	HDC hdc;
	HBITMAP hBitmap;
	BITMAPINFO info;
	BYTE *pPixels;
};
//-----------------------------------------------------------------------------------
void Image_Destroy(BitmapImage *pImage);
bool Image_Create(BitmapImage *pImage, HDC hdc, int width, int height, unsigned short bits);
void Image_PreMultAlpha(BitmapImage *pImage);
//-----------------------------------------------------------------------------------
unsigned char *image_load_tga(unsigned char *image_data, unsigned int image_data_size, unsigned int *width, unsigned int *height);
//-----------------------------------------------------------------------------------
#endif
//===================================================================================
// Copyright (C) GW-Labs, 2004-2009
//===================================================================================