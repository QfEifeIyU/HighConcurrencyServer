#ifndef _MEMORYMANAGER_HPP_
#define _MEMORYMANAGER_HPP_

#include <stdlib.h>		// 

#ifdef _DEBUG
	#include  <stdio.h>
	#define dePrint(...) printf(__VA_ARGS__)
#elif
	#define dePrint(...)
#endif




class MemoryPool;

// 内存块最小单元
class BlockUnit
{
public:
	MemoryPool* _pool;	// 所属内存池
	BlockUnit* _nextUnit;	// 下一块内存单元
	int _nID;		// id号
	int _nRef;		// 引用次数
	bool _inPool;	// 是否在池中
};


// 内存池
class MemoryPool
{
public:
	MemoryPool()
	{
		_pAddress = nullptr;
		_headBlockUnit = nullptr;
		_blockSize = 0;
		_blockAmount = 0;
	}

	~MemoryPool()
	{
		if (_pAddress)
		{
			free(_pAddress);
		}
	}

	void* allocMemory(size_t size)
	{
		if (!_pAddress)
		{
			InitMemoryPool();
		}

		BlockUnit* ret = nullptr;
		if (_headBlockUnit == nullptr)
		{
			// 当前内存池为空，向操作系统申请 内存+头信息
			ret = (BlockUnit*)malloc(size + sizeof(BlockUnit));
			if (ret == nullptr)
			{
				// 内存不足
			}
			else{
				ret->_inPool = false;
				ret->_nID = -1;
				ret->_nRef = 1;
				ret->_pool = nullptr;
				ret->_nextUnit = nullptr;
			}
		}
		else 
		{
			// 从内存池取一个块，头删
			ret = _headBlockUnit;
			_headBlockUnit = _headBlockUnit->_nextUnit;
			ret->_nRef = 1;
		}
		dePrint("alloc: %llx, id-%d, size-%d\n", ret, ret->_nID, size);
		return ((char*)ret + sizeof(BlockUnit));	// 只返回可操作内存，不反悔头信息
	}

	void freeMemory(void* p)
	{
		BlockUnit* pUnit = reinterpret_cast<BlockUnit*>((char*)p - sizeof(BlockUnit));
		if (--pUnit->_nRef != 0)
		{
			return;
		}
		if (pUnit->_inPool)
		{
			// 在内存池里，直接头插覆盖
			pUnit->_nextUnit = _headBlockUnit;
			_headBlockUnit = pUnit;
		}
		else 
		{
			// 不在内存池中，释放头+指针
			free(pUnit);
		}
	}

	// 初始化内存池头信息
	void InitMemoryPool()
	{
		if (_pAddress != nullptr)
		{
			return;
		}

		size_t capacity = _blockSize + sizeof(BlockUnit);		// 头 + 块 总大小
		size_t poolSize = capacity * _blockAmount;
		_pAddress = (char*)malloc(poolSize);

		// 初始化内存池
		_headBlockUnit = reinterpret_cast<BlockUnit*>(_pAddress);
		if (_headBlockUnit != nullptr)
		{
			_headBlockUnit->_inPool = true;
			_headBlockUnit->_nID = 0;
			_headBlockUnit->_nRef = 0;
			_headBlockUnit->_pool = this;
			_headBlockUnit->_nextUnit = nullptr;
		}
		
		BlockUnit* cur = _headBlockUnit;
		for (size_t n = 1; n < _blockAmount; ++n)
		{
			// 尾插
			BlockUnit* next = reinterpret_cast<BlockUnit*>(_pAddress + n * capacity);
			if (next != nullptr)
			{
				next->_inPool = true;
				next->_nextUnit = nullptr;
				next->_nID = n;
				next->_nRef = 0;
				next->_pool = this;

				cur->_nextUnit = next;
				cur = next;
			}
		}
	}

protected:
	char* _pAddress;			// 内存池的地址
	BlockUnit* _headBlockUnit;		// 头部内存单元
	size_t _blockSize;			// 内存单元的大小
	size_t _blockAmount;	    // 内存单元数量
};


// 使用模板为内存池传递内存单元的大小和数量
template<size_t size, size_t amount>
class MemoryPoolAlloctor: public MemoryPool
{
public:
	MemoryPoolAlloctor()
	{
		const int aline = sizeof(void*);		// 对齐数--结构体的大小是最大成员的整数倍
		_blockSize = (size / aline) * aline + (size % aline ? aline : 0);		// 内存对齐;
		_blockAmount = amount;
	}
};


const int ALLOC_SIZE =  1024;
// 内存管理工具
class MemoryManager
{
	/* 使用单例模式--构造函数私有化 */
private:
	MemoryManager()
	{
		InitArray(0, 64, &_mem64);
		InitArray(64, 128, &_mem128);
		InitArray(128, 256, &_mem256);
		InitArray(256, 512, &_mem512);
		InitArray(512, 1024, &_mem1024);
		//InitArray(1024, 1024, &_mem64);
	}
	~MemoryManager()
	{}

	// 初始化内存池映射数组
	void InitArray(int begin, int end, MemoryPool* pool)
	{
		for (int n = begin; n < end; ++n)
		{
			_poolMem[n] = pool;		// 根据申请的大小，向对应的内存池进行申请
		}
	}

public:
	/* 单例模式--使得整个系统中只有一份内存管理类 */
	static MemoryManager& Instance()
	{
		static MemoryManager manager;
		return manager;
	}

	void* allocMemory(size_t size)
	{
		if (size <= ALLOC_SIZE)
		{
			// size小于_mem64内存池，通过poolMem映射到64B内存池中
			return _poolMem[size]->allocMemory(size);
		}
		else
		{
			// 大于内存池数量，额外申请
			BlockUnit* ret = (BlockUnit*)malloc(size + sizeof(BlockUnit));
			ret->_inPool = false;
			ret->_nID = -1;
			ret->_nRef = 1;
			ret->_pool = nullptr;
			ret->_nextUnit = nullptr;

			dePrint("alloc: %llx, id-%d, size-%d\n", ret, ret->_nID, size);
			return ((char*)ret + sizeof(BlockUnit));
		}
	}
	
	
	void freeMemory(void* p)
	{
		BlockUnit* pUnit = reinterpret_cast<BlockUnit*>((char*)p - sizeof(BlockUnit));
		dePrint("free: %llx, id-%d\n", pUnit, pUnit->_nID);
		if (pUnit->_inPool)
		{
			// 如果是内存池里的内存，交给所属的内存池释放
			pUnit->_pool->freeMemory(p);
		}
		else 
		{
			// 如果不是内存池的内存，因为申请的时候做了单元信息，所以释放BlockUnit类型		
			if (--pUnit->_nRef == 0)
				free(pUnit);
		}		
	}


private:
	MemoryPoolAlloctor<64, 1000> _mem64;		// 申请1000个小于  64B的内存池
	MemoryPoolAlloctor<128, 1000> _mem128;		// 申请1000个小于 128B的内存池
	MemoryPoolAlloctor<256, 1000> _mem256;		// 申请1000个小于 256B的内存池
	MemoryPoolAlloctor<512, 1000> _mem512;		// 申请1000个小于 512B的内存池
	MemoryPoolAlloctor<1024, 1000> _mem1024;		// 申请1000个小于1024B的内存池
	//MemoryPoolAlloctor<1024, 10> _mem64;		// 申请1000个小于1024B的内存池
	MemoryPool* _poolMem[ALLOC_SIZE];		// 映射数组
};



#endif // !_MEMORYMANAGER_HPP_

