#pragma once

#include "DefaultAllocator.h"

template<typename Allocator = DefaultAllocator>
class StringTemplate
{
	public:

		/*template<int N>
		String(const char16_t (&StringLiteral)[N])
		{
			StringData = (char16_t*)StringLiteral;
		}*/

		/*operator wchar_t*()
		{
			return (wchar_t*)StringData;
		}

		operator const wchar_t*() const
		{
			return (const wchar_t*)StringData;
		}*/

		StringTemplate()
		{
			StringData = nullptr;
			StringLength = 0;
		}

		StringTemplate(const StringTemplate& OtherString)
		{
			StringLength = strlen(OtherString.StringData);
			StringData = (char*)Allocator::AllocateMemory(sizeof(char) * (StringLength + 1));
			strcpy(StringData, OtherString.StringData);
		}

		StringTemplate(const char* Arg)
		{
			StringLength = strlen(Arg);
			StringData = (char*)Allocator::AllocateMemory(sizeof(char) * (StringLength + 1));
			strcpy(StringData, Arg);
		}

		StringTemplate(int Number)
		{
			StringLength = 0;

			int NumberCopy = Number;

			while (NumberCopy > 0)
			{
				StringLength++;
				NumberCopy = NumberCopy / 10;
			}

			StringData = (char*)Allocator::AllocateMemory(sizeof(char) * (StringLength + 1));

			size_t Index = StringLength;

			while (Number > 0)
			{
				StringData[Index] = (Number % 10) - '0';
				Index--;
				Number = Number / 10;
			}
		}

		~StringTemplate()
		{
			Allocator::FreeMemory(StringData);
		}

		StringTemplate& operator=(const StringTemplate& OtherString)
		{
			Allocator::FreeMemory(StringData);
			StringLength = strlen(OtherString.StringData);
			StringData = (char*)Allocator::AllocateMemory(sizeof(char) * (StringLength + 1));
			strcpy(StringData, OtherString.StringData);

			return *this;
		}

		bool operator==(const StringTemplate& OtherString)
		{
			return strcmp(StringData, OtherString.StringData) == 0;
		}

		const size_t GetLength() const { return StringLength; }

		const char operator[](size_t Index) const { return StringData[Index]; }

		const char* GetData() const { return StringData; }

		size_t FindFirst(const char Ch)
		{
			for (size_t i = 0; i < StringLength; i++)
			{
				if (StringData[i] == Ch) return i;
			}

			return -1;
		}

		size_t FindLast(const char Ch)
		{
			for (size_t i = 0; i < StringLength; i++)
			{
				if (StringData[i] == Ch) return i;
			}

			return -1;
		}

		StringTemplate operator+=(const StringTemplate& OtherString)
		{
			char* OldStringData = StringData;
			size_t OldStringLength = StringLength;

			StringLength += OtherString.StringLength;

			StringData = (char*)Allocator::AllocateMemory(sizeof(char) * (StringLength + 1));
			strcpy(StringData, OldStringData);
			strcpy(StringData + OldStringLength, OtherString.StringData);

			Allocator::FreeMemory(OldStringData);

			return *this;
		}

		StringTemplate operator+(const StringTemplate& OtherString)
		{
			StringTemplate NewString;

			NewString.StringLength = StringLength + OtherString.StringLength;
			NewString.StringData = (char*)Allocator::AllocateMemory(sizeof(char) * (NewString.StringLength + 1));

			strcpy(NewString.StringData, StringData);
			strcpy(NewString.StringData + StringLength, OtherString.StringData);

			return NewString;
		}

		StringTemplate GetSubString(size_t First, size_t Length = -1)
		{
			StringTemplate NewString;

			if (Length == -1)
			{
				NewString.StringLength = StringLength - First;
			}
			else
			{
				NewString.StringLength = Length;
			}

			NewString.StringData = (char*)Allocator::AllocateMemory(sizeof(char) * (NewString.StringLength + 1));
			memcpy(NewString.StringData, (char*)StringData + First, NewString.StringLength);

			return NewString;
		}

	private:

		char *StringData;
		size_t StringLength;
};

using String = StringTemplate<>;