#include "MemoryPool.hpp"

/* 申请内存流程 */
/*
	########################
	1.使用 new 申请内存
	2.进入 MyNewDelete 中的 new 重载
	3.内存管理工具 Instance_Singleton() 初始化静态的 MemoryManager 对象
	4.MemoryManager 的构造函数内 初始化 MemoryPoolAlloctor 成员
	5.MemoryPoolAlloctor<> 继承自 MemoryPool
		先调用 MemoryPool 基类构造函数
		再使用模板技术给内存池分配大小，并在模板内初始化
	6.
	########################
*/

void* operator new(size_t allocSize)
{
	return MemoryManager::Instance_Singleton().allocMemory(allocSize);
}


void* operator new[](size_t allocSize)
{
	return MemoryManager::Instance_Singleton().allocMemory(allocSize);
}


void operator delete(void* pMem)
{
	MemoryManager::Instance_Singleton().freeMemory(pMem);
}


void operator delete[](void* pMem)
{
	MemoryManager::Instance_Singleton().freeMemory(pMem);
}