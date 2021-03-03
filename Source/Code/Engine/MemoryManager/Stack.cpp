#include "Stack.h"

void Stack::CreateStack(const size_t StackSize)
{
	StackData = HeapAlloc(GetProcessHeap(), 0, StackSize);
}

void Stack::DestroyStack()
{
	BOOL Result = HeapFree(GetProcessHeap(), 0, StackData);
	StackData = nullptr;
}