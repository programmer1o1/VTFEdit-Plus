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

#ifndef VTF_CMD_IMAGE_IO_H
#define VTF_CMD_IMAGE_IO_H

#include "stdafx.h"

typedef struct VTFCmdLoadedImage
{
	vlUInt uiWidth;
	vlUInt uiHeight;
	vlUInt uiChannelsInFile; // 1..4
	vlByte *lpRGBA; // uiWidth * uiHeight * 4
} VTFCmdLoadedImage;

vlBool vtfcmdImageIOInit(vlChar *lpError, vlUInt uiErrorSize);
vlVoid vtfcmdImageIOShutdown();

vlBool vtfcmdLoadImageRGBA(const vlChar *lpPath, VTFCmdLoadedImage *pOut, vlChar *lpError, vlUInt uiErrorSize);
vlVoid vtfcmdFreeLoadedImage(VTFCmdLoadedImage *pImage);

vlBool vtfcmdWriteImage(const vlChar *lpPath, const vlByte *lpData, vlUInt uiWidth, vlUInt uiHeight, vlUInt uiChannels, vlChar *lpError, vlUInt uiErrorSize);

#endif
