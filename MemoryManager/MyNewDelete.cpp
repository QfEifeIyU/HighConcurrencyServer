#include "MemoryPool.hpp"

/* �����ڴ����� */
/*
	########################
	1.ʹ�� new �����ڴ�
	2.���� MyNewDelete �е� new ����
	3.�ڴ������ Instance_Singleton() ��ʼ����̬�� MemoryManager ����
	4.MemoryManager �Ĺ��캯���� ��ʼ�� MemoryPoolAlloctor ��Ա
	5.MemoryPoolAlloctor<> �̳��� MemoryPool
		�ȵ��� MemoryPool ���๹�캯��
		��ʹ��ģ�弼�����ڴ�ط����С������ģ���ڳ�ʼ��
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