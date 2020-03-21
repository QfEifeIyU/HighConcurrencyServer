#ifndef _MEMORYPOOL_HPP_
#define _MEMORYPOOL_HPP_

#include <mutex>
#include "MyNewDelete.h"
class MemoryPool;
typedef unsigned __int64 ptr;		// 打印地址
static std::mutex static_mutex;

/* 内存池 */
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

	// 分配块
	void* TakeBlock(size_t allocSize)
	{
		std::lock_guard<std::mutex> lock(static_mutex);		// 加锁保证线程安全

		BlockHeader* pTake = nullptr;
		if (_header == nullptr)
		{
			/* 内存池中可用内存块空了---向系统申请*/
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
			/* 从内存单元链表中取 */
			pTake = _header;
			_header = _header->_next;
			++pTake->_refCount;
			dePrint("\tTake Block[id: %d, size: %llu, addr: %llX] from Pool[%d]\n",
				pTake->_Id, allocSize, (ptr)pTake, _bodySize);
		}
		return (char*)pTake + sizeof(BlockHeader);
	}

	// 归还块
	void ReturnBlock(void* pMem)
	{
		
		BlockHeader* pReturn = (BlockHeader*)((char*)pMem - sizeof(BlockHeader));
		dePrint("\tReturn Block[id: %d, addr: %llX] to Pool[%d]\n", 
			pReturn->_Id, (ptr)pReturn, _bodySize);
		if (pReturn->_Id != -1)
		{
			/* 在内存池中*/
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

	// 申请一个内存池，并使用头部节点将所有内存块链起来
	void InitMemoryPool()
	{
		if (_pAddr != nullptr)
			return;

		// 申请一个内存池
		uint32_t blockSize = _bodySize + sizeof(BlockHeader);
		uint32_t poolSize = blockSize * _blockAmount;
		_pAddr = malloc(poolSize);
		dePrint(">>>>>Memory[%4dPool] alloc-> [addr: %llX] amount:%d, Total = %dB.\n", 
			_bodySize, (ptr)_pAddr, _blockAmount, poolSize);

		// 将内存池的块‘链’起来
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
			// 计算下一个块的偏移位置并初始化头信息
			int offset = blockSize * n;
			BlockHeader* nextHeader = (BlockHeader*)((char*)_pAddr + offset);	
			if (nextHeader != nullptr)
			{
				nextHeader->_Id = n;
				nextHeader->_next = nullptr;
				nextHeader->_Pool = this;
				nextHeader->_refCount = 0;
			}
			// 开始‘链’起来
			if (curHeader != nullptr)
			{
				curHeader->_next = nextHeader;
				curHeader = nextHeader;
			}
		}
	}


public:
	/* 内存池中内存单元的头部信息 */
	class BlockHeader
	{
	public:
		/*int a = sizeof(BlockHeader); // 24; */
		MemoryPool* _Pool;		// 所属内存池
		BlockHeader* _next;		// 下一个内存单元的信息
		short _Id;				// 内存单元id编号
		short _refCount;		// 内存单元引用计数
	};

protected:
	/* 	sizeof(MemoryPool) = 24 */
	void* _pAddr;			// 内存池的首地址
	BlockHeader* _header;	// 内存单元的头部--链表头
	uint32_t _bodySize;		// 内存单元的大小 - 头部
	uint32_t _blockAmount;	// 内存单元的数量
};


/* 使用模板技术给内存池分配大小 */
template <int size, int amount>
class MemoryPoolAlloctor: public MemoryPool
{
public:
	MemoryPoolAlloctor()
	{
		// 派生类对象构造时，默认首先调用基类构造
		const int num = sizeof(void*);		// 对齐数--结构体的大小是最大成员的整数倍
		_bodySize = (size / num) * num + (size % num ? num : 0);;
		_blockAmount = amount;
		InitMemoryPool();
		//dePrint("pool Alloc: %d - %d\n", size, amount);
	}
};


/* 进行内存池的申请 */
const int MAX_ALLOC_SIZE = 256;
class MemoryManager
{
	/* 使用单例模式--构造函数私有化 */
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

	// 初始化内存池映射数组
	void InitArray(int begin, int end)
	{

		for (int n = begin; n < end; ++n)
		{
			if (n <= 64)
				_poolMem[n] = &_mem64;
			else if (n <= 128)
				_poolMem[n] = &_mem128;
			else
				_poolMem[n] = &_mem256;		// 根据申请的大小，向对应的内存池进行申请
		}
	}

public:
	typedef MemoryPool::BlockHeader BlockUnitHeader;		// 内存单元的头指针

	/* 单例模式--使得整个系统中只有一份内存管理类 */
	static MemoryManager& Instance_Singleton()
	{
		static MemoryManager static_mgr;
		return static_mgr;
	}

	void* allocMemory(size_t allocSize)
	{
		if (allocSize <= MAX_ALLOC_SIZE)
		{
			// size小于MAX_ALLOC_SIZE，通过poolMem映射到对应内存池中
			return _poolMem[allocSize - 1]->TakeBlock(allocSize);
		}
		else
		{
			// 大于内存池数量，额外申请	
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
			// 如果是内存池里的内存，交给所属的内存池释放
			pReturn->_Pool->ReturnBlock(pMem);
		}
		else
		{
			// 如果不是内存池的内存，因为申请的时候做了单元信息，所以释放BlockUnit类型		
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
	MemoryPool* _poolMem[MAX_ALLOC_SIZE];		// 映射数组
};



#endif // !_MEMORYPOOL_HPP_

