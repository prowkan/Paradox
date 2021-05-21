#pragma once

#include <Containers/HashTable.h>

struct FileTableEntry
{
	HANDLE FileHandle;
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

		void* GetShaderData(const String& ShaderName);
		size_t GetShaderSize(const String& ShaderName);

	private:

		void MountPackage(const String& PackageName, const char16_t* FileName);

		void FindAssetsRecursively(const char16_t* BaseDirectory);

		HashTable<String, HANDLE> ArchiveFiles;

		HashTable<String, FileTableEntry> GlobalAssetTable;

		struct ShaderData { void* Data; size_t Size; };

		HashTable<String, ShaderData> ShadersTable;
};