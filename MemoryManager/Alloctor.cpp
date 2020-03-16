#include "Alloctor.h"

void* operator new(size_t size)
{
	return MemoryManager::Instance().allocMemory(size);
}

void* operator new[](size_t size)
{
	return MemoryManager::Instance().allocMemory(size);
}

void operator delete(void* p)
{
	MemoryManager::Instance().freeMemory(p);
}

void operator delete[](void* p)
{
	MemoryManager::Instance().freeMemory(p);
}

