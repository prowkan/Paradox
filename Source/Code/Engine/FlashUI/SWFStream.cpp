// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "SWFStream.h"

uint64_t SWFStream::ReadUnsignedBits(const uint8_t BitsCount)
{
#if 1
	uint64_t Value = 0;

	uint8_t Byte = *(uint8_t*)(SWFData + Position);

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
		this->Position++;
	}

	SIZE_T RemainingBits = BitsCount - RemainingBitsInCurrentByte;

	for (SIZE_T i = 0; i < AdditionalBytesCount; i++)
	{
		if (i < AdditionalBytesCount - 1)
		{
			uint8_t Byte = *(uint8_t*)(SWFData + Position);

			Value |= (Byte << (RemainingBits - 8));

			Position++;

			RemainingBits -= 8;
		}
		else
		{
			uint8_t Byte = *(uint8_t*)(SWFData + Position);

			uint8_t Mask = ((1 << RemainingBits) - 1) << (8 - RemainingBits);
			Value |= ((Byte & Mask) >> (8 - RemainingBits));

			CurrentBit = RemainingBits;

			if (CurrentBit == 8)
			{
				this->CurrentBit = 0;
				this->Position++;
			}
		}
	}

	return Value;
#endif

#if 0
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
#endif
}

int64_t SWFStream::ReadSignedBits(const uint8_t BitsCount)
{
#if 1
	uint64_t Value = ReadUnsignedBits(BitsCount);

	if ((Value & (1ull << ((uint64_t)BitsCount - 1ull))) > 0)
	{
		Value |= (((1ull << (64 - (uint64_t)BitsCount)) - 1ull) << (uint64_t)BitsCount);
	}

	return *((int64_t*)&Value);
#endif

#if 0
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
#endif
}

float SWFStream::ReadFixedBits(const uint8_t BitsCount)
{
	int64_t Value1 = ReadSignedBits(BitsCount - 16);
	uint64_t Value2 = ReadUnsignedBits(16);

	return (float)Value1 + (float)Value2 / 65536.0f;
}

uint32_t SWFStream::ReadEncodedU32()
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

float SWFStream::ReadFixed()
{
	int16_t Value1 = Read<int16_t>();
	int16_t Value2 = Read<int16_t>();

	return (float)Value2 + (float)Value1 / 65536.0f;
}

float SWFStream::ReadFixed8()
{
	int8_t Value1 = Read<int8_t>();
	int8_t Value2 = Read<int8_t>();

	return (float)Value2 + (float)Value1 / 256.0f;
}

SWFRect SWFStream::ReadRect()
{
	AlignToByte();

	SWFRect Rect;

	uint8_t BitsPerCoords = (uint8_t)ReadUnsignedBits(BITS_PER_RECT_COORD);

	Rect.XMin = ReadSignedBits(BitsPerCoords) / TWIPS_IN_PIXEL;
	Rect.XMax = ReadSignedBits(BitsPerCoords) / TWIPS_IN_PIXEL;
	Rect.YMin = ReadSignedBits(BitsPerCoords) / TWIPS_IN_PIXEL;
	Rect.YMax = ReadSignedBits(BitsPerCoords) / TWIPS_IN_PIXEL;

	return Rect;
}

SWFRGB SWFStream::ReadRGB()
{
	SWFRGB RGB;

	RGB.R = Read<uint8_t>();
	RGB.G = Read<uint8_t>();
	RGB.B = Read<uint8_t>();

	return RGB;
}

SWFRGBA SWFStream::ReadRGBA()
{
	SWFRGBA RGBA;

	RGBA.R = Read<uint8_t>();
	RGBA.G = Read<uint8_t>();
	RGBA.B = Read<uint8_t>();
	RGBA.A = Read<uint8_t>();

	return RGBA;
}

SWFRGBA SWFStream::ReadARGB()
{
	SWFRGBA RGBA;

	RGBA.A = Read<uint8_t>();
	RGBA.R = Read<uint8_t>();
	RGBA.G = Read<uint8_t>();
	RGBA.B = Read<uint8_t>();

	return RGBA;
}

SWFMatrix SWFStream::ReadMatrix()
{
	AlignToByte();

	SWFMatrix Matrix;

	uint8_t HasScale = (uint8_t)ReadUnsignedBits(1);

	if (HasScale)
	{
		uint8_t ScaleBits = (uint8_t)ReadUnsignedBits(5);

		Matrix.ScaleX = ReadFixedBits(ScaleBits);
		Matrix.ScaleY = ReadFixedBits(ScaleBits);
	}
	else
	{
		Matrix.ScaleX = 1.0f;
		Matrix.ScaleY = 1.0f;
	}

	uint8_t HasRotate = (uint8_t)ReadUnsignedBits(1);

	if (HasRotate)
	{
		uint8_t RotateBits = (uint8_t)ReadUnsignedBits(5);

		Matrix.RotateSkew0 = ReadFixedBits(RotateBits);
		Matrix.RotateSkew1 = ReadFixedBits(RotateBits);
	}
	else
	{
		Matrix.RotateSkew0 = 0.0f;
		Matrix.RotateSkew1 = 0.0f;
	}

	uint8_t TranslateBits = (uint8_t)ReadUnsignedBits(5);

	int32_t TranslateX = (int32_t)ReadSignedBits(TranslateBits) / TWIPS_IN_PIXEL;
	int32_t TranslateY = (int32_t)ReadSignedBits(TranslateBits) / TWIPS_IN_PIXEL;

	Matrix.TranslateX = TranslateX;
	Matrix.TranslateY = TranslateY;

	return Matrix;
}

void SWFStream::SkipBytes(const size_t BytesCount)
{
	AlignToByte();

	Position += BytesCount;
}

void SWFStream::AlignToByte()
{
	if ((CurrentBit % 8) > 0)
	{
		CurrentBit = 0;
		Position++;
	}
}

String SWFStream::ReadString()
{
	AlignToByte();

	String string((char*)SWFData + Position);

	Position += string.GetLength() + 1;

	return string;
}