#include "tcpClient.hpp"
#include <thread>

static bool g_Cli_Running = true;
// 线程入口函数--构造请求
void th_route() 
{
	string request = "";
	cin >> request;
	if (request == "exit") 
	{
		g_Cli_Running = false;
		printf("线程 thread 退出\n");
		return;
	}
}

const int cli_count = 4000;		// 客户端总数
const int thr_count = 4;		// 根据cpu调整创建的线程数量
Login login(1.0);
void thr_SendMsg(int tid) 
{
	const int num = cli_count / thr_count;	// 每个线程创建的客户端数量
	TcpClient* cli_arr = new TcpClient[num]();		// new-> num个TcpClient对象数组

	for (int n = 0; n < num; ++n)
	{
		cli_arr[n].Connect("127.0.0.1", 0x4567);
		printf("<thr%d> 连接 cli{%d}\n", tid, n);
	}
	printf("<thr%d> 连接结束.cli{%d}\n", tid, num);

	while (g_Cli_Running)
	{
		for (int n = 0; n < num; ++n)
		{
			cli_arr[n].SendData(&login);
		}
	}

	for (int n = 0; n < num; ++n)
	{
		cli_arr[n].CleanUp();
	}
	delete[] cli_arr;
}


int main() 
{
	thread thr_ui(th_route);
	thr_ui.detach();

	for (int n = 0; n < thr_count; ++n) 
	{
		// 开辟线程循环发送消息
		thread thr_Send(thr_SendMsg, n + 1);
		thr_Send.detach();
	}
	system("pause");
	return 0;
}