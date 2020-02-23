#define _CRT_SECURE_NO_WARNINGS			// scanf安全性
#define WIN32_LEAN_AND_MEAN			// 避免windows库和WinSock2库的宏重定义
#define _WINSOCK_DEPRECATED_NO_WARNINGS

enum CMD {
	_LOGIN,
	_LOGOUT,
	_LOGIN_RESULT,
	_LOGOUT_RESULT,
	_ERROR
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
				Login login;	
				strcpy(login._userName, "lyb");
				strcpy(login._passwd, "123456");
				send(sockfd, reinterpret_cast<const char*>(&login), sizeof(Login), 0);
			}
			else if (strcmp(request, "logout") == 0) {
				Logout logout;
				strcpy(logout._userName, "lyb");
				send(sockfd, reinterpret_cast<const char*>(&logout), sizeof(Logout), 0);
			}
			else if (strcmp(request, "exit") == 0) {
				printf("exit, 任务结束.\n");	
				break;
			}
			else {
				printf("error, 错误的命令.\n");
				continue;
			}
			// 5.recv 接收服务器的响应数据包
			Response response = {};
			recv(sockfd, reinterpret_cast<char*>(&response), sizeof(Response), 0);
#ifndef Debug
			printf("包头信息：$cmd: %d, $lenght: %d\n", response._cmd, response._dataLength);
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