#include "MemoryPool.hpp"
#include "ObjectPoo.hpp"

class classTest : public ObjectPoolBase<classTest, 10>
{
public:
	classTest()
	{
		a = 0;
		//dePrint("\t\tcreate classTest. \n");
	}
	classTest(int n)
	{
		a = n;
		//dePrint("\t\tcreate classTest.a = 100, \n");
	}
	~classTest()
	{
		//dePrint("\t\tdestory classTest. \n");
	}

private:
	int a;
};

/*
class classB : public ObjectPoolBase<classB, 1000>
{
public:
	classB()
	{
		dePrint("\t\tcreate B. \n");
	}
	classB(int n)
	{
		b = n;
		dePrint("\t\tcreate B.a = 100, \n");
	}
	~classB()
	{
		dePrint("\t\tdestory B. \n");
	}

private:
	int b;
};
*/

// 饿汉模式使得整个程序中只有一份内存管理工具
MemoryManagerTools MemoryManagerTools::_Manager;


int main()
{
	
	char* data[20];

	for (int i = 0; i < 20; ++i)
	{
		data[i] = new char[10 * i + 5];
	}
	for (int i = 0; i < 20; ++i)
	{
		delete data[i];
	}
	

	/*
	classA* a1 = new classA(5);
	classA* a2 = classA::createObject(6);
	classA::destroyObject(a2);
	delete a1;


	classB* b1 = new classB(5);
	classB* b2 = classB::createObject(6);
	classB::destroyObject(b2);
	delete b1;
	*/
	
	//classTest* dt[12];

	//for (int i = 0; i < 12; ++i)
	//{
	//	dt[i] = new classTest(3);
	//}
	//for (int i = 11; i >= 0; --i)
	//{
	//	delete dt[i];
	//}
	//

	return 0;
}