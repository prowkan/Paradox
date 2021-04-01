// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ScopedMemoryBlockBase.h"

#include <Engine/Engine.h>

ScopedMemoryBlockBase::~ScopedMemoryBlockBase()
{
	Engine::GetEngine().GetMemoryManager().GetGlobalStack().DeAllocateToStack(BlockSize);
}