#define _CRT_SECURE_NO_WARNINGS			// scanf安全性
#define WIN32_LEAN_AND_MEAN			// 避免windows库和WinSock2库的宏重定义
#define _WINSOCK_DEPRECATED_NO_WARNINGS

enum CMD {
	_LOGIN,
	_LOGOUT,
	_ERROR
};

// 传输结构化数据格式
struct DataHeader {
	short _dataLength;
	CMD _cmd;		// 命令
};

struct Login {
	char _userName[32];
	char _passwd[32];
};

struct LoginResult {
	bool _result;
};

struct Logout {
	char _userName[32];
};

struct LogoutResult {
	bool _result;
};

struct Response {
	bool _result;
};

#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT 4567


//#pragma comment(lib, "ws2_32.lib")     // 常规->输入->附加依赖项中添加库名称
using namespace std;

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
		printf("成功连接到服务器\n");
		while (true) {
			// 3. 输入请求
			char request[128] = {};
			printf("请输入请求: ");
			fflush(0);
			scanf("%s", request);

			// 4. send 向服务器发送请求包
			if (strcmp(request, "login") == 0) {
				DataHeader head = { sizeof(Login), _LOGIN };
				Login login = {"lyb", "123456"};
				// 发送包头+包体
				send(sockfd, reinterpret_cast<const char*>(&head), sizeof(DataHeader), 0);
				send(sockfd, reinterpret_cast<const char*>(&login), sizeof(Login), 0);
			}
			else if (strcmp(request, "logout") == 0) {
				DataHeader head = { sizeof(Logout), _LOGOUT };
				Logout logout = { "lyb" };
				// 发送包头+包体
				send(sockfd, reinterpret_cast<const char*>(&head), sizeof(DataHeader), 0);
				send(sockfd, reinterpret_cast<const char*>(&logout), sizeof(Logout), 0);
			}
			else {
				if (strcmp(request, "exit") == 0) 
					printf("exit, 任务结束.\n");
				else
					printf("error, 不支持的命令.\n");
				break;
			}
			// 5.recv 接收服务器的响应数据包
			DataHeader response_head = {};
			Response response = {};
			recv(sockfd, reinterpret_cast<char*>(&response_head), sizeof(DataHeader), 0);
			recv(sockfd, reinterpret_cast<char*>(&response), sizeof(Response), 0);
#ifndef Debug
			printf("包头信息：$cmd: %d, $lenght: %d\n", response_head._cmd, response_head._dataLength);
			printf("\t ##log result: %d->", response._result);
			if (response._result == true) {
				printf("成功\n");
			}
			else {
				printf("失败\n");
			}
#endif
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