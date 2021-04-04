// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "FileSystem.h"

struct FileRecord
{
	char FileName[1024];
	UINT64 FileSize;
	UINT64 FileOffset;
};

void FileSystem::MountPackage(const string& PackageName, const char16_t* FileName)
{
	HANDLE FileHandle = CreateFile((const wchar_t*)FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

	UINT FileRecordsCount;
	BOOL Result = ReadFile(FileHandle, &FileRecordsCount, 4, NULL, NULL);

	FileRecord *FileRecords = new FileRecord[FileRecordsCount];

	Result = ReadFile(FileHandle, FileRecords, FileRecordsCount * sizeof(FileRecord), NULL, NULL);

	for (UINT i = 0; i < FileRecordsCount; i++)
	{
		GlobalFileTable.emplace(FileRecords[i].FileName, FileTableEntry{ FileHandle, FileRecords[i].FileSize, FileRecords[i].FileOffset });
	}

	delete[] FileRecords;

	ArchiveFiles.emplace(PackageName, FileHandle);
}

void FileSystem::InitSystem()
{
	MountPackage("Objects", u"AssetPackages/Objects.assetpackage");
	MountPackage("Textures", u"AssetPackages/Textures.assetpackage");
	MountPackage("Shaders", u"AssetPackages/Shaders.assetpackage");
}

void FileSystem::ShutdownSystem()
{

}

size_t FileSystem::GetFileSize(const string& FileName)
{
	return GlobalFileTable[FileName].Size;
}

void FileSystem::LoadFile(const string& FileName, void *PointerToWrite)
{
	BOOL Result;
	LARGE_INTEGER FileOffset;
	FileOffset.QuadPart = GlobalFileTable[FileName].Offset;
	Result = SetFilePointerEx(GlobalFileTable[FileName].ArchiveFileHandle, FileOffset, NULL, FILE_BEGIN);
	Result = ReadFile(GlobalFileTable[FileName].ArchiveFileHandle, PointerToWrite, (DWORD)GlobalFileTable[FileName].Size, NULL, NULL);
}