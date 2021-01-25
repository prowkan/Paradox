// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "SWFFile.h"

void SWFFile::Open(const char16_t* FileName)
{
	HANDLE SWFFileHandle = CreateFile((const wchar_t*)FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER SWFFileSize;
	BOOL Result = GetFileSizeEx(SWFFileHandle, &SWFFileSize);
}