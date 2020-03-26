#ifndef _MEMORYPOOL_HPP_
#define _MEMORYPOOL_HPP_

#include <mutex>
#include "MyNewDelete.h"
class MemoryPool;
typedef unsigned __int64 ptr;		// 打印地址


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
			dePrint(">>>>>Memory[%d]: [addr: %llX].\n", _bodySize, (ptr)_pAddr);
			free(_pAddr);
		}	
		
	}

	// 分配块
	void* TakeBlock(size_t allocSize)
	{
		std::lock_guard<std::mutex> lock(_mutex);		// 加锁保证线程安全

		BlockHeader* pTake = nullptr;
		if (_header == nullptr)
		{
			/* 内存池中可用内存块空了---向更大的内存池申请*/
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

		if (pReturn->_Id != -1)
		{
			/* 在内存池中*/
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
			/* 不在内存池中
			if (--pReturn->_refCount != 0)
				return;
			dePrint("\tReturn Block[id: %d, addr: %llX] to Pool[%llx]\n",
				pReturn->_Id, (ptr)pReturn, (ptr)pReturn->_Pool);
			free(pReturn); */
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
		dePrint(">>>>>Memory[%d]: [addr: %llX] amount:%d, Total = %dB.\n", 
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



	/*############## 成员列表 #####################*/
public:
	/* 内存池中内存单元的头部信息 */
	struct BlockHeader
	{
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
	std::mutex _mutex;
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

	// 初始化内存池映射数组
	void InitArray(int begin, int end, MemoryPool* pMem)
	{
		for (int n = begin; n <= end; ++n)
		{
			_poolMem[n] = pMem;		// 根据申请的大小，向对应的内存池进行申请
		}
	}

public:
	typedef MemoryPool::BlockHeader BlockUnitHeader;		// 内存单元的头指针

	/* 单例模式--使得整个系统中只有一份内存管理类 */
	static MemoryManagerTools* Get_pInstance()
	{
		return &_Manager;
	}

	// 申请空间-> 大于allocSize的，向系统申请，否则向对应内存池申请
	void* allocMemory(size_t allocSize)
	{
		if (allocSize <= MAX_ALLOC_SIZE)
		{
			// size小于MAX_ALLOC_SIZE，通过poolMem映射到对应内存池中
			return _poolMem[allocSize]->TakeBlock(allocSize);
		}
		else
		{
			// 大于内存池最大申请数量，额外申请	
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

	// 根据内存单元的头部信息，交给内存池/操作系统释放空间
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
				//dePrint("\tReturn Block[id: %d, addr: %llX] to Pool[%llx]\n",
				//	pReturn->_Id, (ptr)pReturn, (ptr)pReturn->_Pool);
				dePrint(" free [addr: %llX] [id: %d] to [%llx].\n",
					(ptr)pReturn, pReturn->_Id, (ptr)pReturn->_Pool);
				free(pReturn);
			}
		}
	}


private:
	static MemoryManagerTools _Manager;			// 静态成员MemoryManagerTools对象，程序一开始初始化
	MemoryPoolAlloctor<64, 100> _mem64;			// 如果使用指针成员，则不会初始化，需要手动进行初始化
	MemoryPoolAlloctor<128, 100> _mem128;	
	MemoryPoolAlloctor<256, 100> _mem256;	
	//MemoryPoolAlloctor<512, 100000> _mem512;
	//MemoryPoolAlloctor<1024, 100000> _mem102
	//MemoryPoolAlloctor<1024, 1000> _mem64;	
	MemoryPool* _poolMem[MAX_ALLOC_SIZE + 1];		// 映射数组

};
//int a = sizeof(MemoryPoolAlloctor<6,19>);



#endif // !_MEMORYPOOL_HPP_

