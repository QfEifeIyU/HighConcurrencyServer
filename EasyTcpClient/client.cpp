#include "tcpClient.hpp"
#include <thread>
// 线程入口函数--构造请求
static bool g_Cli_Running = true;
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

int main() 
{
	const int cli_count = FD_SETSIZE - 1;
	TcpClient* cli_arr[cli_count];
	int a = sizeof(cli_arr);
	for (auto& e: cli_arr) 
	{
		e = new TcpClient();
#ifdef _WIN32
		e->Connect("127.0.0.1", 0x4567);
		//e.Connect("106.15.187.148", 0x4567);
#else
		e->Connect("106.15.187.148", 0x4567);
#endif	
	}
	std::thread t(th_route);
	t.detach();

	Login login;
	strcpy(login._userName, "lyb");
	strcpy(login._passwd, "123456");

	while (g_Cli_Running)
	{
		for (auto e : cli_arr)
		{
			e->SendData(&login);
			e->StartSelect();
		}
	}	
	for (auto e: cli_arr)
		e->CleanUp();

	getchar();
	return 0;
}