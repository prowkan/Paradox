// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Hash.h"

template<>
uint64_t DefaultHashFunction(const String& Arg)
{
	const uint64_t Prime = 67;

	uint64_t Multiplier = 1;

	const size_t StringLength = Arg.GetLength();

	uint64_t Hash = 0;

	for (size_t i = 0; i < StringLength; i++)
	{
		Hash += Arg[i] * Multiplier;
		Multiplier *= Prime;
	}

	return Hash;
}