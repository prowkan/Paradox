#pragma once

#include "Hash.h"
#include "DefaultAllocator.h"

template<typename KeyType, typename ValueType, typename Allocator = DefaultAllocator, uint64_t(HashFunc)(const KeyType&) = DefaultHashFunction<KeyType>>
class HashTable
{
	public:

		HashTable()
		{
		
		}

		HashTable(const HashTable& OtherHashTable)
		{
			for (size_t i = 0; i < TableSize; i++)
			{
				Node *OldTableNode = OtherHashTable.Nodes[i];
				Node *LastCurrentNode = nullptr;

				while (OldTableNode)
				{
					Node *NewNode = (Node*)Allocator::AllocateMemory(sizeof(Node));
					new (NewNode) Node;
					NewNode->Key = OldTableNode->Key;
					NewNode->Value = OldTableNode->Value;
					NewNode->Next = nullptr;

					if (LastCurrentNode)
					{
						LastCurrentNode->Next = NewNode;
					}
					else
					{
						Nodes[i] = NewNode;
					}

					LastCurrentNode = NewNode;

					OldTableNode = OldTableNode->Next;
				}
			}
		}

		void Insert(const KeyType& Key, const ValueType& Value)
		{
			uint64_t Hash = HashFunc(Key) % TableSize;

			Node *NewNode = (Node*)Allocator::AllocateMemory(sizeof(Node));
			new (NewNode) Node;
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

		ValueType& operator[](const KeyType& Key)
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

			return *(ValueType*)nullptr;
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