#ifndef _Tools_hpp
#define	_Tools_hpp

/* 网络模块 */
#ifdef _WIN32
	#define FD_SETSIZE	1024
	#define WIN32_LEAN_AND_MEAN			// 避免windows库和WinSock2库的宏重定义
	#define _WINSOCK_DEPRECATED_NO_WARNINGS			// itoa转换函数版本过低
	#include <windows.h>
	#include <WinSock2.h>
#else
	#include <unistd.h>     // std 
	#include <arpa/inet.h>  // socket
	typedef int SOCKET;     // 适应sockfd
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif 


#ifdef _WIN32
	// 搭建windows socket 2.x环境
	void StartUp()
	{
		WORD ver = MAKEWORD(2, 2);
		WSADATA data;
		int ret = WSAStartup(ver, &data);
		if (ret < 0)
			{}
	}
	void CleanNet()
	{
		int ret = WSACleanup();
		if (ret < 0)
			{}
	}
#endif

// 设置地址信息
void SetAddr(sockaddr_in* addr, const char* ip, const unsigned short port)
{
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
#ifdef _WIN32 
	addr->sin_addr.S_un.S_addr = inet_addr(ip);
#else
	addr->sin_addr.s_addr = inet_addr(ip);
#endif
}


/* 2.多线程模块 */
#include <thread>
//const int TIME_AWAKE = 1;	// select轮询时间
const int RECVBUFSIZE = 10240;		// 接收缓冲区大小
const int CPU_THREAD_AMOUNT = 4;		// 根据cpu调整创建的线程数量

#include <iostream>
#include <string>

// 控制程序运行状态
bool g_TaskRuning = true;
void th_route()
{
	std::string request = "";
	std::cin >> request;
	if (request == "exit")
	{
		g_TaskRuning = false;
		printf("线程 thread 退出\n");
	}
	return;
}

#endif	/* end of Tools.hpp */