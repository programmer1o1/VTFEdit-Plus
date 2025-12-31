/*
 * VTFCmd
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "image_io.h"

#ifndef VTFCMD_USE_DEVIL
#define VTFCMD_USE_DEVIL 1
#endif

#if VTFCMD_USE_DEVIL

static ILuint g_DevILImage = 0;

static void flip_vertical_copy(vlByte *lpDest, const vlByte *lpSrc, vlUInt uiWidth, vlUInt uiHeight, vlUInt uiChannels)
{
	const vlUInt uiRowSize = uiWidth * uiChannels;
	for(vlUInt y = 0; y < uiHeight; y++)
	{
		memcpy(lpDest + y * uiRowSize, lpSrc + (uiHeight - 1 - y) * uiRowSize, uiRowSize);
	}
}

vlBool vtfcmdImageIOInit(vlChar *lpError, vlUInt uiErrorSize)
{
	(void)uiErrorSize;

	ilInit();

	ilEnable(IL_ORIGIN_SET);
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);

	ilGenImages(1, &g_DevILImage);
	ilBindImage(g_DevILImage);

	if(lpError) *lpError = '\0';
	return vlTrue;
}

vlVoid vtfcmdImageIOShutdown()
{
	if(g_DevILImage != 0)
	{
		ilDeleteImages(1, &g_DevILImage);
		g_DevILImage = 0;
	}

	ilShutDown();
}

vlBool vtfcmdLoadImageRGBA(const vlChar *lpPath, VTFCmdLoadedImage *pOut, vlChar *lpError, vlUInt uiErrorSize)
{
	if(pOut == NULL)
		return vlFalse;

	memset(pOut, 0, sizeof(*pOut));

	if(!ilLoadImage(lpPath))
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "DevIL failed to load image.");
		return vlFalse;
	}

	pOut->uiWidth = (vlUInt)ilGetInteger(IL_IMAGE_WIDTH);
	pOut->uiHeight = (vlUInt)ilGetInteger(IL_IMAGE_HEIGHT);
	pOut->uiChannelsInFile = (vlUInt)ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
	if(pOut->uiChannelsInFile < 1) pOut->uiChannelsInFile = 1;
	if(pOut->uiChannelsInFile > 4) pOut->uiChannelsInFile = 4;

	if(!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE))
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "DevIL failed to convert image to RGBA8.");
		return vlFalse;
	}

	const vlUInt uiSize = pOut->uiWidth * pOut->uiHeight * 4;
	pOut->lpRGBA = (vlByte *)malloc(uiSize);
	if(pOut->lpRGBA == NULL)
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "malloc() failed.");
		return vlFalse;
	}

	memcpy(pOut->lpRGBA, ilGetData(), uiSize);
	if(lpError) *lpError = '\0';
	return vlTrue;
}

vlVoid vtfcmdFreeLoadedImage(VTFCmdLoadedImage *pImage)
{
	if(pImage == NULL)
		return;

	free(pImage->lpRGBA);
	memset(pImage, 0, sizeof(*pImage));
}

vlBool vtfcmdWriteImage(const vlChar *lpPath, const vlByte *lpData, vlUInt uiWidth, vlUInt uiHeight, vlUInt uiChannels, vlChar *lpError, vlUInt uiErrorSize)
{
	if(uiChannels != 3 && uiChannels != 4)
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "Unsupported channel count %u.", uiChannels);
		return vlFalse;
	}

	const vlUInt uiSize = uiWidth * uiHeight * uiChannels;
	vlByte *lpFlipped = (vlByte *)malloc(uiSize);
	if(lpFlipped == NULL)
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "malloc() failed.");
		return vlFalse;
	}

	flip_vertical_copy(lpFlipped, lpData, uiWidth, uiHeight, uiChannels);

	if(!ilTexImage((ILuint)uiWidth, (ILuint)uiHeight, 1, (ILubyte)uiChannels,
	               uiChannels == 4 ? IL_RGBA : IL_RGB, IL_UNSIGNED_BYTE, lpFlipped))
	{
		free(lpFlipped);
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "DevIL failed to create image for output.");
		return vlFalse;
	}

	free(lpFlipped);

	if(!ilSaveImage(lpPath))
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "DevIL failed to save image.");
		return vlFalse;
	}

	if(lpError) *lpError = '\0';
	return vlTrue;
}

#else

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

static vlBool ext_equals(const char *lpExt, const char *lpWant)
{
	while(*lpExt == '.')
		lpExt++;

	while(*lpExt && *lpWant)
	{
		char a = (char)tolower((unsigned char)*lpExt++);
		char b = (char)tolower((unsigned char)*lpWant++);
		if(a != b)
			return vlFalse;
	}

	return *lpExt == '\0' && *lpWant == '\0';
}

static const char *find_extension(const char *lpPath)
{
	const char *dot = strrchr(lpPath, '.');
	if(dot == NULL || dot[1] == '\0')
		return "";
	return dot + 1;
}

vlBool vtfcmdImageIOInit(vlChar *lpError, vlUInt uiErrorSize)
{
	(void)uiErrorSize;
	if(lpError) *lpError = '\0';
	return vlTrue;
}

vlVoid vtfcmdImageIOShutdown()
{
}

vlBool vtfcmdLoadImageRGBA(const vlChar *lpPath, VTFCmdLoadedImage *pOut, vlChar *lpError, vlUInt uiErrorSize)
{
	if(pOut == NULL)
		return vlFalse;

	memset(pOut, 0, sizeof(*pOut));

	int w = 0, h = 0, c = 0;
	unsigned char *pixels8 = NULL;

	if(stbi_is_hdr(lpPath))
	{
		float *pixelsf = stbi_loadf(lpPath, &w, &h, &c, 4);
		if(pixelsf == NULL)
		{
			if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "stb_image failed to load HDR: %s", stbi_failure_reason());
			return vlFalse;
		}

		pixels8 = (unsigned char *)malloc((size_t)w * (size_t)h * 4);
		if(pixels8 == NULL)
		{
			stbi_image_free(pixelsf);
			if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "malloc() failed.");
			return vlFalse;
		}

		for(size_t i = 0; i < (size_t)w * (size_t)h * 4; i++)
		{
			float v = pixelsf[i];
			if(v < 0.0f) v = 0.0f;
			if(v > 1.0f) v = 1.0f;
			pixels8[i] = (unsigned char)(v * 255.0f + 0.5f);
		}

		stbi_image_free(pixelsf);
	}
	else
	{
		pixels8 = stbi_load(lpPath, &w, &h, &c, 4);
		if(pixels8 == NULL)
		{
			if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "stb_image failed to load image: %s", stbi_failure_reason());
			return vlFalse;
		}
	}

	pOut->uiWidth = (vlUInt)w;
	pOut->uiHeight = (vlUInt)h;
	pOut->uiChannelsInFile = (vlUInt)c;
	if(pOut->uiChannelsInFile < 1) pOut->uiChannelsInFile = 1;
	if(pOut->uiChannelsInFile > 4) pOut->uiChannelsInFile = 4;

	pOut->lpRGBA = (vlByte *)pixels8;
	if(lpError) *lpError = '\0';
	return vlTrue;
}

vlVoid vtfcmdFreeLoadedImage(VTFCmdLoadedImage *pImage)
{
	if(pImage == NULL)
		return;

	if(pImage->lpRGBA != NULL)
		stbi_image_free(pImage->lpRGBA);

	memset(pImage, 0, sizeof(*pImage));
}

vlBool vtfcmdWriteImage(const vlChar *lpPath, const vlByte *lpData, vlUInt uiWidth, vlUInt uiHeight, vlUInt uiChannels, vlChar *lpError, vlUInt uiErrorSize)
{
	if(uiChannels != 3 && uiChannels != 4)
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "Unsupported channel count %u.", uiChannels);
		return vlFalse;
	}

	const char *ext = find_extension(lpPath);
	int ok = 0;

	if(ext_equals(ext, "png"))
		ok = stbi_write_png(lpPath, (int)uiWidth, (int)uiHeight, (int)uiChannels, lpData, (int)(uiWidth * uiChannels));
	else if(ext_equals(ext, "tga"))
		ok = stbi_write_tga(lpPath, (int)uiWidth, (int)uiHeight, (int)uiChannels, lpData);
	else if(ext_equals(ext, "bmp"))
		ok = stbi_write_bmp(lpPath, (int)uiWidth, (int)uiHeight, (int)uiChannels, lpData);
	else if(ext_equals(ext, "jpg") || ext_equals(ext, "jpeg"))
		ok = stbi_write_jpg(lpPath, (int)uiWidth, (int)uiHeight, (int)uiChannels, lpData, 90);
	else if(ext_equals(ext, "hdr"))
	{
		if(uiChannels < 3)
		{
			if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, ".hdr export requires at least 3 channels.");
			return vlFalse;
		}

		float *pixelsf = (float *)malloc((size_t)uiWidth * (size_t)uiHeight * 3 * sizeof(float));
		if(pixelsf == NULL)
		{
			if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "malloc() failed.");
			return vlFalse;
		}

		for(size_t i = 0; i < (size_t)uiWidth * (size_t)uiHeight; i++)
		{
			pixelsf[i * 3 + 0] = lpData[i * uiChannels + 0] / 255.0f;
			pixelsf[i * 3 + 1] = lpData[i * uiChannels + 1] / 255.0f;
			pixelsf[i * 3 + 2] = lpData[i * uiChannels + 2] / 255.0f;
		}

		ok = stbi_write_hdr(lpPath, (int)uiWidth, (int)uiHeight, 3, pixelsf);
		free(pixelsf);
	}
	else
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "Unsupported export format '%s' (use png, tga, bmp, jpg, hdr).", ext);
		return vlFalse;
	}

	if(!ok)
	{
		if(lpError && uiErrorSize > 0) snprintf(lpError, uiErrorSize, "stb_image_write failed to save image.");
		return vlFalse;
	}

	if(lpError) *lpError = '\0';
	return vlTrue;
}

#endif
