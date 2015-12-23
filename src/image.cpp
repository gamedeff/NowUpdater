//===================================================================================
// GameWare Engine      
// Copyright (C) GW-Labs, 2004-2009
//===================================================================================
// Module name:         image.cpp
// Module date:         18.11.2009 - 21:29
// Module author:       Fedor Gavrilov aka Dorfe
// Module description:  
//===================================================================================
// Precompiled header:
//-----------------------------------------------------------------------------------
//#include "precompiled.h"
//#pragma hdrstop
#include <stdlib.h>
#include <assert.h>
//-----------------------------------------------------------------------------------
#include "image.h"
//-----------------------------------------------------------------------------------
void Image_Destroy(BitmapImage *pImage)
{
	if(!pImage)
		return;

	pImage->width  = 0;
	pImage->height = 0;
	pImage->pitch  = 0;

	if(pImage->hBitmap)
	{
		DeleteObject(pImage->hBitmap);
		pImage->hBitmap = NULL;
	}

	if(pImage->hdc)
	{
		DeleteDC(pImage->hdc);
		pImage->hdc = NULL;
	}

	memset(&pImage->info, 0, sizeof(pImage->info));
	pImage->pPixels = NULL;
}
//-----------------------------------------------------------------------------------
bool Image_Create(BitmapImage *pImage, HDC hdc, int width, int height, unsigned short bits)
{
	if(!pImage)
		return false;

	// All Windows DIBs are aligned to 4-byte (DWORD) memory boundaries. This
	// means that each scan line is padded with extra bytes to ensure that the
	// next scan line starts on a 4-byte memory boundary. The 'pitch' member
	// of the Image structure contains width of each scan line (in bytes).

	pImage->width  = width;
	pImage->height = height;
	pImage->pitch  = ((width * 32 + 31) & ~31) >> 3;

	pImage->pPixels = NULL;

	pImage->hdc = CreateCompatibleDC(hdc);

	if(!pImage->hdc)
		return false;

	memset(&pImage->info, 0, sizeof(pImage->info));

	pImage->info.bmiHeader.biSize        = sizeof(pImage->info.bmiHeader);
	pImage->info.bmiHeader.biWidth       = width;
	pImage->info.bmiHeader.biHeight      = height; // -height;
	pImage->info.bmiHeader.biBitCount    = bits;
	pImage->info.bmiHeader.biCompression = BI_RGB;
	pImage->info.bmiHeader.biPlanes      = 1;

	pImage->hBitmap = CreateDIBSection(pImage->hdc, &pImage->info, DIB_RGB_COLORS, (void **) &pImage->pPixels, NULL, 0);

	if(!pImage->hBitmap)
	{
		Image_Destroy(pImage);
		return false;
	}

	GdiFlush();
	return true;
}
//-----------------------------------------------------------------------------------
void Image_PreMultAlpha(BitmapImage *pImage)
{
	// The per pixel alpha blending API for layered windows deals with
	// pre-multiplied alpha values in the RGB channels. For further details see
	// the MSDN documentation for the BLENDFUNCTION structure. It basically
	// means we have to multiply each red, green, and blue channel in our image
	// with the alpha value divided by 255.
	//
	// Notes:
	// 1. ImagePreMultAlpha() needs to be called before every call to
	//    UpdateLayeredWindow() (in the RedrawLayeredWindow() function).
	//
	// 2. Must divide by 255.0 instead of 255 to prevent alpha values in range
	//    [1, 254] from causing the pixel to become black. This will cause a
	//    conversion from 'float' to 'BYTE' possible loss of data warning which
	//    can be safely ignored.

	if(!pImage)
		return;

	BYTE *pPixel = NULL;

	if(pImage->width * 4 == pImage->pitch)
	{
		// This is a special case. When the image width is already a multiple
		// of 4 the image does not require any padding bytes at the end of each
		// scan line. Consequently we do not need to address each scan line
		// separately. This is much faster than the below case where the image
		// width is not a multiple of 4.

		int totalBytes = pImage->width * pImage->height * 4;

		for(int i = 0; i < totalBytes; i += 4)
		{
			pPixel = &pImage->pPixels[i];

			pPixel[0] *= (BYTE)((float)pPixel[3] / 255.0f);
			pPixel[1] *= (BYTE)((float)pPixel[3] / 255.0f);
			pPixel[2] *= (BYTE)((float)pPixel[3] / 255.0f);
		}
	}
	else
	{
		// Width of the image is not a multiple of 4. So padding bytes have
		// been included in the DIB's pixel data. Need to address each scan
		// line separately. This is much slower than the above case where the
		// width of the image is already a multiple of 4.

		for(int y = 0; y < pImage->height; ++y)
		{
			for(int x = 0; x < pImage->width; ++x)
			{
				pPixel = &pImage->pPixels[(y * pImage->pitch) + (x * 4)];

				pPixel[0] *= (BYTE)((float)pPixel[3] / 255.0f);
				pPixel[1] *= (BYTE)((float)pPixel[3] / 255.0f);
				pPixel[2] *= (BYTE)((float)pPixel[3] / 255.0f);
			}
		}
	}
}
//-----------------------------------------------------------------------------------
//#define fread(a, b, c, d) fread(a, b, c, d); assert(memcmp(a, image_data + offset, c) == 0); offset += c
//#define fread(a, b, c, d) memcpy(a, image_data + offset, c); offset += c; assert(offset < image_data_size)
#define image_memread(a, b) memcpy(a, image_data + offset, b); offset += b; assert(offset < image_data_size)
//-----------------------------------------------------------------------------------
unsigned char *image_load_tga(unsigned char *image_data, unsigned int image_data_size, unsigned int *width, unsigned int *height)
{
	unsigned char rep, *data, *buffer, *ptr, info[18];
	unsigned int Width, Height, Components, Size;
	unsigned int i, j, k;

	unsigned int offset = 0;

	//FILE *FileTGA = _tfopen(_T("data/images/simple.tga"), _T("rb"));
	if(!image_data || image_data_size <= 0)
		return NULL;

	//fread(&info, 1, sizeof(info), FileTGA);  // Read header
	image_memread(&info, sizeof(info));  // Read header

	Width  = info[12] + info[13] * 256;
	Height = info[14] + info[15] * 256;

	switch(info[16]) // Read only 32 && 24 bit per pixel
	{
		case 32:
			Components = 4; // 32 bit per pixel (RGBA)
			break;
		case 24:
			Components = 3; // 24 bit per pixel (RGB)
			break;
		default:
			//fclose(FileTGA);
			return NULL;
	}

	Size = Width * Height * Components;

	buffer = (unsigned char*)malloc(Size);  // Buffer for RGB or RGBA image
	data = (unsigned char*)malloc(Width * Height * 4);  // Output RGBA image

	if(!data || !buffer)
	{
		//fclose(FileTGA);
		return NULL;
	}

	//fseek(FileTGA, info[0], SEEK_CUR);
	offset += info[0];
	i = 0;
	ptr = buffer;
	switch(info[2])
	{
		case 2:     // Unmapped RGB image
			//fread(buffer,1,Size,FileTGA);
			image_memread(buffer, Size);
			break;
		case 10:    // Run length encoded
			while(i < Size)
			{
				//if(i > 191770)
				//	DebugBreak();
				//fread(&rep,1,1,FileTGA);
				image_memread(&rep, 1);
				if(rep & 0x80)
				{
					rep ^= 0x80;
					//fread(ptr,1,Components,FileTGA);
					image_memread(ptr, Components);
					ptr += Components;
					for(j = 0; j < rep * Components; j++)
					{
						*ptr = *(ptr - Components);
						ptr ++;
					}
					i += Components * (rep + 1);
				}
				else
				{
					//if(i >= 192000)
					//	DebugBreak();
					k = Components * (rep + 1);
					//fread(ptr,1,k,FileTGA);
					image_memread(ptr, k);
					//if(ptr-buffer > 195040)
					//	DebugBreak();
					ptr += k;
					i += k;
				}
			}
			break;
		default:
			//fclose(FileTGA);
			free(buffer);
			free(data);
			return NULL;
	}

	//fclose(FileTGA);

	for(i = 0, j = 0; i < Size; i += Components, j += 4)  // BGR -> RGBA
	{
		data[j] = buffer[i + 2];
		data[j + 1] = buffer[i + 1];
		data[j + 2] = buffer[i];
		if(Components == 4) data[j + 3] = buffer[i + 3];
		else data[j + 3] = 255;
	}
	if(!(info[17] & 0x20))
		for(j = 0, k = Width * 4; j < Height / 2; j ++)
			for(i = 0; i < Width * 4; i ++)
			{
				unsigned char tmp = data[j * k + i];
				data[j * k + i] = data[(Height - j - 1) * k + i];
				data[(Height - j - 1) * k + i] = tmp;
			}

	free(buffer);

	*width = Width;
	*height = Height;

	return data;
}

//===================================================================================
// Copyright (C) GW-Labs, 2004-2009
//===================================================================================