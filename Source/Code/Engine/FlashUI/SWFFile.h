#pragma once

struct SWFRect
{
	int64_t XMin, XMax, YMin, YMax;
};

struct SWFRGB
{
	uint8_t R, G, B;
};

struct SWFMatrix
{
	uint8_t HasScale : 1;
	uint8_t HasRotate : 1;
	float ScaleX, ScaleY;
	float RotateSkew0, RotateSkew1;
	int32_t TranslateX, TranslateY;
};

class SWFFile
{
	public:

		void Open(const char16_t* FileName);
		void Close();

		template<typename T>
		T Read();

		uint64_t ReadUnsignedBits(const uint8_t BitsCount);
		int64_t ReadSignedBits(const uint8_t BitsCount);

		uint32_t ReadEncodedU32();

		SWFRect ReadRect();
		SWFRGB ReadRGB();
		SWFMatrix ReadMatrix();

		void ReadString();

		void SkipBytes(const size_t BytesCount);
		void AlignToByte();

		bool IsEndOfFile() { return CurrentByte == SWFFileSize; }

		static const uint32_t TWIPS_IN_PIXEL = 20;
		static const uint32_t BITS_PER_RECT_COORD = 5;

	private:

		BYTE *SWFFileData;
		SIZE_T SWFFileSize;

		SIZE_T CurrentByte;
		SIZE_T CurrentBit;		
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