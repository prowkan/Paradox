#pragma once

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

		size_t GetFileSize(const string& FileName);
		void LoadFile(const string& FileName, void *PointerToWrite);

	private:

		void MountPackage(const string& PackageName, const char16_t* FileName);

		map<string, HANDLE> ArchiveFiles;

		map<string, FileTableEntry> GlobalFileTable;
};