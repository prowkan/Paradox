// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Stack.h"

#include "SystemAllocator.h"

void Stack::CreateStack(const size_t StackSize)
{
	StackData = SystemAllocator::AllocateMemory(StackSize);
	StackOffset = 0;
}

void Stack::DestroyStack()
{
	SystemAllocator::FreeMemory(StackData);
	StackData = nullptr;
}