#pragma once

class String
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

		String()
		{
			StringData = nullptr;
			StringLength = 0;
		}

		String(const String& OtherString)
		{
			StringLength = strlen(OtherString.StringData);
			StringData = new char[StringLength + 1];
			strcpy(StringData, OtherString.StringData);
		}

		String(const char* Arg)
		{
			StringLength = strlen(Arg);
			StringData = new char[StringLength + 1];
			strcpy(StringData, Arg);
		}

		~String()
		{
			delete[] StringData;
		}

		String& operator=(const String& OtherString)
		{
			delete[] StringData;
			StringLength = strlen(OtherString.StringData);
			StringData = new char[StringLength + 1];
			strcpy(StringData, OtherString.StringData);

			return *this;
		}

		bool operator==(const String& OtherString)
		{
			return strcmp(StringData, OtherString.StringData) == 0;
		}

		const size_t GetLength() const { return StringLength; }

		const char operator[](size_t Index) const { return StringData[Index]; }

	private:

		char *StringData;
		size_t StringLength;
};