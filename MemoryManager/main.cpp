#include "MemoryManager.hpp"
#include "Alloctor.h"


int main()
{
	
	char* arr[105];
	for (int i = 0; i < 105; ++i)
	{
		arr[i] = new char[1 + rand() % 1023];
	}
	for (int i = 0; i < 105; ++i)
	{
		delete[] arr[i];
	}

	
	return 0;
}


//char ch = 'a';
//for (int j = 0; j < 20; ++j)
//{
//	arr[i][j] = ch++;
//}