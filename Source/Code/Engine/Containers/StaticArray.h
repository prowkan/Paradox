#pragma once

template<typename T, size_t N>
class StaticArray
{
	public:

		T& operator[](size_t Index)
		{
			return ArrayData[Index];
		}

	private:

		T ArrayData[N];
};