#pragma once

#include "DefaultAllocator.h"

template<typename CharType, typename Allocator = DefaultAllocator>
class StringTemplate
{
	public:

		StringTemplate()
		{
			StringData = nullptr;
			StringLength = 0;
		}

		StringTemplate(const StringTemplate& OtherString)
		{
			StringLength = OtherString.StringLength;

			StringData = (CharType*)Allocator::AllocateMemory(sizeof(CharType) * (StringLength + 1));

			for (size_t i = 0; i < StringLength; i++)
			{
				StringData[i] = OtherString.StringData[i];
			}

			StringData[StringLength] = 0;
		}

		StringTemplate(const CharType* Arg)
		{
			StringLength = 0;

			for (size_t i = 0; Arg[i] != 0; i++) StringLength++;

			StringData = (CharType*)Allocator::AllocateMemory(sizeof(CharType) * (StringLength + 1));

			for (size_t i = 0; i < StringLength; i++)
			{
				StringData[i] = Arg[i];
			}

			StringData[StringLength] = 0;
		}

		StringTemplate(CharType ch)
		{
			StringLength = 1;
			StringData = (CharType*)Allocator::AllocateMemory(sizeof(CharType) * 2);
			StringData[0] = ch;
			StringData[1] = 0;
		}

		StringTemplate(int Number)
		{
			StringLength = 0;

			if (Number == 0)
			{
				StringLength = 1;
				StringData = (char*)Allocator::AllocateMemory(sizeof(char) * (StringLength + 1));
				StringData[0] = '0';
				StringData[StringLength] = 0;

				return;
			}

			int NumberCopy = Number;

			while (NumberCopy > 0)
			{
				StringLength++;
				NumberCopy = NumberCopy / 10;
			}

			StringData = (CharType*)Allocator::AllocateMemory(sizeof(CharType) * (StringLength + 1));

			size_t Index = StringLength - 1;

			while (Number > 0)
			{
				StringData[Index] = (Number % 10) + '0';
				Index--;
				Number = Number / 10;
			}

			StringData[StringLength] = 0;
		}

		~StringTemplate()
		{
			Allocator::FreeMemory(StringData);
		}

		StringTemplate& operator=(const StringTemplate& OtherString)
		{
			Allocator::FreeMemory(StringData);

			StringLength = OtherString.StringLength;

			StringData = (CharType*)Allocator::AllocateMemory(sizeof(CharType) * (StringLength + 1));

			for (size_t i = 0; i < StringLength; i++)
			{
				StringData[i] = OtherString.StringData[i];
			}

			StringData[StringLength] = 0;

			return *this;
		}

		bool operator==(const StringTemplate& OtherString) const
		{
			if (StringLength != OtherString.StringLength) return false;

			for (size_t i = 0; i < StringLength; i++)
			{
				if (StringData[i] != OtherString.StringData[i]) return false;
			}

			return true;
		}

		const size_t GetLength() const { return StringLength; }

		const CharType operator[](size_t Index) const { return StringData[Index]; }

		const CharType* GetData() const { return StringData; }

		size_t FindFirst(const CharType Ch) const
		{
			for (size_t i = 0; i < StringLength; i++)
			{
				if (StringData[i] == Ch) return i;
			}

			return -1;
		}

		size_t FindLast(const CharType Ch) const
		{
			for (size_t i = StringLength - 1; i >= 0 && i != -1; i++)
			{
				if (StringData[i] == Ch) return i;
			}

			return -1;
		}

		StringTemplate operator+=(const StringTemplate& OtherString)
		{
			CharType* OldStringData = StringData;
			size_t OldStringLength = StringLength;

			StringLength += OtherString.StringLength;

			StringData = (CharType*)Allocator::AllocateMemory(sizeof(CharType) * (StringLength + 1));

			for (size_t i = 0; i < OldStringLength; i++)
			{
				StringData[i] = OldStringData[i];
			}

			for (size_t i = 0; i < OtherString.StringLength; i++)
			{
				StringData[OldStringLength + i] = OtherString.StringData[i];
			}

			StringData[StringLength] = 0;

			Allocator::FreeMemory(OldStringData);

			return *this;
		}

		StringTemplate operator+(const StringTemplate& OtherString)
		{
			StringTemplate NewString;

			NewString.StringLength = StringLength + OtherString.StringLength;
			NewString.StringData = (CharType*)Allocator::AllocateMemory(sizeof(CharType) * (NewString.StringLength + 1));

			for (size_t i = 0; i < StringLength; i++)
			{
				NewString.StringData[i] = StringData[i];
			}

			for (size_t i = 0; i < OtherString.StringLength; i++)
			{
				NewString.StringData[StringLength + i] = OtherString.StringData[i];
			}

			NewString.StringData[NewString.StringLength] = 0;

			return NewString;
		}

		StringTemplate GetSubString(size_t First, size_t Length = -1) const
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

			NewString.StringData = (CharType*)Allocator::AllocateMemory(sizeof(CharType) * (NewString.StringLength + 1));
			
			for (size_t i = 0; i < NewString.StringLength; i++)
			{
				NewString.StringData[i] = StringData[i + First];
			}

			NewString.StringData[NewString.StringLength] = 0;

			return NewString;
		}

	private:

		CharType *StringData;
		size_t StringLength;
};

using String = StringTemplate<char>;
using UTF16String = StringTemplate<char16_t>;