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

	TcpServer* s = new TcpServer();
	s->Bind("127.0.0.1", 0x4567);
	s->Listen(5);
	s->CreateHandleMessageProcess();		// 创建线程微程序

	std::thread ui(th_route);
	ui.detach();
	
	while (g_Ser_Running) 
	{
		s->StartSelect();
	}
	s->CleanUp();

	return 0;
}
