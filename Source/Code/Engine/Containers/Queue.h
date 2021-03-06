#pragma once

template<typename T>
class Queue
{
	public:

		Queue()
		{
			First = nullptr;
			Last = nullptr;
		}

		void Push(const T& Element)
		{
			if (Last)
			{
				Node *NewNode = new Node;
				NewNode->Data = Element;
				NewNode->Prev = Last;
				NewNode->Next = nullptr;
				Last->Next = NewNode;
				Last = NewNode;
			}
			else
			{
				Node *NewNode = new Node;
				NewNode->Data = Element;
				NewNode->Next = nullptr;
				NewNode->Prev = nullptr;
				First = NewNode;
				Last = NewNode;
			}

			Size++;
		}

		T Pop()
		{
			if (First)
			{
				Size--;
				T Element = First->Data;
				if (First->Next) First->Next->Prev = nullptr;
				else Last = nullptr;
				Node *NewFirst = First->Next;
				delete First;
				First = NewFirst;
				return Element;
			}

			return T();
		}

		const size_t GetSize() const
		{
			return Size;
		}

	private:

		struct Node
		{
			T Data;
			Node *Next;
			Node *Prev;
		};

		Node *First;
		Node *Last;

		size_t Size;
};