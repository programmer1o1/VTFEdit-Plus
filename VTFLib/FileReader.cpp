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
#include "FileReader.h"

using namespace VTFLib;
using namespace VTFLib::IO::Readers;

CFileReader::CFileReader(const vlChar *cFileName)
{
	this->hFile = NULL;

	this->cFileName = new vlChar[strlen(cFileName) + 1];
	strcpy(this->cFileName, cFileName);
}

CFileReader::~CFileReader()
{
	this->Close();

	delete []this->cFileName;
}

vlBool CFileReader::Opened() const
{
	return this->hFile != NULL;
}

vlBool CFileReader::Open()
{
	this->Close();

	this->hFile = fopen(this->cFileName, "rb");

	if(this->hFile == NULL)
	{
		LastError.Set("Error opening file.", vlTrue);
		return vlFalse;
	}

	return vlTrue;
}

vlVoid CFileReader::Close()
{
	if(this->hFile != NULL)
	{
		fclose(this->hFile);
		this->hFile = NULL;
	}
}

vlUInt CFileReader::GetStreamSize() const
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

vlUInt CFileReader::GetStreamPointer() const
{
	if(this->hFile == NULL)
	{
		return 0;
	}

	return (vlUInt)ftell(this->hFile);
}

vlUInt CFileReader::Seek(vlLong lOffset, vlUInt uiMode)
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

vlBool CFileReader::Read(vlChar &cChar)
{
	if(this->hFile == NULL)
	{
		return vlFalse;
	}

	int ch = fgetc(this->hFile);
	if(ch == EOF)
	{
		return vlFalse;
	}

	cChar = (vlChar)ch;
	return vlTrue;
}

vlUInt CFileReader::Read(vlVoid *vData, vlUInt uiBytes)
{
	if(this->hFile == NULL)
	{
		return 0;
	}

	size_t read = fread(vData, 1, uiBytes, this->hFile);
	if(read < uiBytes && ferror(this->hFile))
	{
		LastError.Set("fread() failed.", vlTrue);
	}

	return (vlUInt)read;
}
