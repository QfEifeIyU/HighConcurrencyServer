#ifndef _MEMORYPOOL_HPP_
#define _MEMORYPOOL_HPP_

#include <mutex>
#include "MyNewDelete.h"
class MemoryPool;
typedef unsigned __int64 ptr;		// ��ӡ��ַ
static std::mutex static_mutex;

/* �ڴ�� */
class MemoryPool
{
public:
	MemoryPool()
	{
		_pAddr = nullptr;
		_header = nullptr;
		_bodySize = 0;
		_blockAmount = 0;
	}

	~MemoryPool()
	{
		if (_pAddr)
		{
			dePrint(">>>>>Memory[%4d]Pool free >>>>> [addr: %llX].\n", _bodySize, (ptr)_pAddr);
			free(_pAddr);
		}	
		
	}

	// �����
	void* TakeBlock(size_t allocSize)
	{
		std::lock_guard<std::mutex> lock(static_mutex);		// ������֤�̰߳�ȫ

		BlockHeader* pTake = nullptr;
		if (_header == nullptr)
		{
			/* �ڴ���п����ڴ�����---��ϵͳ����*/
			//dePrint("Memory[%d]Pool is empty!!!\n", _bodySize);
			pTake = (BlockHeader*)malloc(allocSize + sizeof(BlockHeader));
			if (pTake != nullptr)
			{
				pTake->_Id = -1;
				pTake->_next = nullptr;
				pTake->_refCount = 1;
				pTake->_Pool = nullptr;
				dePrint("\tmalloc [addr: %llX] [id: %d] [size: %llu] from [%llx].\n", 
					(ptr)pTake, pTake->_Id, allocSize, (ptr)pTake->_Pool);
				//dePrint("   malloc[id = %d, size = %llu, addr = %llX] from Pool[%llX]\n",
					//pTake->_Id, allocSize, (ptr)pTake, (ptr)pTake->_Pool);
			}
		}
		else 
		{
			/* ���ڴ浥Ԫ������ȡ */
			pTake = _header;
			_header = _header->_next;
			++pTake->_refCount;
			dePrint("\tTake Block[id: %d, size: %llu, addr: %llX] from Pool[%d]\n",
				pTake->_Id, allocSize, (ptr)pTake, _bodySize);
		}
		return (char*)pTake + sizeof(BlockHeader);
	}

	// �黹��
	void ReturnBlock(void* pMem)
	{
		
		BlockHeader* pReturn = (BlockHeader*)((char*)pMem - sizeof(BlockHeader));
		dePrint("\tReturn Block[id: %d, addr: %llX] to Pool[%d]\n", 
			pReturn->_Id, (ptr)pReturn, _bodySize);
		if (pReturn->_Id != -1)
		{
			/* ���ڴ����*/
			std::lock_guard<std::mutex> lock(static_mutex);
			if (--pReturn->_refCount != 0)
				return;

			pReturn->_next = _header;
			_header = pReturn;
		}
		else
		{
			if (--pReturn->_refCount != 0)
				return;
			free(pReturn);
		}
	}

protected:

	// ����һ���ڴ�أ���ʹ��ͷ���ڵ㽫�����ڴ��������
	void InitMemoryPool()
	{
		if (_pAddr != nullptr)
			return;

		// ����һ���ڴ��
		uint32_t blockSize = _bodySize + sizeof(BlockHeader);
		uint32_t poolSize = blockSize * _blockAmount;
		_pAddr = malloc(poolSize);
		dePrint(">>>>>Memory[%4dPool] alloc-> [addr: %llX] amount:%d, Total = %dB.\n", 
			_bodySize, (ptr)_pAddr, _blockAmount, poolSize);

		// ���ڴ�صĿ顮��������
		_header = (BlockHeader*)_pAddr;
		if (_header != nullptr)
		{
			_header->_Id = 0;
			_header->_next = nullptr;
			_header->_Pool = this;
			_header->_refCount = 0;
		}		

		BlockHeader* curHeader = _header;
		for (uint32_t n = 1; n < _blockAmount; ++n)
		{
			// ������һ�����ƫ��λ�ò���ʼ��ͷ��Ϣ
			int offset = blockSize * n;
			BlockHeader* nextHeader = (BlockHeader*)((char*)_pAddr + offset);	
			if (nextHeader != nullptr)
			{
				nextHeader->_Id = n;
				nextHeader->_next = nullptr;
				nextHeader->_Pool = this;
				nextHeader->_refCount = 0;
			}
			// ��ʼ����������
			if (curHeader != nullptr)
			{
				curHeader->_next = nextHeader;
				curHeader = nextHeader;
			}
		}
	}


public:
	/* �ڴ�����ڴ浥Ԫ��ͷ����Ϣ */
	class BlockHeader
	{
	public:
		/*int a = sizeof(BlockHeader); // 24; */
		MemoryPool* _Pool;		// �����ڴ��
		BlockHeader* _next;		// ��һ���ڴ浥Ԫ����Ϣ
		short _Id;				// �ڴ浥Ԫid���
		short _refCount;		// �ڴ浥Ԫ���ü���
	};

protected:
	/* 	sizeof(MemoryPool) = 24 */
	void* _pAddr;			// �ڴ�ص��׵�ַ
	BlockHeader* _header;	// �ڴ浥Ԫ��ͷ��--����ͷ
	uint32_t _bodySize;		// �ڴ浥Ԫ�Ĵ�С - ͷ��
	uint32_t _blockAmount;	// �ڴ浥Ԫ������
};


