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

// �ڴ����С��Ԫ
class BlockUnit
{
public:
	MemoryPool* _pool;	// �����ڴ��
	BlockUnit* _nextUnit;	// ��һ���ڴ浥Ԫ
	int _nID;		// id��
	int _nRef;		// ���ô���
	bool _inPool;	// �Ƿ��ڳ���
};


// �ڴ��
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
			// ��ǰ�ڴ��Ϊ�գ������ϵͳ���� �ڴ�+ͷ��Ϣ
			ret = (BlockUnit*)malloc(size + sizeof(BlockUnit));
			if (ret == nullptr)
			{
				// �ڴ治��
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
			// ���ڴ��ȡһ���飬ͷɾ
			ret = _headBlockUnit;
			_headBlockUnit = _headBlockUnit->_nextUnit;
			ret->_nRef = 1;
		}
		dePrint("alloc: %llx, id-%d, size-%d\n", ret, ret->_nID, size);
		return ((char*)ret + sizeof(BlockUnit));	// ֻ���ؿɲ����ڴ棬������ͷ��Ϣ
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
			// ���ڴ���ֱ��ͷ�帲��
			pUnit->_nextUnit = _headBlockUnit;
			_headBlockUnit = pUnit;
		}
		else 
		{
			// �����ڴ���У��ͷ�ͷ+ָ��
			free(pUnit);
		}
	}

	// ��ʼ���ڴ��ͷ��Ϣ
	void InitMemoryPool()
	{
		if (_pAddress != nullptr)
		{
			return;
		}

		size_t capacity = _blockSize + sizeof(BlockUnit);		// ͷ + �� �ܴ�С
		size_t poolSize = capacity * _blockAmount;
		_pAddress = (char*)malloc(poolSize);

		// ��ʼ���ڴ��
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
			// β��
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
	char* _pAddress;			// �ڴ�صĵ�ַ
	BlockUnit* _headBlockUnit;		// ͷ���ڴ浥Ԫ
	size_t _blockSize;			// �ڴ浥Ԫ�Ĵ�С
	size_t _blockAmount;	    // �ڴ浥Ԫ����
};


// ʹ��ģ��Ϊ�ڴ�ش����ڴ浥Ԫ�Ĵ�С������
template<size_t size, size_t amount>
class MemoryPoolAlloctor: public MemoryPool
{
public:
	MemoryPoolAlloctor()
	{
		const int aline = sizeof(void*);		// ������--�ṹ��Ĵ�С������Ա��������
		_blockSize = (size / aline) * aline + (size % aline ? aline : 0);		// �ڴ����;
		_blockAmount = amount;
	}
};


const int ALLOC_SIZE =  1024;
// �ڴ������
class MemoryManager
{
	/* ʹ�õ���ģʽ--���캯��˽�л� */
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

	// ��ʼ���ڴ��ӳ������
	void InitArray(int begin, int end, MemoryPool* pool)
	{
		for (int n = begin; n < end; ++n)
		{
			_poolMem[n] = pool;		// ��������Ĵ�С�����Ӧ���ڴ�ؽ�������
		}
	}

public:
	/* ����ģʽ--ʹ������ϵͳ��ֻ��һ���ڴ������ */
	static MemoryManager& Instance()
	{
		static MemoryManager manager;
		return manager;
	}

	void* allocMemory(size_t size)
	{
		if (size <= ALLOC_SIZE)
		{
			// sizeС��_mem64�ڴ�أ�ͨ��poolMemӳ�䵽64B�ڴ����
			return _poolMem[size]->allocMemory(size);
		}
		else
		{
			// �����ڴ����������������
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
			// ������ڴ������ڴ棬�����������ڴ���ͷ�
			pUnit->_pool->freeMemory(p);
		}
		else 
		{
			// ��������ڴ�ص��ڴ棬��Ϊ�����ʱ�����˵�Ԫ��Ϣ�������ͷ�BlockUnit����		
			if (--pUnit->_nRef == 0)
				free(pUnit);
		}		
	}


private:
	MemoryPoolAlloctor<64, 1000> _mem64;		// ����1000��С��  64B���ڴ��
	MemoryPoolAlloctor<128, 1000> _mem128;		// ����1000��С�� 128B���ڴ��
	MemoryPoolAlloctor<256, 1000> _mem256;		// ����1000��С�� 256B���ڴ��
	MemoryPoolAlloctor<512, 1000> _mem512;		// ����1000��С�� 512B���ڴ��
	MemoryPoolAlloctor<1024, 1000> _mem1024;		// ����1000��С��1024B���ڴ��
	//MemoryPoolAlloctor<1024, 10> _mem64;		// ����1000��С��1024B���ڴ��
	MemoryPool* _poolMem[ALLOC_SIZE];		// ӳ������
};



#endif // !_MEMORYMANAGER_HPP_

