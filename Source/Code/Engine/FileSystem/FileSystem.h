#pragma once

#include <Containers/HashTable.h>

struct FileTableEntry
{
	HANDLE ArchiveFileHandle;
	size_t Size;
	size_t Offset;
};

class FileSystem
{
	public:

		void InitSystem();
		void ShutdownSystem();

		size_t GetFileSize(const String& FileName);
		void LoadFile(const String& FileName, void *PointerToWrite);

	private:

		void MountPackage(const String& PackageName, const char16_t* FileName);

		HashTable<String, HANDLE> ArchiveFiles;

		HashTable<String, FileTableEntry> GlobalFileTable;
};