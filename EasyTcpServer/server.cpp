#include "tcpServer.hpp"
#include <thread>
#include <string>
#include <iostream>
// 线程入口函数--构造请求
static bool g_Ser_Running = true;
void th_route()
{
	std::string request = "";
	std::cin >> request;
	if (request == "exit")
	{
		g_Ser_Running = false;
		printf("线程 thread 退出\n");
	}
	return;
}


int main() {

	TcpServer s;
	//s.InitSocket();
#ifdef _WIN32
	s.Bind("127.0.0.1", 0x4567);
#else
	s.Bind("172.19.119.190", 0x4567);
#endif
	s.Listen(5);
	std::thread t(th_route);
	t.detach();
	while (g_Ser_Running) {
		s.StartSelect();
	}
	s.CleanUp();
	getchar();
	return 0;
}
