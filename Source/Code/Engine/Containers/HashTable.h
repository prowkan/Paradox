#pragma once

#include "Hash.h"
#include "DefaultAllocator.h"

template<typename KeyType, typename ValueType, typename Allocator = DefaultAllocator, uint64_t(HashFunc)(const KeyType&) = DefaultHashFunction<KeyType>>
class HashTable
{
	public:

		HashTable()
		{
			Size = 0;
		}

		HashTable(const HashTable& OtherHashTable)
		{
			Size = OtherHashTable.Size;

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

			Size++;
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
				else
				{
					break;
				}
			}

			return *(ValueType*)nullptr;
		}

		const int GetSize() const { return Size; }

		struct Iterator
		{
			private:

				using NodeType = typename HashTable<KeyType, ValueType, Allocator, HashFunc>::Node;

				int TableCellIndex;
				NodeType *TableNode, **TableNodes;

			public:

				Iterator(int TableCellIndex, NodeType** TableNodes, NodeType* TableNode) : TableCellIndex(TableCellIndex), TableNodes(TableNodes), TableNode(TableNode)
				{

				}

				NodeType* GetTableNode() { return TableNode; }

				Iterator& operator++()
				{
					if (TableNode->Next)
					{
						TableNode = TableNode->Next;
					}
					else
					{
						TableCellIndex++;

						while (true)
						{
							if (TableCellIndex >= TableSize)
							{
								TableNode = nullptr;
								break;
							}

							if (TableNodes[TableCellIndex])
							{
								TableNode = TableNodes[TableCellIndex];
								break;
							}

							TableCellIndex++;
						}
					}					

					return *this;
				}

				bool operator!=(Iterator& OtherIterator)
				{
					return TableNode != OtherIterator.TableNode;
				}

				NodeType*& operator*()
				{
					return TableNode;
				}
		};

		Iterator Begin()
		{
			for (size_t i = 0; i < TableSize; i++)
			{
				if (Nodes[i])
				{
					return Iterator(i, Nodes, Nodes[i]);
				}
			}
		}

		Iterator End()
		{
			return Iterator(TableSize, Nodes, nullptr);
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

		int Size;
};

template<typename KeyType, typename ValueType>
typename HashTable<KeyType, ValueType>::Iterator begin(HashTable<KeyType, ValueType>& Table)
{
	return Table.Begin();
}

template<typename KeyType, typename ValueType>
typename HashTable<KeyType, ValueType>::Iterator end(HashTable<KeyType, ValueType>& Table)
{
	return Table.End();
}