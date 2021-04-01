// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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