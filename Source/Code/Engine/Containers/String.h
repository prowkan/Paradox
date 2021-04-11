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

	private:

		char *StringData;
		size_t StringLength;
};

using String = StringTemplate<>;