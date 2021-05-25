// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "FileSystem.h"

#include <Containers/DynamicArray.h>

#include <MemoryManager/SystemAllocator.h>

struct FileRecord
{
	char FileName[1024];
	UINT64 FileSize;
	UINT64 FileOffset;
};

void FileSystem::MountPackage(const String& PackageName, const char16_t* FileName)
{
	HANDLE FileHandle = CreateFile((const wchar_t*)FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

	UINT FileRecordsCount;
	BOOL Result = ReadFile(FileHandle, &FileRecordsCount, 4, NULL, NULL);

	FileRecord *FileRecords = new FileRecord[FileRecordsCount];

	Result = ReadFile(FileHandle, FileRecords, FileRecordsCount * sizeof(FileRecord), NULL, NULL);

	for (UINT i = 0; i < FileRecordsCount; i++)
	{
		GlobalAssetTable.Insert(FileRecords[i].FileName, FileTableEntry{ FileHandle, FileRecords[i].FileSize, FileRecords[i].FileOffset });
	}

	delete[] FileRecords;

	ArchiveFiles.Insert(PackageName, FileHandle);
}

void FileSystem::FindAssetsRecursively(const char16_t* BaseDirectory)
{
	char16_t AllFileMask[255] = { 0 };
	wcscat((wchar_t*)AllFileMask, (const wchar_t*)BaseDirectory);
	wcscat((wchar_t*)AllFileMask, (const wchar_t*)u"/*");

	WIN32_FIND_DATA FindData;

	HANDLE FindHandle = FindFirstFile((const wchar_t*)AllFileMask, &FindData);

	if (FindHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && wcscmp(FindData.cFileName, (const wchar_t*)u".") != 0 && wcscmp(FindData.cFileName, (const wchar_t*)u"..") != 0)
			{
				char16_t Directory[255] = { 0 };
				wcscat((wchar_t*)Directory, (const wchar_t*)BaseDirectory);
				wcscat((wchar_t*)Directory, (const wchar_t*)u"/");
				wcscat((wchar_t*)Directory, (const wchar_t*)FindData.cFileName);

				FindAssetsRecursively(Directory);
			}
		} 			
		while (FindNextFile(FindHandle, &FindData) != 0);
	}

	char16_t AssetFileMask[255] = { 0 };
	wcscat((wchar_t*)AssetFileMask, (const wchar_t*)BaseDirectory);
	wcscat((wchar_t*)AssetFileMask, (const wchar_t*)u"/*.dasset");

	FindHandle = FindFirstFile((const wchar_t*)AssetFileMask, &FindData);

	if (FindHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			char16_t FileName[255] = { 0 };
			wcscat((wchar_t*)FileName, (const wchar_t*)BaseDirectory);
			wcscat((wchar_t*)FileName, (const wchar_t*)u"/");
			wcscat((wchar_t*)FileName, (const wchar_t*)FindData.cFileName);

			u16string FileNameStr(FileName);
			FileNameStr = FileNameStr.substr(FileNameStr.find('/') + 1);
			FileNameStr = FileNameStr.substr(0, FileNameStr.rfind('.'));
			
			for (auto& ch : FileNameStr)
			{
				if (ch == '/') ch = '.';
			}

			char FileNameKey[255];

			for (size_t i = 0; i < FileNameStr.size(); i++)
			{
				FileNameKey[i] = (char)FileNameStr[i];
			}

			FileNameKey[FileNameStr.length()] = 0;

			HANDLE FileHandle = CreateFile((const wchar_t*)FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
			LARGE_INTEGER FileSize;
			BOOL Result = GetFileSizeEx(FileHandle, &FileSize);

			GlobalAssetTable.Insert(FileNameKey, FileTableEntry{ FileHandle, (size_t)FileSize.QuadPart, 0 });
		} 		
		while (FindNextFile(FindHandle, &FindData) != 0);
	}
}

