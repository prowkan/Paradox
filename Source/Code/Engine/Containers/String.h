#pragma once

class String
{
	public:

		template<int N>
		String(const char16_t (&StringLiteral)[N])
		{
			StringData = (char16_t*)StringLiteral;
		}

		operator wchar_t*()
		{
			return (wchar_t*)StringData;
		}

		operator const wchar_t*() const
		{
			return (const wchar_t*)StringData;
		}

	private:

		char16_t *StringData;
		size_t StringLength;
};