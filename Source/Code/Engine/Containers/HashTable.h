#pragma once

#include "String.h"

template<typename T>
uint64_t DefaultHashFunction(const T& Arg);

template<>
inline uint64_t DefaultHashFunction(const String& Arg)
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

template<typename KeyType, typename ValueType, uint64_t(HashFunc)(const KeyType&) = DefaultHashFunction>
class HashTable
{
	public:

		void Insert(const KeyType& Key, const ValueType& Value)
		{
			uint64_t Hash = HashFunc(Key) % TableSize;

			Node *NewNode = new Node;
			NewNode->Key = Key;
			NewNode->Value = Value;
			NewNode->Next = nullptr;

			if (!Nodes[Hash])
			{			
				Nodes[Hash] = NewNode;
			}
			else
			{
				Node *LastNodeInList = Nodes[Hash];

				while (true)
				{
					if (!LastNodeInList->Next)
					{
						LastNodeInList->Next = NewNode;
						break;
					}

					LastNodeInList = LastNodeInList->Next;
				}
			}
		}

		ValueType operator[](const KeyType& Key)
		{
			uint64_t Hash = HashFunc(Key) % TableSize;

			Node *CurrentNode = Nodes[Hash];

			while (true)
			{
				if (CurrentNode->Key == Key)
				{
					return CurrentNode->Value;
				}
				if (CurrentNode->Next)
				{
					CurrentNode = CurrentNode->Next;
				}
			}

			return ValueType();
		}

	private:

		static const int TableSize = 50000;

		struct Node
		{
			KeyType Key;
			ValueType Value;
			Node *Next;
		};

		Node *Nodes[TableSize] = { nullptr };
};