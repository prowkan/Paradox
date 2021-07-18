#pragma once

class ABCStream
{
	public:

		template<typename T>
		T Read();

		uint32_t ReadEncodedU30();
		uint32_t ReadEncodedU32();
		int32_t ReadEncodedS32();
		uint32_t ReadU24();
		int32_t ReadS24();

		ABCStream(BYTE* ABCData, const SIZE_T ABCDataLength) : ABCData(ABCData), ABCDataLength(ABCDataLength)
		{
			Position = 0;
		}

		SIZE_T GetPosition() { return Position; }
		void Advane(const SIZE_T BytesCount) { Position += BytesCount; }
		BYTE* GetData() { return ABCData; }
		SIZE_T GetLength() { return ABCDataLength; }

		bool IsEndOfStream() { return Position == ABCDataLength; }

	private:

		BYTE *ABCData;
		SIZE_T ABCDataLength;
		SIZE_T Position;
};

template<typename T>
inline T ABCStream::Read()
{
	T Value = *(T*)(ABCData + Position);
	Position += sizeof(T);
	return Value;
}