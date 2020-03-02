#pragma once

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN			// 避免windows库和WinSock2库的宏重定义
	#define _WINSOCK_DEPRECATED_NO_WARNINGS			// itoa转换函数版本过低
	#include <windows.h>
	#include <WinSock2.h>
	//#pragma comment(lib, "ws2_32.lib")     // 链接器->输入->附加依赖项中添加库名称
#else
	#include <unistd.h>     // std 
	#include <arpa/inet.h>  // socket
	#include <string.h>     // strcpy
	typedef int SOCKET;     // 适应sockfd
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif 

#define TIME_AWAKE 1	// select轮询时间
#define RECVBUFSIZE 10240
//const unsigned short PORT = 0x4567;		// 服务端绑定端口号
#include <stdio.h>  

enum CMD 
{
	_LOGIN,		
	_LOGOUT,
	_LOGIN_RESULT,		
	_LOGOUT_RESULT,
	_NEWUSER_JOIN,
	_USER_QUIT,
	_ERROR = 1000
};

// 传输结构化数据格式
struct DataHeader 
{
	DataHeader() 
	{
		_dataLength = sizeof(DataHeader);
		_cmd = _ERROR;
	}
	
	short _dataLength;
	CMD _cmd;		// 命令
};

struct Login :public DataHeader 
{
	Login() 
	{
		this->_cmd = _LOGIN;
		this->_dataLength = sizeof(Login);
	}
	char _userName[32];
	char _passwd[32];
	char _data[928];
};

struct LoginResult :public DataHeader 
{
	LoginResult() 
	{
		this->_cmd = _LOGIN_RESULT;
		this->_dataLength = sizeof(LoginResult);
		this->_result = false;
	}
	bool _result;
	char _data[990];
};

struct Logout :public DataHeader 
{
	Logout() 
	{
		this->_cmd = _LOGOUT;
		this->_dataLength = sizeof(Logout);
	}
	char _userName[32];
};

struct LogoutResult :public DataHeader 
{
	LogoutResult() 
	{
		this->_cmd = _LOGOUT_RESULT;
		this->_dataLength = sizeof(LogoutResult);
		this->_result = false;
	}
	bool _result;
};

struct Response : public DataHeader 
{
	Response() 
	{
		this->_cmd = _ERROR;
		this->_dataLength = sizeof(Response);
		this->_result = false;
		memset(_data, 0, 1024);
	}
	bool _result;
	char _data[990];
};
int a = sizeof(LoginResult);
struct NewJoin :public DataHeader 
{
	NewJoin() 
	{
		this->_cmd = _NEWUSER_JOIN;
		this->_dataLength = sizeof(NewJoin);
		this->_fd = INVALID_SOCKET;
	}
	SOCKET _fd;
};


#ifdef _WIN32
// 搭建windows socket 2.x环境
void StartUp() 
{
	WORD ver = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(ver, &data);
}
void CleanNet() 
{
	WSACleanup();
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