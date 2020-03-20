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
			dePrint("ObjectPool: free## ->pool = %p, size = %llu, amount = %llu.\n", _pAddr, sizeof(Type), objAmount);
			delete[] _pAddr;
		}
			
	}

	// �������
	void* TakeObject(size_t allocSize)
	{
		std::lock_guard<std::mutex> _lock(static_mutex1);
		
		NodeHeader* pTake = nullptr;
		if (_header == nullptr)
		{
			/* �ڴ浥Ԫ��---��ϵͳ����*/
			dePrint("objectPool is end!\n");
			pTake = (NodeHeader*)new char[sizeof(Type) + sizeof(NodeHeader)];
			pTake->_Id = -1;
			++pTake->_refCount;
			pTake->_next = nullptr;
		}
		else
		{
			/* ���ڴ浥Ԫ������ȡ */
			pTake = _header;
			_header = _header->_next;
			++pTake->_refCount;
		}
		if (pTake != nullptr)
			dePrint("\tTake Object[id = %d, size = %llu] from Pool[%p]\n", pTake->_Id, allocSize, this);
		return (char*)pTake + sizeof(NodeHeader);
	}

	// �ͷŶ���
	void ReturnObject(void* pMem)
	{
		NodeHeader* pReturn = (NodeHeader*)((char*)pMem - sizeof(NodeHeader));
		dePrint("\tReturn Object[id = %d] to Pool[%p]\n", pReturn->_Id, this);
		if (pReturn->_Id != -1)
		{
			/* ���ڴ����*/
			std::lock_guard<std::mutex> lock(static_mutex1);
			if (--pReturn->_refCount != 0)
				return;

			pReturn->_next = _header;
			_header = pReturn;
		}
		else
		{
			if (--pReturn->_refCount != 0)
				return;
			delete[] pReturn;
		}
	}
private:

	void InitObjectPool()
	{
		if (_pAddr != nullptr)
			return;

		// ����һ�������
		uint32_t nodeSize = sizeof(Type) + sizeof(NodeHeader);
		uint32_t poolSize = nodeSize * objAmount;
		_pAddr = new char[poolSize];
		dePrint("ObjectPool: alloc## ->pool = %p, size = %llu, amount = %llu.\n", _pAddr, sizeof(Type), objAmount);

		// ���ڴ�صĿ顮��������
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
			// ������һ�����ƫ��λ�ò���ʼ��ͷ��Ϣ
			int offset = nodeSize * n;
			NodeHeader* nextHeader = (NodeHeader*)((char*)_pAddr + offset);
			if (nextHeader != nullptr)
			{
				nextHeader->_Id = n;
				nextHeader->_next = nullptr;
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

private:
	struct NodeHeader
	{
		NodeHeader* _next;		// ��һ���ڵ����Ϣ
		short _Id;				// ����ڵ�id���
		short _refCount;		// ����ڵ����ü���
	};
	void* _pAddr;				// ����ص��׵�ַ
	NodeHeader* _header;		// ����ص�ͷ��--����ͷ
	/*   ����ڵ��ͷ��Ϣ  */
};



/* ����ض��� ���� */
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

	// ͨ��ģ�庯������һ������
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
	static TypePool& ObjectPool()
	{
		static TypePool pool;
		return pool;
	}
};


#endif // !_OBJECTPOO_HPP_

