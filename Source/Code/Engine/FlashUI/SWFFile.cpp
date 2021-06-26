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

uint32_t SWFFile::ReadUnsignedBits(const uint32_t BitsCount)
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

int32_t SWFFile::ReadSignedBits(const uint32_t BitsCount)
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

uint32_t SWFFile::ReadEncodedU32()
{
	uint32_t Value = 0;

	for (int i = 0; i < 5; i++)
	{
		uint8_t Byte = Read<uint8_t>();

		Value |= ((Byte & (0b01111111)) << (i * 7));

		if (((Byte & 0b10000000) >> 7) == 0) break;
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

SWFRGB SWFFile::ReadRGB()
{
	SWFRGB RGB;

	RGB.R = Read<uint8_t>();
	RGB.G = Read<uint8_t>();
	RGB.B = Read<uint8_t>();

	return RGB;
}

void SWFFile::SkipBytes(const size_t BytesCount)
{
	if ((CurrentBit % 8) > 0)
	{
		CurrentBit = 0;
		CurrentByte++;
	}

	CurrentByte += BytesCount;
}

void SWFFile::ReadString()
{
	char Ch;

	while (true)
	{
		Ch = Read<char>();

		if (Ch == 0)
		{
			break;
		}
	}
}