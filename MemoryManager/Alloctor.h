#ifndef _ALLOCTOR_H_
#define _ALLOCTOR_H_

#include "MemoryManager.hpp"

void* operator new(size_t size);
void* operator new[](size_t size);

void operator delete(void* p);
void operator delete[](void* p);


#endif // !_ALLOCTOR_H_
