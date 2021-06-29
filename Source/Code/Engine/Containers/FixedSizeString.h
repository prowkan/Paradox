#pragma once

template<int N>
class FixedSizeString
{
	public:

		operator wchar_t*()
		{
			return (wchar_t*)StringData;
		}

		size_t GetStorageLength() const
		{
			return StorageLength;
		}

	private:

		char16_t StringData[N];
		size_t StringLength;
		size_t StorageLength = N;
};