#include "tcpClient.hpp"


static bool g_Cli_Running = true;
const int cli_count = 12;		// 客户端总数

static Login login[10];
static const size_t size = sizeof(login);
// 高频发送请求包线程
void thr_SendMsg(int tid) 
{
	const int num = cli_count / CPU_THREAD_AMOUNT;	// 每个线程创建的客户端数量
	
	TcpClient* cli_arr = new TcpClient[num]();		
	for (int n = 0; n < num; ++n)
	{
		cli_arr[n].Connect("127.0.0.1", 0x4567);
		//printf("<thr%d> 连接 cli{%d}\n", tid, n);
	}
	printf("<thr%d> 连接结束.cli{%d}\n", tid, num);

	std::chrono::milliseconds time(3000);
	std::this_thread::sleep_for(time);

	while (g_TaskRuning)
	{
		for (int n = 0; n < num; ++n)
		{
			cli_arr[n].SendData(login, size);
			cli_arr[n].ActTask();
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
	std::thread thr_ui(th_route);
	thr_ui.detach();

	for (int n = 0; n < CPU_THREAD_AMOUNT; ++n)
	{
		// 开辟线程循环发送消息
		std::thread thr_Send(thr_SendMsg, n + 1);
		thr_Send.detach();
	}
	while (true)
		;
	return 0;
}