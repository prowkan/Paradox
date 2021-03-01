#pragma once

class ScopedMemoryBlockBase
{
	public:

		~ScopedMemoryBlockBase();

	protected:

		size_t BlockSize;
};