#define _CRT_SECURE_NO_WARNINGS			// scanf安全性
#define WIN32_LEAN_AND_MEAN			// 避免windows库和WinSock2库的宏重定义
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define PORT 4567
#define TIME_AWAKE 1


#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>		// 随机种子
#include "windows.h"
#include <iostream>
//#pragma comment(lib, "ws2_32.lib")     // 常规->输入->附加依赖项中添加库名称

enum CMD {
	_LOGIN = 1,
	_LOGOUT,
	_LOGIN_RESULT = 10,
	_LOGOUT_RESULT,
	_NEWUSER_JOIN = 100,
	_ERROR = 1000
};

// 传输结构化数据格式
struct DataHeader {
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

using namespace std;

bool Proc(SOCKET sockfd) {
	// 5.recv 接收服务端响应包
	Response response = {};
	int recv_len = recv(sockfd, reinterpret_cast<char*>(&response), sizeof(Response), 0);
	if (recv_len <= 0) {
		return false;
	}
#ifndef Debug
	printf("响应包头信息：$cmd: %d, $lenght: %d\n", response._cmd, response._dataLength);
	switch (response._cmd) {
		case _LOGIN_RESULT: {	printf("\tlogin->");	}	break;
		case _LOGOUT_RESULT: {	printf("\tlogout->");	 }	break;
		case _NEWUSER_JOIN: {
			NewJoin* broad = reinterpret_cast<NewJoin*>(&response);
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED);
			cout << "\t有新人到场.->$" << broad->_sockfd;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY);
		}	break;
		default: {	printf("\terror->"); }	break;
	}
	if (response._result) {
		printf("成功\n");
	}
	else {
		printf("失败\n");
	}
#endif
	return true;
}

void ProcRequest(SOCKET sockfd) {
	srand((unsigned int)time(0));
	int num = rand() % 3;
	if (num == 0) {
		Login login;
		strcpy(login._userName, "lyb");
		strcpy(login._passwd, "123456");
		send(sockfd, reinterpret_cast<const char*>(&login), sizeof(Login), 0);
	}
	else if (num == 1) {
		Logout logout;
		strcpy(logout._userName, "lyb");
		send(sockfd, reinterpret_cast<const char*>(&logout), sizeof(Logout), 0);
	}
	else {
		DataHeader request_head;
		request_head._cmd = _ERROR;
		request_head._dataLength = sizeof(DataHeader);
		send(sockfd, reinterpret_cast<const char*>(&request_head), sizeof(DataHeader), 0);
		return;
	}
#ifdef Debug
	{	const char* arr[] = { "login", "logout", "request" }; printf("随机数%d\t--->发送-%s\n", num, arr[num]); }
#endif
}

int main() {
	// 启动windows socket 2.x环境
	

	WORD ver = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(ver, &data);
	// 添加网络程序
	// 1.建立服务端 socket
	SOCKET sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == INVALID_SOCKET) {
		printf("错误的SOCKET.\n");
	}
	else {
		printf("SOCKET创建成功.\n");
	}
	// 2.connect 连接服务器
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (connect(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr_in)) == INVALID_SOCKET) {
		printf("连接服务器失败\n");
	}
	else {
		printf("成功连接到服务器$%d.\n", sockfd);
		while (true) {
			// 对客户端加入select模型
			// 3-1.将服务端套接字加入到监控集合中
			fd_set reads;
			FD_ZERO(&reads);
			FD_SET(sockfd, &reads);
			timeval timeout = { TIME_AWAKE, 0 };
			int ret = select(0, &reads, NULL, NULL, &timeout);	
			if (ret < 0) {
				printf("select出错.\n ###正在断开服务器...\n");
			}
#ifdef Debug
			else if (ret == 0) {
				printf(" select 监测到无就绪的描述符.\n");
			}
#endif
			else {
				// 判断服务端套接字是否就绪
				if (FD_ISSET(sockfd, &reads)) {
					FD_CLR(sockfd, &reads);
					if (Proc(sockfd) == false) {
						printf("Proc导致通讯中断...\n");
						break;
					}
				}
				// 自动构造命令向服务端发送请求
				ProcRequest(sockfd);
			}
		}		
	}
	// 7.关闭套接字 close
	printf("正在退出...\n\n");
	closesocket(sockfd);

	// 清除windows socket环境
	WSACleanup();
	system("pause");
	return 0;
}