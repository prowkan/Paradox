#pragma once

#include <Containers/String.h>

struct SWFRect
{
	int64_t XMin, XMax, YMin, YMax;
};

struct SWFRGB
{
	uint8_t R, G, B;
};

struct SWFRGBA
{
	uint8_t R, G, B, A;
};

struct SWFMatrix
{
	float ScaleX, ScaleY;
	float RotateSkew0, RotateSkew1;
	int32_t TranslateX, TranslateY;
};

class SWFStream
{
	public:

		static SWFStream CreateFromFile(const char16_t *FileName)
		{
			SWFStream Stream;
			Stream.Position = 0;
			Stream.CurrentBit = 0;

			HANDLE SWFFileHandle = CreateFile((const wchar_t*)FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			LARGE_INTEGER SWFFileSize;
			BOOL Result = GetFileSizeEx(SWFFileHandle, &SWFFileSize);
			Stream.SWFData = (BYTE*)malloc(SWFFileSize.QuadPart);
			Stream.SWFDataLength = SWFFileSize.QuadPart;
			Result = ReadFile(SWFFileHandle, Stream.SWFData, (DWORD)SWFFileSize.QuadPart, NULL, NULL);
			Result = CloseHandle(SWFFileHandle);

			return Stream;
		}

		static SWFStream CreateSubStreamFromAnother(const SWFStream& Other, const SIZE_T Length)
		{
			SWFStream Stream;
			Stream.SWFData = Other.SWFData + Other.Position;
			Stream.Position = 0;
			Stream.CurrentBit = 0;
			Stream.SWFDataLength = min(Length, Other.SWFDataLength - Other.Position);

			return Stream;
		}

		template<typename T>
		T Read();

		uint64_t ReadUnsignedBits(const uint8_t BitsCount);
		int64_t ReadSignedBits(const uint8_t BitsCount);
		float ReadFixedBits(const uint8_t BitsCount);

		uint32_t ReadEncodedU32();

		float ReadFixed();
		float ReadFixed8();

		SWFRect ReadRect();
		SWFRGB ReadRGB();
		SWFRGBA ReadRGBA();
		SWFRGBA ReadARGB();
		SWFMatrix ReadMatrix();

		String ReadString();

		void SkipBytes(const size_t BytesCount);
		void AlignToByte();

		bool IsEndOfStream() { return Position == SWFDataLength; }

		SIZE_T GetPosition() { return Position; }
		BYTE* GetData() { return SWFData; }
		SIZE_T GetLength() { return SWFDataLength; }

		static const uint32_t TWIPS_IN_PIXEL = 20;
		static const uint32_t BITS_PER_RECT_COORD = 5;

	private:

		BYTE *SWFData;
		SIZE_T SWFDataLength;
		SIZE_T Position;
		SIZE_T CurrentBit;
};

template<typename T>
inline T SWFStream::Read()
{
	AlignToByte();

	T Value = *(T*)(SWFData + Position);
	Position += sizeof(T);
	return Value;
}