/* ʹ��ģ�弼�����ڴ�ط����С */
template <int size, int amount>
class MemoryPoolAlloctor: public MemoryPool
{
public:
	MemoryPoolAlloctor()
	{
		// �����������ʱ��Ĭ�����ȵ��û��๹��
		const int num = sizeof(void*);		// ������--�ṹ��Ĵ�С������Ա��������
		_bodySize = (size / num) * num + (size % num ? num : 0);;
		_blockAmount = amount;
		InitMemoryPool();
		//dePrint("pool Alloc: %d - %d\n", size, amount);
	}
};


/* �����ڴ�ص����� */
const int MAX_ALLOC_SIZE = 256;
class MemoryManager
{
	/* ʹ�õ���ģʽ--���캯��˽�л� */
private:
	MemoryManager()
	{
		InitArray(0, MAX_ALLOC_SIZE);
		dePrint("MemoryManager : Alloc Pool ing...\n");
	}
	~MemoryManager()
	{
		dePrint("MemoryManager : Free Pool ing...\n");
	}

	// ��ʼ���ڴ��ӳ������
	void InitArray(int begin, int end)
	{

		for (int n = begin; n < end; ++n)
		{
			if (n <= 64)
				_poolMem[n] = &_mem64;
			else if (n <= 128)
				_poolMem[n] = &_mem128;
			else
				_poolMem[n] = &_mem256;		// ��������Ĵ�С�����Ӧ���ڴ�ؽ�������
		}
	}

public:
	typedef MemoryPool::BlockHeader BlockUnitHeader;		// �ڴ浥Ԫ��ͷָ��

	/* ����ģʽ--ʹ������ϵͳ��ֻ��һ���ڴ������ */
	static MemoryManager& Instance_Singleton()
	{
		static MemoryManager static_mgr;
		return static_mgr;
	}

	void* allocMemory(size_t allocSize)
	{
		if (allocSize <= MAX_ALLOC_SIZE)
		{
			// sizeС��MAX_ALLOC_SIZE��ͨ��poolMemӳ�䵽��Ӧ�ڴ����
			return _poolMem[allocSize - 1]->TakeBlock(allocSize);
		}
		else
		{
			// �����ڴ����������������	
			BlockUnitHeader* pTake = (BlockUnitHeader*)malloc(allocSize + sizeof(BlockUnitHeader));
			if (pTake != nullptr)
			{
				pTake->_Id = -1;
				pTake->_refCount = 1;
				pTake->_Pool = nullptr;
				pTake->_next = nullptr;
				dePrint("   malloc [addr: %llX] [id: %d] [size: %llu] from [%llx].\n",
					(ptr)pTake, pTake->_Id, allocSize, (ptr)pTake->_Pool);
				//dePrint("\talloc: %llX, id = %d, size = %llu.\n", (ptr)pTake, pTake->_Id, allocSize);
			}
			return ((char*)pTake + sizeof(BlockUnitHeader));
		}
	}


	void freeMemory(void* pMem)
	{
		BlockUnitHeader* pReturn = (BlockUnitHeader*)((char*)pMem - sizeof(BlockUnitHeader));
		if (pReturn->_Id != -1)
		{
			// ������ڴ������ڴ棬�����������ڴ���ͷ�
			pReturn->_Pool->ReturnBlock(pMem);
		}
		else
		{
			// ��������ڴ�ص��ڴ棬��Ϊ�����ʱ�����˵�Ԫ��Ϣ�������ͷ�BlockUnit����		
			if (--pReturn->_refCount == 0)
			{
				dePrint("\tfree [addr: %llX] [id: %d] from [%llx].\n",
					(ptr)pReturn, pReturn->_Id, (ptr)pReturn->_Pool);
				//dePrint("   free: %llX, id = %d, ref = %d.\n", 
					//(ptr)pReturn, pReturn->_Id, pReturn->_refCount);
				free(pReturn);
			}
		}
	}


private:
	MemoryPoolAlloctor<64, 10> _mem64;		
	MemoryPoolAlloctor<128, 100> _mem128;	
	MemoryPoolAlloctor<256, 100> _mem256;	
	//MemoryPoolAlloctor<512, 100000> _mem512;
	//MemoryPoolAlloctor<1024, 100000> _mem102
	//MemoryPoolAlloctor<1024, 1000> _mem64;	
	MemoryPool* _poolMem[MAX_ALLOC_SIZE];		// ӳ������
};



#endif // !_MEMORYPOOL_HPP_