void FileSystem::InitSystem()
{
	/*MountPackage("Objects", u"AssetPackages/Objects.assetpackage");
	MountPackage("Textures", u"AssetPackages/Textures.assetpackage");
	MountPackage("Shaders", u"AssetPackages/Shaders.assetpackage");*/

	GlobalAssetTable.ReHash(50000);
	ShadersTable.ReHash(50000);

	FindAssetsRecursively(u"GameContent");

	WIN32_FIND_DATA FindData;

	HANDLE FindHandle = FindFirstFile((const wchar_t*)u"Shaders/ShaderModel50/*.dxbc", &FindData);

	if (FindHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			char16_t FileName[255] = { 0 };
			wcscat((wchar_t*)FileName, (const wchar_t*)u"Shaders/ShaderModel50");
			wcscat((wchar_t*)FileName, (const wchar_t*)u"/");
			wcscat((wchar_t*)FileName, (const wchar_t*)FindData.cFileName);

			u16string FileNameStr(FileName);
			FileNameStr = FileNameStr.substr(FileNameStr.find('/') + 1);
			FileNameStr = FileNameStr.substr(0, FileNameStr.rfind('.'));

			for (auto& ch : FileNameStr)
			{
				if (ch == '/') ch = '.';
			}

			char FileNameKey[255];

			for (size_t i = 0; i < FileNameStr.size(); i++)
			{
				FileNameKey[i] = (char)FileNameStr[i];
			}

			FileNameKey[FileNameStr.length()] = 0;

			HANDLE FileHandle = CreateFile((const wchar_t*)FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
			LARGE_INTEGER FileSize;
			BOOL Result = GetFileSizeEx(FileHandle, &FileSize);

			ShadersTable.Insert(FileNameKey, ShaderData{});
			ShadersTable[FileNameKey].Size = (size_t)FileSize.QuadPart;
			ShadersTable[FileNameKey].Data = SystemAllocator::AllocateMemory(FileSize.QuadPart);

			Result = ReadFile(FileHandle, ShadersTable[FileNameKey].Data, (DWORD)FileSize.QuadPart, NULL, NULL);

			Result = CloseHandle(FileHandle);

		} 		
		while (FindNextFile(FindHandle, &FindData) != 0);
	}
	
	FindHandle = FindFirstFile((const wchar_t*)u"Shaders/ShaderModel51/*.dxbc", &FindData);

	if (FindHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			char16_t FileName[255] = { 0 };
			wcscat((wchar_t*)FileName, (const wchar_t*)u"Shaders/ShaderModel51");
			wcscat((wchar_t*)FileName, (const wchar_t*)u"/");
			wcscat((wchar_t*)FileName, (const wchar_t*)FindData.cFileName);

			u16string FileNameStr(FileName);
			FileNameStr = FileNameStr.substr(FileNameStr.find('/') + 1);
			FileNameStr = FileNameStr.substr(0, FileNameStr.rfind('.'));

			for (auto& ch : FileNameStr)
			{
				if (ch == '/') ch = '.';
			}

			char FileNameKey[255];

			for (size_t i = 0; i < FileNameStr.size(); i++)
			{
				FileNameKey[i] = (char)FileNameStr[i];
			}

			FileNameKey[FileNameStr.length()] = 0;

			HANDLE FileHandle = CreateFile((const wchar_t*)FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
			LARGE_INTEGER FileSize;
			BOOL Result = GetFileSizeEx(FileHandle, &FileSize);

			ShadersTable.Insert(FileNameKey, ShaderData{});
			ShadersTable[FileNameKey].Size = (size_t)FileSize.QuadPart;
			ShadersTable[FileNameKey].Data = SystemAllocator::AllocateMemory(FileSize.QuadPart);

			Result = ReadFile(FileHandle, ShadersTable[FileNameKey].Data, (DWORD)FileSize.QuadPart, NULL, NULL);

			Result = CloseHandle(FileHandle);

		} 		
		while (FindNextFile(FindHandle, &FindData) != 0);
	}
}

void FileSystem::ShutdownSystem()
{

}

void* FileSystem::GetShaderData(const String& ShaderName)
{
	return ShadersTable[ShaderName].Data;
}

size_t FileSystem::GetShaderSize(const String& ShaderName)
{
	return ShadersTable[ShaderName].Size;
}

size_t FileSystem::GetFileSize(const String& FileName)
{
	return GlobalAssetTable[FileName].Size;
}

void FileSystem::LoadFile(const String& FileName, void *PointerToWrite)
{
	BOOL Result;
	LARGE_INTEGER FileOffset;
	FileOffset.QuadPart = GlobalAssetTable[FileName].Offset;
	Result = SetFilePointerEx(GlobalAssetTable[FileName].FileHandle, FileOffset, NULL, FILE_BEGIN);
	Result = ReadFile(GlobalAssetTable[FileName].FileHandle, PointerToWrite, (DWORD)GlobalAssetTable[FileName].Size, NULL, NULL);
}