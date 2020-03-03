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

	s.Bind("192.168.0.103", 0x4567);	// local
	//s.Bind("127.0.0.1", 0x4567);	// local
	//s.Bind("172.19.119.190", 0x4567);		// aliyun
	//s.Bind("192.168.187.129", 0x4567);		// virtual Ubuntu

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
