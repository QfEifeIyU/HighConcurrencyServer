#ifndef _MEMORYPOOL_HPP_
#define _MEMORYPOOL_HPP_

#include <mutex>
#include "MyNewDelete.h"
class MemoryPool;
typedef unsigned __int64 ptr;		// ��ӡ��ַ


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
			dePrint(">>>>>Memory[%d]: [addr: %llX].\n", _bodySize, (ptr)_pAddr);
			free(_pAddr);
		}	
		
	}

	// �����
	void* TakeBlock(size_t allocSize)
	{
		std::lock_guard<std::mutex> lock(_mutex);		// ������֤�̰߳�ȫ

		BlockHeader* pTake = nullptr;
		if (_header == nullptr)
		{
			/* �ڴ���п����ڴ�����---�������ڴ������*/
			//dePrint("[%d] is empty, and alloc from more.\n", _bodySize);
			dePrint("Too Big.");
			return new char[allocSize + _bodySize];
			/*pTake = (BlockHeader*)malloc(allocSize + sizeof(BlockHeader));
			if (pTake != nullptr)
			{
				pTake->_Id = -1;
				pTake->_next = nullptr;
				pTake->_refCount = 1;
				pTake->_Pool = nullptr;
				dePrint("\tTake Block[id: %d, size: %llu, addr: %llX] from Pool[%llx]\n",
					pTake->_Id, allocSize, (ptr)pTake, (ptr)pTake->_Pool);
			}*/
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

		if (pReturn->_Id != -1)
		{
			/* ���ڴ����*/
			std::lock_guard<std::mutex> lock(_mutex);
			if (--pReturn->_refCount != 0)
				return;
			dePrint("\tReturn Block[id: %d, addr: %llX] to Pool[%d]\n",
				pReturn->_Id, (ptr)pReturn, _bodySize);
			pReturn->_next = _header;
			_header = pReturn;
		}
		else
		{
			dePrint("errorerrorerrorerrorerrorerrorerror.\n");
			/* �����ڴ����
			if (--pReturn->_refCount != 0)
				return;
			dePrint("\tReturn Block[id: %d, addr: %llX] to Pool[%llx]\n",
				pReturn->_Id, (ptr)pReturn, (ptr)pReturn->_Pool);
			free(pReturn); */
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
		dePrint(">>>>>Memory[%d]: [addr: %llX] amount:%d, Total = %dB.\n", 
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



	/*############## ��Ա�б� #####################*/
public:
	/* �ڴ�����ڴ浥Ԫ��ͷ����Ϣ */
	struct BlockHeader
	{
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
	std::mutex _mutex;
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
class MemoryManagerTools
{
private:
	MemoryManagerTools()
	{
		InitArray(1, 64, &_mem64);
		InitArray(65, 128, &_mem128);
		InitArray(129, 256, &_mem256);
		//dePrint("MemoryManager : Alloc Pool ing...\n");
	}

	~MemoryManagerTools()
	{
		//dePrint("MemoryManager : Free Pool ing...\n");
	}

	// ��ʼ���ڴ��ӳ������
	void InitArray(int begin, int end, MemoryPool* pMem)
	{
		for (int n = begin; n <= end; ++n)
		{
			_poolMem[n] = pMem;		// ��������Ĵ�С�����Ӧ���ڴ�ؽ�������
		}
	}

public:
	typedef MemoryPool::BlockHeader BlockUnitHeader;		// �ڴ浥Ԫ��ͷָ��

	/* ����ģʽ--ʹ������ϵͳ��ֻ��һ���ڴ������ */
	static MemoryManagerTools* Get_pInstance()
	{
		return &_Manager;
	}

	// ����ռ�-> ����allocSize�ģ���ϵͳ���룬�������Ӧ�ڴ������
	void* allocMemory(size_t allocSize)
	{
		if (allocSize <= MAX_ALLOC_SIZE)
		{
			// sizeС��MAX_ALLOC_SIZE��ͨ��poolMemӳ�䵽��Ӧ�ڴ����
			return _poolMem[allocSize]->TakeBlock(allocSize);
		}
		else
		{
			// �����ڴ�����������������������	
			BlockUnitHeader* pTake = (BlockUnitHeader*)malloc(allocSize + sizeof(BlockUnitHeader));
			if (pTake != nullptr)
			{
				pTake->_Id = -1;
				pTake->_refCount = 1;
				pTake->_Pool = nullptr;
				pTake->_next = nullptr;
				dePrint(" malloc [addr: %llX] [id: %d] [size: %llu] from [%llx].\n",
					(ptr)pTake, pTake->_Id, allocSize, (ptr)pTake->_Pool);
			}
			return ((char*)pTake + sizeof(BlockUnitHeader));
		}
	}

	// �����ڴ浥Ԫ��ͷ����Ϣ�������ڴ��/����ϵͳ�ͷſռ�
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
				//dePrint("\tReturn Block[id: %d, addr: %llX] to Pool[%llx]\n",
				//	pReturn->_Id, (ptr)pReturn, (ptr)pReturn->_Pool);
				dePrint(" free [addr: %llX] [id: %d] to [%llx].\n",
					(ptr)pReturn, pReturn->_Id, (ptr)pReturn->_Pool);
				free(pReturn);
			}
		}
	}


private:
	static MemoryManagerTools _Manager;			// ��̬��ԱMemoryManagerTools���󣬳���һ��ʼ��ʼ��
	MemoryPoolAlloctor<64, 100> _mem64;			// ���ʹ��ָ���Ա���򲻻��ʼ������Ҫ�ֶ����г�ʼ��
	MemoryPoolAlloctor<128, 100> _mem128;	
	MemoryPoolAlloctor<256, 100> _mem256;	
	//MemoryPoolAlloctor<512, 100000> _mem512;
	//MemoryPoolAlloctor<1024, 100000> _mem102
	//MemoryPoolAlloctor<1024, 1000> _mem64;	
	MemoryPool* _poolMem[MAX_ALLOC_SIZE + 1];		// ӳ������

};
//int a = sizeof(MemoryPoolAlloctor<6,19>);



#endif // !_MEMORYPOOL_HPP_

