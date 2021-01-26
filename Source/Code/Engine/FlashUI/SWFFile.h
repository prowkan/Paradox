#pragma once

struct SWFRect
{
	uint32_t XMin, XMax, YMin, YMax;
};

class SWFFile
{
	public:

		void Open(const char16_t* FileName);
		void Close();

		template<typename T>
		T Read();

		uint32_t ReadUnsignedBits(uint32_t BitsCount);
		int32_t ReadSignedBits(uint32_t BitsCount);

		SWFRect ReadRect();

	private:

		BYTE *SWFFileData;
		SIZE_T SWFFileSize;

		SIZE_T CurrentByte;
		SIZE_T CurrentBit;

		const uint32_t TWIPS_IN_PIXEL = 20;
		const uint32_t BITS_PER_RECT_COORD = 5;
};

template<typename T>
inline T SWFFile::Read()
{
	if ((CurrentBit % 8) > 0)
	{
		CurrentBit = 0;
		CurrentByte++;
	}

	T Value = *(T*)(SWFFileData + CurrentByte);
	CurrentByte += sizeof(T);
	return Value;
}