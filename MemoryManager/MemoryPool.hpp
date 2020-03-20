#ifndef _MEMORYPOOL_HPP_
#define _MEMORYPOOL_HPP_

#include <mutex>
#include "NEWDELop.h"
class MemoryPool;

/* �ڴ�����ڴ浥Ԫ��ͷ����Ϣ */
class BlockHeader
{
public:
	/* sizeof(BlockHeader) = 24 */
	MemoryPool* _Pool;		// �����ڴ��
	BlockHeader* _next;		// ��һ���ڴ浥Ԫ����Ϣ
	short _Id;				// �ڴ浥Ԫid���
	short _refCount;		// �ڴ浥Ԫ���ü���
};

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
			free(_pAddr);
		//dePrint("MemoryPool: free~\n");
	}

	void* TakeBlock(size_t allocSize)
	{
		std::lock_guard<std::mutex> lock(static_mutex);
		if (_pAddr == nullptr)
			InitMemoryPool();

		BlockHeader* pTake = nullptr;
		if (_header == nullptr)
		{
			/* �ڴ浥Ԫ��---��ϵͳ����*/
			dePrint("memoryPool is end!\n");
			pTake = (BlockHeader*)malloc(allocSize + sizeof(BlockHeader));
			if (pTake != nullptr)
			{
				pTake->_Id = -1;
				pTake->_next = nullptr;
				pTake->_refCount = 1;
				pTake->_Pool = nullptr;
			}
		}
		else 
		{
			/* ���ڴ浥Ԫ������ȡ */
			pTake = _header;
			_header = _header->_next;
			++pTake->_refCount;
		}
		if (pTake != nullptr)
			dePrint("\tTake Block[id = %d, size = %llu] from Pool[%p]\n", pTake->_Id, allocSize, pTake->_Pool);
		return (char*)pTake + sizeof(BlockHeader);
	}

	void ReturnBlock(void* pMem)
	{
		
		BlockHeader* pReturn = (BlockHeader*)((char*)pMem - sizeof(BlockHeader));
		dePrint("\tReturn Block[id = %d] to Pool[%p]\n", pReturn->_Id, pReturn->_Pool);
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

private:
	void InitMemoryPool()
	{
		if (_pAddr != nullptr)
			return;

		// ����һ���ڴ��
		uint32_t blockSize = _bodySize + sizeof(BlockHeader);
		uint32_t poolSize = blockSize * _blockAmount;
		_pAddr = malloc(poolSize);
		dePrint("MemoryPool: alloc## size = %d, amount = %d.\n", _bodySize, _blockAmount);

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
			// ����������
			if (curHeader != nullptr)
			{
				curHeader->_next = nextHeader;
				curHeader = nextHeader;
			}
		}
	}

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
		//InitArray(256, 512, &_mem512);
		//InitArray(512, 1024, &_mem1024);
		dePrint("MemoryManager : Alloc Pool64 ��Pool126�� Pool256...\n");
	}
	~MemoryManager()
	{
		dePrint("MemoryManager : Free Pool64 ��Pool126�� Pool256...\n");
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
	/* ����ģʽ--ʹ������ϵͳ��ֻ��һ���ڴ������ */
	static MemoryManager& Instance()
	{
		static MemoryManager manager;
		return manager;
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
			BlockHeader* ret = (BlockHeader*)malloc(allocSize + sizeof(BlockHeader));
			if (ret != nullptr)
			{
				ret->_Id = -1;
				ret->_refCount = 1;
				ret->_Pool = nullptr;
				ret->_next = nullptr;
				dePrint("\talloc: %p, id = %d, size = %llu\n", ret, ret->_Id, allocSize);
			}
			return ((char*)ret + sizeof(BlockHeader));
		}
	}


	void freeMemory(void* pMem)
	{
		BlockHeader* pUnit = reinterpret_cast<BlockHeader*>((char*)pMem - sizeof(BlockHeader));

		if (pUnit->_Id != -1)
		{
			// ������ڴ������ڴ棬�����������ڴ���ͷ�
			pUnit->_Pool->ReturnBlock(pMem);
		}
		else
		{
			// ��������ڴ�ص��ڴ棬��Ϊ�����ʱ�����˵�Ԫ��Ϣ�������ͷ�BlockUnit����		
			if (--pUnit->_refCount == 0)
			{
				dePrint("\tfree: %p, id = %d, ref = %d.\n", pUnit, pUnit->_Id, pUnit->_refCount);
				free(pUnit);
			}
		}
	}


private:
	MemoryPoolAlloctor<64, 10> _mem64;		// ����1000��С��  64B���ڴ��
	MemoryPoolAlloctor<128, 100> _mem128;		// ����1000��С�� 128B���ڴ��
	MemoryPoolAlloctor<256, 100> _mem256;		// ����1000��С�� 256B���ڴ��
	//MemoryPoolAlloctor<512, 100000> _mem512;		// ����1000��С�� 512B���ڴ��
	//MemoryPoolAlloctor<1024, 100000> _mem1024;		// ����1000��С��1024B���ڴ��
	//MemoryPoolAlloctor<1024, 10> _mem64;		// ����1000��С��1024B���ڴ��
	MemoryPool* _poolMem[MAX_ALLOC_SIZE];		// ӳ������
};



#endif // !_MEMORYPOOL_HPP_

