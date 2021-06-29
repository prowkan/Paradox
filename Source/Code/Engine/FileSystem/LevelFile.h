#pragma once

#include <Containers/String.h>

class LevelFile
{
	public:

		void OpenFile(const wchar_t* FileName);
		void CloseFile();

		template<typename T>
		T Read();

		template<typename T>
		void Read(T& Value);

	private:

		HANDLE LevelFile;

		void *FileData;
		size_t FilePointer;
};

template<typename T>
inline T LevelFile::Read()
{
	T Value = *(T*)((BYTE*)FileData + FilePointer);
	FilePointer += sizeof(T);
	return Value;
}

template<typename T>
inline void LevelFile::Read(T& Value)
{
	Value = *(T*)((BYTE*)FileData + FilePointer);
	FilePointer += sizeof(T);
}

template<>
inline String LevelFile::Read()
{
	String Value((char*)((BYTE*)FileData + FilePointer));

	FilePointer += Value.GetLength() + 1;
	
	return Value;
}

template<>
inline void LevelFile::Read(String& Value)
{
	new (&Value) String((char*)((BYTE*)FileData + FilePointer));

	FilePointer += Value.GetLength() + 1;
}