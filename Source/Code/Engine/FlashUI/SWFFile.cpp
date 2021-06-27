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

uint64_t SWFFile::ReadUnsignedBits(const uint8_t BitsCount)
{
	uint64_t Value = 0;

	uint8_t Byte = *(uint8_t*)(SWFFileData + CurrentByte);

	SIZE_T RemainingBitsInCurrentByte = 8 - CurrentBit;
	SIZE_T AdditionalBytesCount = 0;
	if (BitsCount > RemainingBitsInCurrentByte)
	{
		AdditionalBytesCount = (BitsCount - RemainingBitsInCurrentByte) / 8 + ((BitsCount - RemainingBitsInCurrentByte) % 8 == 0 ? 0 : 1);
	}

	uint8_t BitsToRead = min(BitsCount, (uint8_t)RemainingBitsInCurrentByte);

	uint8_t Mask = ((1 << BitsToRead) - 1) << (8 - (CurrentBit + BitsToRead));
	Value |= ((Byte & Mask) >> (8 - (CurrentBit + BitsToRead))) << (BitsCount - BitsToRead);

	CurrentBit += BitsToRead;

	if (CurrentBit == 8)
	{
		this->CurrentBit = 0;
		this->CurrentByte++;
	}

	SIZE_T RemainingBits = BitsCount - RemainingBitsInCurrentByte;

	for (SIZE_T i = 0; i < AdditionalBytesCount; i++)
	{
		if (i < AdditionalBytesCount - 1)
		{
			uint8_t Byte = *(uint8_t*)(SWFFileData + CurrentByte);

			Value |= (Byte << (RemainingBits - 8));

			CurrentByte++;

			RemainingBits -= 8;
		}
		else
		{
			uint8_t Byte = *(uint8_t*)(SWFFileData + CurrentByte);

			uint8_t Mask = ((1 << RemainingBits) - 1) << (8 - RemainingBits);
			Value |= ((Byte & Mask) >> (8 - RemainingBits));

			CurrentBit = RemainingBits;

			if (CurrentBit == 8)
			{
				this->CurrentBit = 0;
				this->CurrentByte++;
			}
		}
	}

	return Value;
}

int64_t SWFFile::ReadSignedBits(const uint8_t BitsCount)
{
	uint64_t Value = ReadUnsignedBits(BitsCount);

	if ((Value & (1ull << ((uint64_t)BitsCount - 1ull))) > 0)
	{
		Value |= (((1ull << (64 - (uint64_t)BitsCount)) - 1ull) << (uint64_t)BitsCount);
	}

	return *((int64_t*)&Value);
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

	uint8_t BitsPerCoords = (uint8_t)ReadUnsignedBits(BITS_PER_RECT_COORD);

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

SWFMatrix SWFFile::ReadMatrix()
{
	SWFMatrix Matrix;

	uint8_t HasScale = (uint8_t)ReadUnsignedBits(1);

	if (HasScale)
	{
		uint8_t ScaleBits = (uint8_t)ReadUnsignedBits(5);
	}

	uint8_t HasRotate = (uint8_t)ReadUnsignedBits(1);

	if (HasRotate)
	{
		uint8_t RotateBits = (uint8_t)ReadUnsignedBits(5);
	}

	uint8_t TranslateBits = (uint8_t)ReadUnsignedBits(5);

	int32_t TranslateX = (int32_t)ReadSignedBits(TranslateBits);
	int32_t TranslateY = (int32_t)ReadSignedBits(TranslateBits);

	Matrix.HasScale = HasScale;
	Matrix.HasRotate = HasRotate;
	Matrix.TranslateX = TranslateX;
	Matrix.TranslateY = TranslateY;

	return Matrix;
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

void SWFFile::AlignToByte()
{
	if ((CurrentBit % 8) > 0)
	{
		CurrentBit = 0;
		CurrentByte++;
	}
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