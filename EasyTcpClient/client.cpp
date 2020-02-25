#define _CRT_SECURE_NO_WARNINGS			// scanf安全性

#include "../HelloSocket/Socket.h"
#include <ctime>		// 随机种子
#include "windows.h"		// 输出颜色
#include <iostream>
#include <thread>

bool g_Running = true;

using namespace std;

bool Proc(SOCKET sockfd) {
	// 1.recv 接收服务端响应包
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
			cout << "\t有新人到场.->$" << broad->_sockfd << endl;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY);
			return true;
		}	break;
		case _USER_QUIT: {	printf("\tquit->");		}	break;
		default: {	printf("\terror cmd->"); } break;
	}
	if (response._result) {
		printf("成功->%d\n",response._result);
	}
	else {
		printf("失败\n");
	}
#endif
	return true;
}


// 线程入口函数--构造请求
void th_route(SOCKET sockfd) {
	while (true) {
		string request = "";
		cin >> request;

		if (request == "login") {
			Login login;
			strcpy(login._userName, "lyb");
			strcpy(login._passwd, "123456");
			send(sockfd, reinterpret_cast<const char*>(&login), sizeof(Login), 0);
		}
		else if (request == "logout") {
			Logout logout;
			strcpy(logout._userName, "lyb");
			send(sockfd, reinterpret_cast<const char*>(&logout), sizeof(Logout), 0);
		}
		else if (request == "exit") {		// 退出
			DataHeader request_head;
			request_head._cmd = _USER_QUIT;
			request_head._dataLength = sizeof(DataHeader);
			send(sockfd, reinterpret_cast<const char*>(&request_head), sizeof(DataHeader), 0);
			g_Running = false;
		}
		else {								// 错误命令
			DataHeader request_head;
			request_head._cmd = _ERROR;
			request_head._dataLength = sizeof(DataHeader);
			send(sockfd, reinterpret_cast<const char*>(&request_head), sizeof(DataHeader), 0);
		}
	}
}

int main() {
	// 启动windows socket 2.x环境
	StartUp();

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
	SetAddr(&addr);
	if (connect(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr_in)) == INVALID_SOCKET) {
		printf("连接服务器失败\n");
	}
	else {
		printf("成功连接到服务器$%d...\n", sockfd);
		// 创建一个线程用于构造请求发送给服务器，并分离线程
		thread thr(th_route, sockfd);
		thr.detach();
		while (g_Running) {
			// 3-1.将服务端套接字加入到监控集合中
			fd_set reads;
			FD_ZERO(&reads);
			FD_SET(sockfd, &reads);
			timeval timeout = { TIME_AWAKE, 0 };
			// 3-2.开始监控
			int ret = select(0, &reads, NULL, NULL, &timeout);	
			if (ret < 0) {
				printf("select出错.\n ###与服务器断开连接...\n");
			}
			else if (ret == 0) {
				//here is to do 其余任务
				//printf(" select 监测到无就绪的描述符.\n");
			}
			// 3-2-1.判断服务端套接字是否就绪
			if (FD_ISSET(sockfd, &reads)) {
				FD_CLR(sockfd, &reads);
				if (Proc(sockfd) == false) {
					printf("Proc导致通讯中断...\n");
					break;
				}
			}
			
		}		
	}
	// 4.关闭套接字 close
	printf("正在退出...\n\n");
	closesocket(sockfd);

	// 清除windows socket环境
	CleanUp();
	system("pause");
	return 0;
}