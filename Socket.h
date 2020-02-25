#pragma once

#define WIN32_LEAN_AND_MEAN			// 避免windows库和WinSock2库的宏重定义
#define _WINSOCK_DEPRECATED_NO_WARNINGS			// itoa转换函数版本过低
#define PORT 4567			// 服务端绑定端口号
#define TIME_AWAKE 1		// select轮询时间
//#pragma comment(lib, "ws2_32.lib")     // 链接器->输入->附加依赖项中添加库名称

#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

enum CMD {
	_LOGIN = 1,
	_LOGOUT,
	_LOGIN_RESULT = 10,
	_LOGOUT_RESULT,
	_NEWUSER_JOIN = 100,
	_USER_QUIT = 200,
	_ERROR = 1000
};

// 传输结构化数据格式
struct DataHeader {
	DataHeader() {
		_dataLength = sizeof(DataHeader);
		_cmd = _ERROR;
	}
	short _dataLength;
	CMD _cmd;		// 命令
};

struct Login :public DataHeader {
	Login() {
		this->_cmd = _LOGIN;
		this->_dataLength = sizeof(Login);
	}
	char _userName[32];
	char _passwd[32];
};

struct LoginResult :public DataHeader {
	LoginResult() {
		this->_cmd = _LOGIN_RESULT;
		this->_dataLength = sizeof(LoginResult);
		this->_result = false;
	}
	bool _result;
};

struct Logout :public DataHeader {
	Logout() {
		this->_cmd = _LOGOUT;
		this->_dataLength = sizeof(Logout);
	}
	char _userName[32];
};

struct LogoutResult :public DataHeader {
	LogoutResult() {
		this->_cmd = _LOGOUT_RESULT;
		this->_dataLength = sizeof(LogoutResult);
		this->_result = false;
	}
	bool _result;
};

struct Response : public DataHeader {
	Response() {
		this->_cmd = _ERROR;
		this->_dataLength = sizeof(Response);
		this->_result = false;
	}
	bool _result;
};

struct NewJoin :public DataHeader {
	NewJoin() {
		this->_cmd = _NEWUSER_JOIN;
		this->_dataLength = sizeof(NewJoin);
		this->_sockfd = INVALID_SOCKET;
	}
	SOCKET _sockfd;
};


// 搭建windows socket 2.x环境
void StartUp() {
	WORD ver = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(ver, &data);
}

// 设置地址信息
void SetAddr(sockaddr_in* addr) {
	addr->sin_family = AF_INET;
	addr->sin_port = htons(PORT);
	addr->sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
}

void CleanUp() {
	WSACleanup();
}