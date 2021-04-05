// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "SWFFile.h"

void SWFFile::Open(const char16_t* FileName)
{
	HANDLE SWFFileHandle = CreateFile((const wchar_t*)FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER SWFFileSize;
	BOOL Result = GetFileSizeEx(SWFFileHandle, &SWFFileSize);
	SWFFileData = (BYTE*)malloc(SWFFileSize.QuadPart);
	this->SWFFileSize = SWFFileSize.QuadPart;
	Result = ReadFile(SWFFileHandle, SWFFileData, (DWORD)SWFFileSize.QuadPart, NULL, NULL);
	Result = CloseHandle(SWFFileHandle);

	CurrentByte = 0;
	CurrentBit = 0;
}

void SWFFile::Close()
{
	free(SWFFileData);
}

uint32_t SWFFile::ReadUnsignedBits(uint32_t BitsCount)
{
	uint32_t Value = 0;

	uint8_t CurrentByte = *(uint8_t*)(SWFFileData + this->CurrentByte);

	for (uint32_t CurrentBit = 0; CurrentBit < BitsCount; CurrentBit++)
	{
		Value |= (((1 << (7 - this->CurrentBit)) & CurrentByte) >> (7 - this->CurrentBit)) << (BitsCount - CurrentBit - 1);

		this->CurrentBit++;

		if (this->CurrentBit >= 8)
		{
			this->CurrentBit = 0;
			this->CurrentByte++;

			CurrentByte = *(uint8_t*)(SWFFileData + this->CurrentByte);
		}
	}

	return Value;
}

int32_t SWFFile::ReadSignedBits(uint32_t BitsCount)
{
	int32_t Value = 0;

	uint8_t CurrentByte = *(uint8_t*)(SWFFileData + this->CurrentByte);

	for (uint32_t CurrentBit = 0; CurrentBit < BitsCount; CurrentBit++)
	{
		Value |= (((1 << (7 - this->CurrentBit)) & CurrentByte) >> (7 - this->CurrentBit)) << (BitsCount - CurrentBit - 1);

		this->CurrentBit++;

		if (this->CurrentBit >= 8)
		{
			this->CurrentBit = 0;
			this->CurrentByte++;

			CurrentByte = *(uint8_t*)(SWFFileData + this->CurrentByte);
		}
	}

	return Value;
}

SWFRect SWFFile::ReadRect()
{
	SWFRect Rect;

	uint32_t BitsPerCoords = ReadUnsignedBits(BITS_PER_RECT_COORD);

	Rect.XMin = ReadSignedBits(BitsPerCoords) / TWIPS_IN_PIXEL;
	Rect.XMax = ReadSignedBits(BitsPerCoords) / TWIPS_IN_PIXEL;
	Rect.YMin = ReadSignedBits(BitsPerCoords) / TWIPS_IN_PIXEL;
	Rect.YMax = ReadSignedBits(BitsPerCoords) / TWIPS_IN_PIXEL;

	return Rect;
}

bool SWFFile::ReadTag(uint32_t& TagCode, uint32_t& TagLength, void*& TagData)
{
	if (CurrentByte >= SWFFileSize)
	{
		TagData = nullptr;
		return false;
	}

	uint16_t TagCodeAndlength = Read<uint16_t>();

	TagCode = (TagCodeAndlength & (0b1111111111 << 6)) >> 6;
	TagLength = TagCodeAndlength & 0b111111;

	if (TagLength == 63)
	{
		TagLength = Read<uint32_t>();
	}

	TagData = malloc(TagLength);

	for (uint32_t i = 0; i < TagLength; i++)
	{
		*((uint8_t*)TagData + i) = Read<uint8_t>();
	}

	return true;
}