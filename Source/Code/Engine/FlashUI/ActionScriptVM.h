#pragma once

class ActionScriptVM
{
	public:

		static void ParseASByteCode(BYTE* ByteCodeData, SIZE_T ByteCodeLength);

	private:

		template<typename T>
		static T Read();

		static uint32_t ReadEncodedU30();
		static uint32_t ReadEncodedU32();

		static BYTE* ByteCodeData;

		static SIZE_T CurrentByte;
		static SIZE_T CurrentBit;
};

template<typename T>
inline T ActionScriptVM::Read()
{
	if ((CurrentBit % 8) > 0)
	{
		CurrentBit = 0;
		CurrentByte++;
	}

	T Value = *(T*)(ByteCodeData + CurrentByte);
	CurrentByte += sizeof(T);
	return Value;
}