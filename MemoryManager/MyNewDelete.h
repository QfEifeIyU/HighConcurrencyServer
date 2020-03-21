#ifndef _NEWDELOP_HPP_
#define _NEWDELOP_HPP_

#ifdef _DEBUG
	#include  <stdio.h>
	#define dePrint(...) printf(__VA_ARGS__)
#else
	#define dePrint(...)
#endif

/*  ÖØÔØnew ¡¢delete ²Ù×÷ÔËËã·û  */
void* operator new(size_t allocSize);
void* operator new[](size_t allocSize);

void operator delete(void* pMem);
void operator delete[](void* pMem);

#endif // !_NEWDELOP_HPP_

