#include "MemoryPool.hpp"


void* operator new(size_t allocSize)
{
	return MemoryManager::Instance().allocMemory(allocSize);
}


void* operator new[](size_t allocSize)
{
	return MemoryManager::Instance().allocMemory(allocSize);
}


void operator delete(void* pMem)
{
	MemoryManager::Instance().freeMemory(pMem);
}


void operator delete[](void* pMem)
{
	MemoryManager::Instance().freeMemory(pMem);
}