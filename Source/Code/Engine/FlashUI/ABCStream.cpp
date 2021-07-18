// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ABCStream.h"

uint32_t ABCStream::ReadEncodedU30()
{
	return ReadEncodedU32() & 0x3FFFFFFF;
}

uint32_t ABCStream::ReadEncodedU32()
{
	uint32_t Value = 0;

	for (int i = 0; i < 5; i++)
	{
		uint8_t Byte = Read<uint8_t>();

		Value |= ((Byte & 0x7F) << (i * 7));

		if ((Byte & 0x80) == 0)
		{
			break;
		}
	}

	return Value;
}

int32_t ABCStream::ReadEncodedS32()
{
	int32_t Value = 0;

	for (int i = 0; i < 5; i++)
	{
		uint8_t Byte = Read<uint8_t>();

		Value |= ((Byte & 0x7F) << (i * 7));

		if ((Byte & 0x80) == 0)
		{
			if (Byte & 0x40)
			{
				if (i == 0) Value |= 0xFFFFFF80;
				if (i == 1) Value |= 0xFFFF8000;
				if (i == 2) Value |= 0xFF800000;
				if (i == 3) Value |= 0x80000000;
				if (i == 4) Value |= 0x80000000;
			}

			break;
		}
	}

	return *(int32_t*)&Value;
}

uint32_t ABCStream::ReadU24()
{
	uint8_t V1 = Read<uint8_t>();
	uint8_t V2 = Read<uint8_t>();
	uint8_t V3 = Read<uint8_t>();

	return (V3 << 16) | (V2 << 8) | V1;
}

int32_t ABCStream::ReadS24()
{
	uint8_t V1 = Read<uint8_t>();
	uint8_t V2 = Read<uint8_t>();
	uint8_t V3 = Read<uint8_t>();

	uint32_t V = ((V3 << 16) | (V2 << 8) | V1);

	return *(int32_t*)&V;
}