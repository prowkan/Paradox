// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "StackAllocator.h"

#include "SystemAllocator.h"

void StackAllocator::CreateStackAllocator(const size_t StackSize)
{
	StackAllocatorData = SystemAllocator::AllocateMemory(StackSize);
}

void StackAllocator::DestroyStackAllocator()
{
	SystemAllocator::FreeMemory(StackAllocatorData);
	StackAllocatorData = nullptr;
}