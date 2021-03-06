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

			TableSize = 1;

			Nodes = (Node**)Allocator::AllocateMemory(sizeof(Node*));
			Nodes[0] = nullptr;
		}

		HashTable(const HashTable& OtherHashTable)
		{
			Size = OtherHashTable.Size;
			TableSize = OtherHashTable.TableSize;

			Allocator::FreeMemory(Nodes);
			Nodes = (Node**)Allocator::AllocateMemory(sizeof(Node*) * TableSize);

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

		void ReHash(const size_t NewTableSize)
		{
			size_t OldTableSize = TableSize;
			Node** OldNodes = Nodes;

			TableSize = NewTableSize;

			Nodes = (Node**)Allocator::AllocateMemory(sizeof(Node*) * TableSize);
			ZeroMemory(Nodes, sizeof(Node*) * TableSize);

			for (size_t i = 0; i < OldTableSize; i++)
			{
				if (OldNodes[i])
				{
					Node *CurrentNode = OldNodes[i];

					while (CurrentNode)
					{
						uint64_t Hash = HashFunc(CurrentNode->Key) % TableSize;

						if (!Nodes[Hash])
						{
							Nodes[Hash] = CurrentNode;
							CurrentNode = CurrentNode->Next;
							Nodes[Hash]->Next = nullptr;
						}
						else
						{
							Node *LastNodeInList = Nodes[Hash];

							while (true)
							{
								if (!LastNodeInList->Next)
								{
									LastNodeInList->Next = CurrentNode;
									CurrentNode = CurrentNode->Next;
									LastNodeInList->Next->Next = nullptr;
									break;
								}

								LastNodeInList = LastNodeInList->Next;
							}
						}
					}
				}
			}

			Allocator::FreeMemory(OldNodes);
		}

		void Insert(const KeyType& Key, const ValueType& Value)
		{
			float LoadFactor = (float)Size / (float)TableSize;

			if (LoadFactor > 2.0f)
			{
				ReHash(TableSize + 1);
			}

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

		bool HasKey(const KeyType& Key)
		{
			uint64_t Hash = HashFunc(Key) % TableSize;

			Node *CurrentNode = Nodes[Hash];

			if (!CurrentNode) return false;

			while (true)
			{
				if (CurrentNode->Key == Key)
				{
					return true;
				}

				if (CurrentNode->Next)
				{
					CurrentNode = CurrentNode->Next;
				}
				else
				{
					return false;
				}
			}

			return false;
		}

		const int GetSize() const { return Size; }

		struct Iterator
		{
			private:

				using NodeType = typename HashTable<KeyType, ValueType, Allocator, HashFunc>::Node;

				size_t TableCellIndex;
				size_t TableSize;
				NodeType *TableNode, **TableNodes;

			public:

				Iterator(size_t TableCellIndex, NodeType** TableNodes, NodeType* TableNode, size_t TableSize) : TableCellIndex(TableCellIndex), TableNodes(TableNodes), TableNode(TableNode), TableSize(TableSize)
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
					return Iterator(i, Nodes, Nodes[i], TableSize);
				}
			}

			return Iterator(TableSize, Nodes, nullptr, TableSize);
		}

		Iterator End()
		{
			return Iterator(TableSize, Nodes, nullptr, TableSize);
		}

	private:

		size_t TableSize;

		struct Node
		{
			KeyType Key;
			ValueType Value;
			Node *Next;

			Node()
			{
				Next = nullptr;
			}
		};

		Node **Nodes = nullptr;

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