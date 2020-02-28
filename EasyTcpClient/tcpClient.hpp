#pragma once
#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS			// scanf安全性
	#include "../HelloSocket/MessageType.hpp"
#else
	#include "MessageType.hpp"
#endif	// _WIN32
        
#include <iostream>
#include <string.h>

using namespace std;
class TcpClient
{
public:
	TcpClient()
		:_sockfd(INVALID_SOCKET)
	{}
	virtual ~TcpClient()
	{
		CleanUp();
	}
	SOCKET GetFd() 
	{
		return _sockfd;
	}
	// 初始化Socket
	void InitSocket()
	{
		// 初始化windows socket 2.x环境
#ifdef _WIN32 
		StartUp();
#endif
		if (_sockfd != INVALID_SOCKET)
		{
			printf("<%d>关闭旧的服务器连接.\n", _sockfd);
			CleanUp();
		}
		_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_sockfd == INVALID_SOCKET)
		{
			printf("错误的SOCKET.\n");
		}
		printf("SOCKET<%d>创建成功.\n", _sockfd);
	}
	// 连接服务器
	void Connect(const char* ip, const unsigned short port)
	{
		if (_sockfd == INVALID_SOCKET)
		{
			InitSocket();
		}
		sockaddr_in addr = {};
		SetAddr(&addr, ip, port);
#ifdef _WIN32
		int addrLen = sizeof(sockaddr_in);
#else
		socklen_t addrLen = sizeof(sockaddr_in);
#endif
		if (connect(_sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr_in)) == INVALID_SOCKET)
		{
			printf("连接服务器失败\n");
			CleanUp();
		}
		else 
		{
			printf("<%d>成功连接到服务器<$%s : %d>...\n", _sockfd, ip, port);
		}
	}
	// 关闭Socket
	void CleanUp()
	{
		if (_sockfd != INVALID_SOCKET) {
			// 关闭windows socket 2.x环境
#ifdef _WIN32 
			closesocket(_sockfd);
			CleanNet();
#else
			close(_sockfd);
#endif
			_sockfd = INVALID_SOCKET;
			printf("正在退出...\n\n");
		}
	}

	// 开始监控工作
	bool StartSelect()
	{
		if (IsRunning())
		{
			// 1.将服务端套接字加入到监控集合中
			fd_set reads;
			FD_ZERO(&reads);
			FD_SET(_sockfd, &reads);
			timeval timeout = { TIME_AWAKE, 0 };
			// 2.监控
			int ret = select(static_cast<int>(_sockfd) + 1, &reads, NULL, NULL, &timeout);
			if (ret < 0)
			{
				printf("Select任务出错,<%d>与服务器断开连接...\n", _sockfd);
				CleanUp();
				return false;
			}
			else if (ret == 0)
			{
				//here is to do 其余任务
				//printf(" select 监测到无就绪的描述符.\n");
			}
			// 3.判断服务端套接字是否就绪
			if (FD_ISSET(_sockfd, &reads))
			{
				FD_CLR(_sockfd, &reads);
				if (RecvData() == false)
				{
					printf("Proc导致通讯中断...\n");
					return false;
				}
			}
			return true;
		}
		return false;
	}
	// 套接字是否有效
	bool IsRunning()
	{
		return _sockfd != INVALID_SOCKET;	
	}

	// 发送请求包
	bool SendData(DataHeader* request)
	{
		if (IsRunning() && request) 
		{
			send(_sockfd, reinterpret_cast<const char*>(request), request->_dataLength, 0);
			return true;
		}
		return false;
	}
	// 响应包接收->分包和拆包
	bool RecvData() {
		char recv_buf[4096] = {};
		if (recv(_sockfd, recv_buf, sizeof(recv_buf), 0) <= 0)
		{
			CleanUp();
			return false;
		}
		// 用DataHeader类型的响应头接收数据包，然后进行处理
		DataHeader* response = reinterpret_cast<DataHeader*>(recv_buf);
		printf("<%d>响应包头信息：$cmd: %d, $lenght: %d\n", _sockfd, response->_cmd, response->_dataLength);
		MessageHandle(response);
		return true;
	}
	// 响应包处理函数
	virtual void MessageHandle(DataHeader* response)
	{
		// 基类的指针可以通过强制类型转换赋值给派生类的指针。但是必须是基类的指针是指向派生类对象时才是安全的。
		switch (response->_cmd)
		{
			case _LOGIN_RESULT:			// 登录返回
			{	printf("\t收到login指令.\n");	}	break;
			case _LOGOUT_RESULT:		// 下线返回
			{	printf("\t收到logout指令\n");	 }	break;
			case _NEWUSER_JOIN:			// 新人到来
			{
				/************************************************/
				NewJoin* broad = reinterpret_cast<NewJoin*>(response);
				printf("\t有新人到场.->$%d\n", broad->_sockfd);
			}	break;
			case _USER_QUIT:			// 退出
			{	printf("\t收到quit指令\n");		}	break;
			case _ERROR:
			{	printf("\terror cmd\n"); }	break;
			default:
				break;
		}
	}


private:
	SOCKET _sockfd;
};	// end of class
