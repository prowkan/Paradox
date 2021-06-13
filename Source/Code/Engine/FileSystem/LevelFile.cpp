// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "LevelFile.h"

#include <MemoryManager/SystemAllocator.h>

void LevelFile::OpenFile(const wchar_t* FileName)
{
	LevelFile = CreateFile(FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER LevelFileSize;
	BOOL Result = GetFileSizeEx(LevelFile, &LevelFileSize);

	FileData = SystemAllocator::AllocateMemory(LevelFileSize.QuadPart);
	FilePointer = 0;

	Result = ReadFile(LevelFile, FileData, (DWORD)LevelFileSize.QuadPart, NULL, NULL);
}

void LevelFile::CloseFile()
{
	SystemAllocator::FreeMemory(FileData);
	BOOL Result = CloseHandle(LevelFile);
}