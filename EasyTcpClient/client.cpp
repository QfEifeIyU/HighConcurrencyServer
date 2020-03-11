//#include "tcpClient.hpp"
//#include <atomic>
//
//const int cli_count = 5000;		// 客户端总数
//std::atomic_int ready(0);
//static Login login[1];
//static const size_t size = sizeof(login);
//
//// 高频发送请求包线程
//void thr_SendMsg(int tid) 
//{
//	const int num = cli_count / CPU_THREAD_AMOUNT;	// 每个线程创建的客户端数量
//	
//	EasyTcpClient* cli_arr = new EasyTcpClient[num]();		
//	for (int n = 0; n < num; ++n)
//	{
//		cli_arr[n].Connect("127.0.0.1", 0x4567);
//		//cli_arr[n].Connect("106.15.187.148", 0x4567);
//		//printf("<thr%d> 连接 cli{%d}\n", tid, n);
//	}
//	printf("<thr%d> 连接结束.cli{%d} \n", tid, num);
//	++ready;
//
//	while (ready < 4) 
//	{
//		std::chrono::milliseconds time(500);
//		std::this_thread::sleep_for(time);
//		std::cout << "isready\n";
//	}
//
//
//	while (g_TaskRuning)
//	{
//		for (int n = 0; n < num; ++n)
//		{
//			cli_arr[n].SendData(login, size);
//			cli_arr[n].ActTask();
//		}
//	}
//
//	for (int n = 0; n < num; ++n)
//	{
//		cli_arr[n].CleanUp();
//	}
//	delete[] cli_arr;
//}
//
//
//int main() 
//{
//	std::thread thr_ui(th_route);
//	
//
//	for (int n = 0; n < CPU_THREAD_AMOUNT; ++n)
//	{
//		// 开辟线程循环发送消息
//		std::thread thr_Send(thr_SendMsg, n + 1);
//		thr_Send.detach();
//	}
//	thr_ui.join();
//	return 0;
//}


#include <iostream>
int main() 
{
	long long sum = 1;
	for (int i = 0; i < 64; ++i)
	{
		for (int j = 0; j < 100; ++j)
		{
			sum++;
		}
	}
	getchar();
	return 0;
}