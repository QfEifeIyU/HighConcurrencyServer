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
			printf("<%d>关闭旧的服务器连接.\n", static_cast<int>(_sockfd));
			CleanUp();
		}
		_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_sockfd == INVALID_SOCKET)
		{
			printf("错误的SOCKET.\n");
		}
		//printf("SOCKET<%d>创建成功.\n", static_cast<int>(_sockfd));
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
		//printf("<$%d>成功连接到服务器<$%s : %d>...\n", static_cast<int>(_sockfd), ip, port);
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

	int _count = 0;
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
			//printf("select ret = %d count = %d\n", ret, _count++);
			if (ret < 0)
			{
				printf("Select任务出错,<%d>与服务器断开连接...\n", static_cast<int>(_sockfd));
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
					CleanUp();
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
	
	/*
		使用双缓冲解决tcp粘包问题
	*/

	char _recv_buf[RECVBUFSIZE] = {};
	char _msg_buf[RECVBUFSIZE * 10] = {};
	int _endofmsgbf = 0;

	// 响应包接收->分包和拆包
	bool RecvData() 
	{
		int recv_len = recv(_sockfd, _recv_buf, RECVBUFSIZE, 0);
		if (recv_len <= 0)
		{
			return false;
		}
		// 将接收缓冲区的内容拷贝到消息缓冲区中
		memcpy(_msg_buf + _endofmsgbf, _recv_buf, recv_len);
		// 更新消息缓冲区的结尾位置
		_endofmsgbf += recv_len;

		while (_endofmsgbf >= sizeof(DataHeader))
		{
			// 如果消息缓冲区中有一个头部的大小，将消息缓冲区中响应包头部信息提取出来
			DataHeader* response_head = reinterpret_cast<DataHeader*>(_msg_buf);
			//printf("<$%d>响应包头信息：$cmd: %d, $lenght: %d\n", static_cast<int>(_sockfd), response_head->_cmd, response_head->_dataLength);
			// 从提取出来的响应头信息获取当前响应包的长度
			int response_size = response_head->_dataLength;
			if (_endofmsgbf >= response_size)
			{
				// 如果消息缓冲区中的数据大于请求包的长度，将消息缓冲区的响应包提取出来
				// 消息缓冲区中保存剩余数据的长度
				int left_size = _endofmsgbf - response_size;
				// 对提取出来的数据进行处理
				MessageHandle(response_head);
				// 更新消息缓冲区
				memcpy(_msg_buf, _msg_buf + response_size, left_size);
				// 更新消息缓冲区中数据结尾位置
				_endofmsgbf = left_size;
			}
			else 
			{
				printf("消息缓冲区数据不足一个完整包\n");
				break;
			}
		}
		return true;
	}
	// 响应包处理函数
	virtual void MessageHandle(DataHeader* response)
	{
		// 基类的指针可以通过强制类型转换赋值给派生类的指针。但是必须是基类的指针是指向派生类对象时才是安全的。
		switch (response->_cmd)
		{
			case _LOGIN_RESULT:			// 登录返回
			{
				LoginResult* loginRet = reinterpret_cast<LoginResult*>(response);
				//printf("\t收到login指令.\n");	
			}	break;
			case _LOGOUT_RESULT:		// 下线返回
			{	
				LogoutResult* logoutRet = reinterpret_cast<LogoutResult*>(response);
				//printf("\t收到logout指令\n");	 
			}	break;
			case _NEWUSER_JOIN:			// 新人到来
			{
				/************************************************/
				NewJoin* broad = reinterpret_cast<NewJoin*>(response);
				printf("\t有新人到场.->$%d\n", static_cast<int>(broad->_fd));
			}	break;
			case _ERROR:
			{	
				printf("\terror cmd\n"); 
				printf("<$%d>响应包头信息：$cmd: error, $lenght: %d\n", static_cast<int>(_sockfd), response->_dataLength);
			}	break;
			default:
				break;
		}
	}


private:
	SOCKET _sockfd;
};	// end of class
