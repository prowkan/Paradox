#include "ScopedMemoryBlockBase.h"

#include <Engine/Engine.h>

ScopedMemoryBlockBase::~ScopedMemoryBlockBase()
{
	Engine::GetEngine().GetMemoryManager().GetGlobalStack().DeAllocateToStack(BlockSize);
}