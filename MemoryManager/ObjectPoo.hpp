#ifndef _OBJECTPOO_HPP_
#define _OBJECTPOO_HPP_

#include <mutex>
static std::mutex static_mutex1;

template<class Type, size_t objAmount>
class ObjectPool
{
public:
	ObjectPool()
	{
		InitObjectPool();
	}

	~ObjectPool()
	{
		if (_pAddr)
		{
			dePrint("[%s]: Pool free-> [addr: %llX].\n", typeid(Type).name(), (ptr)_pAddr);
			delete[] _pAddr;
		}
	}

	// 申请对象
	void* TakeObject(size_t allocSize)
	{
		std::lock_guard<std::mutex> _lock(static_mutex1);
		
		NodeHeader* pTake = nullptr;
		if (_header == nullptr)
		{
			/* 内存单元无---向系统申请*/
			//dePrint("objectPool is empty!!!\n");
			pTake = (NodeHeader*)new char[sizeof(Type) + sizeof(NodeHeader)];
			pTake->_Id = -1;
			++pTake->_refCount;
			pTake->_next = nullptr;
			
			//dePrint("\tmalloc [addr: %llX] [id: %d] [size: %llu] from Pool[%llX].\n",
				//pTake->_Id, allocSize, (ptr)pTake, (ptr)_pAddr);
		}
		else
		{
			/* 从内存单元链表中取 */
			pTake = _header;
			_header = _header->_next;
			++pTake->_refCount;
			dePrint("\tTake Object[id: %d, size: %llu, addr: %llX] from Pool[%s].\n",
				pTake->_Id, allocSize, (ptr)pTake, typeid(Type).name());
		}			
		return (char*)pTake + sizeof(NodeHeader);
	}

	// 释放对象
	void ReturnObject(void* pMem)
	{
		NodeHeader* pReturn = (NodeHeader*)((char*)pMem - sizeof(NodeHeader));
		if (pReturn->_Id != -1)
		{
			/* 在对象池中*/
			std::lock_guard<std::mutex> lock(static_mutex1);
			if (--pReturn->_refCount != 0)
				return;
			dePrint("\tReturn Object[id = %d, addr = %llX] to Pool[%s].\n", pReturn->_Id, (ptr)pReturn, typeid(Type).name());
			pReturn->_next = _header;
			_header = pReturn;
		}
		else
		{
			//MemoryPool::BlockHeader* pReturn = ()
			//if (--pReturn->_refCount != 0)
				//return;
			delete[] pReturn;
		}
	}

private:
	void InitObjectPool()
	{
		if (_pAddr != nullptr)
			return;

		// 申请一个对象池
		uint32_t nodeSize = sizeof(Type) + sizeof(NodeHeader);
		uint32_t poolSize = nodeSize * objAmount;
		_pAddr = new char[poolSize];
		dePrint("[%s]: Pool alloc-> [addr: %llX] [size: %llu] amount: %llu, Total = %dB.\n",
			typeid(Type).name(), (ptr)_pAddr, sizeof(Type), objAmount, poolSize);

		// 将内存池的块‘链’起来
		_header = (NodeHeader*)_pAddr;
		if (_header != nullptr)
		{
			_header->_Id = 0;
			_header->_next = nullptr;
			_header->_refCount = 0;
		}

		NodeHeader* curHeader = _header;
		for (uint32_t n = 1; n < objAmount; ++n)
		{
			// 计算下一个块的偏移位置并初始化头信息
			int offset = nodeSize * n;
			NodeHeader* nextHeader = (NodeHeader*)((char*)_pAddr + offset);
			if (nextHeader != nullptr)
			{
				nextHeader->_Id = n;
				nextHeader->_next = nullptr;
				nextHeader->_refCount = 0;
			}
			// ‘链’起来
			if (curHeader != nullptr)
			{
				curHeader->_next = nextHeader;
				curHeader = nextHeader;
			}
		}
		//dePrint("ObjectPool Init done!\n");
	}

private:
	struct NodeHeader
	{
		NodeHeader* _next;		// 下一个节点的信息
		short _Id;				// 对象节点id编号
		short _refCount;		// 对象节点引用计数
	};
	void* _pAddr;				// 对象池的首地址
	NodeHeader* _header;		// 对象池的头部--链表头
	/*   对象节点的头信息  */
};



/* 对象池对象 基类 */
template<class Type, size_t poolSize>
class ObjectPoolBase
{
public:
	void* operator new(size_t allocSize)
	{
		return ObjectPool().TakeObject(allocSize);
	}

	void operator delete(void* pMem)
	{
		ObjectPool().ReturnObject(pMem);
	}

	// 通过模板函数创建一个对象
	template<typename ... Args>
	static Type* createObject(Args ... args)
	{
		Type* obj = new Type(args...);
		return obj;
	}

	static void destroyObject(Type* obj)
	{
		delete obj;
	}


private:
	typedef ObjectPool<Type, poolSize> TypePool;

	/* 单例模式--使得整个系统中每个类型都有一个对象池 */
	static TypePool& ObjectPool()
	{
		static TypePool pool;
		return pool;
	}
};


#endif // !_OBJECTPOO_HPP_

