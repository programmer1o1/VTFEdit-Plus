/*
 * VTFLib
 * Copyright (C) 2005-2010 Neil Jedrzejewski & Ryan Gregg

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later
 * version.
 */

#include "VTFLib.h"
#include "FileWriter.h"

using namespace VTFLib;
using namespace VTFLib::IO::Writers;

CFileWriter::CFileWriter(const vlChar *cFileName)
{
	this->hFile = NULL;

	this->cFileName = new vlChar[strlen(cFileName) + 1];
	strcpy(this->cFileName, cFileName);
}

CFileWriter::~CFileWriter()
{
	this->Close();

	delete []this->cFileName;
}

vlBool CFileWriter::Opened() const
{
	return this->hFile != NULL;
}

vlBool CFileWriter::Open()
{
	this->Close();

	this->hFile = fopen(this->cFileName, "wb");

	if(this->hFile == NULL)
	{
		LastError.Set("Error opening file.", vlTrue);

		return vlFalse;
	}

	return vlTrue;
}

vlVoid CFileWriter::Close()
{
	if(this->hFile != NULL)
	{
		fclose(this->hFile);
		this->hFile = NULL;
	}
}

vlUInt CFileWriter::GetStreamSize() const
{
	if(this->hFile == NULL)
	{
		return 0;
	}

	long current = ftell(this->hFile);
	fseek(this->hFile, 0, SEEK_END);
	long size = ftell(this->hFile);
	fseek(this->hFile, current, SEEK_SET);
	return (vlUInt)size;
}

vlUInt CFileWriter::GetStreamPointer() const
{
	if(this->hFile == NULL)
	{
		return 0;
	}

	return (vlUInt)ftell(this->hFile);
}

vlUInt CFileWriter::Seek(vlLong lOffset, vlUInt uiMode)
{
	if(this->hFile == NULL)
	{
		return 0;
	}

	int origin = SEEK_SET;
	switch(uiMode)
	{
	case FILE_BEGIN: origin = SEEK_SET; break;
	case FILE_CURRENT: origin = SEEK_CUR; break;
	case FILE_END: origin = SEEK_END; break;
	default: origin = SEEK_SET; break;
	}

	fseek(this->hFile, lOffset, origin);
	return (vlUInt)ftell(this->hFile);
}

vlBool CFileWriter::Write(vlChar cChar)
{
	if(this->hFile == NULL)
	{
		return vlFalse;
	}

	size_t written = fwrite(&cChar, 1, 1, this->hFile);
	if(written != 1 && ferror(this->hFile))
	{
		LastError.Set("fwrite() failed.", vlTrue);
	}

	return written == 1;
}

vlUInt CFileWriter::Write(vlVoid *vData, vlUInt uiBytes)
{
	if(this->hFile == NULL)
	{
		return 0;
	}

	size_t written = fwrite(vData, 1, uiBytes, this->hFile);
	if(written < uiBytes && ferror(this->hFile))
	{
		LastError.Set("fwrite() failed.", vlTrue);
	}

	return (vlUInt)written;
}
