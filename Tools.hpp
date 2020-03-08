#ifndef _Tools_hpp
#define	_Tools_hpp

/* ����ģ�� */
#ifdef _WIN32
	#define FD_SETSIZE	1024
	#define WIN32_LEAN_AND_MEAN			// ����windows���WinSock2��ĺ��ض���
	#define _WINSOCK_DEPRECATED_NO_WARNINGS			// itoaת�������汾����
	#include <windows.h>
	#include <WinSock2.h>
#else
	#include <unistd.h>     // std 
	#include <arpa/inet.h>  // socket
	typedef int SOCKET;     // ��Ӧsockfd
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif 


#ifdef _WIN32
	// �windows socket 2.x����
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

// ���õ�ַ��Ϣ
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


/* 2.���߳�ģ�� */
#include <thread>
//const int TIME_AWAKE = 1;	// select��ѯʱ��
const int RECVBUFSIZE = 10240;		// ���ջ�������С
const int CPU_THREAD_AMOUNT = 4;		// ����cpu�����������߳�����

#include <iostream>
#include <string>

// ���Ƴ�������״̬
bool g_TaskRuning = true;
void th_route()
{
	std::string request = "";
	std::cin >> request;
	if (request == "exit")
	{
		g_TaskRuning = false;
		printf("�߳� thread �˳�\n");
	}
	return;
}

#endif	/* end of Tools.hpp */