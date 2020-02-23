#define WIN32_LEAN_AND_MEAN			// 避免windows库和WinSock2库的宏重定义
#define _WINSOCK_DEPRECATED_NO_WARNINGS		// itoa太旧

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

//#pragma comment(lib, "ws2_32.lib")     // 链接器->输入->附加依赖项中添加库名称
#define PORT 4567
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
	// 2.bind 绑定用于接收客户端连接请求的网络端口
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);	// 将主机字节序转换为网络字节序
	addr.sin_addr.S_un.S_addr = INADDR_ANY;//inet_addr("127.0.0.1");
	if (bind(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
		printf("绑定端口失败!\n");
	}
	else {
		printf("绑定端口成功...\n");
	}
	// 3.listen 监听网络端口
	if (listen(sockfd, 5) == SOCKET_ERROR) {
		printf("监听错误! \n");
	}
	else {
		printf("服务器$%d已启动, 正在监听端口：%d...\n", sockfd, PORT);
		// 配置客户端网络端口地址
		sockaddr_in cli_addr = {};
		int addr_len = sizeof(cli_addr);
		while (true) {
			// 4.accept 等待客户端连接到来
			SOCKET cli_sockfd = INVALID_SOCKET;
			cli_sockfd = accept(sockfd, reinterpret_cast<sockaddr*>(&cli_addr), &addr_len);
			if (cli_sockfd == INVALID_SOCKET) {
				printf("接收到无效的客户端SOCKET\n");
			}
			else {
				printf("新的客户端加入连接...\n\t cli_sockfd = %d && ip = %s.\n",
					cli_sockfd, inet_ntoa(cli_addr.sin_addr));
			}
			while (true) {
				// 5.recv 接收客户端请求数据包
				DataHeader head = {};
				int recv_len = recv(cli_sockfd, reinterpret_cast<char*>(&head), sizeof(DataHeader), 0);
				if (recv_len <= 0) {
					printf("接收数据失败，客户端退出，任务结束\n");
					break;
				}
				else {
					printf("包头信息：$cmd: %d, $lenght: %d\n", head._cmd, head._dataLength);
				}

				// 6.处理客户端请求，解析信息
				DataHeader response_head = {};		// 响应头
				Response response = {};				// 响应体
				switch (head._cmd) {
					case _LOGIN: {
						// 登录->返回登录信息
						response_head._dataLength = sizeof(LoginResult);
						response_head._cmd = _LOGIN;
						Login login = {};
						recv(cli_sockfd, reinterpret_cast<char*>(&login), sizeof(Login), 0);
#ifndef Debug
						printf("\t$%s, $%s已上线.\n", login._userName, login._passwd);
#endif
						// here is 判断登录信息
						response._result = true;
					}	break;
					case _LOGOUT: {
						// 下线->返回下线信息
						response_head._dataLength = sizeof(LogoutResult);
						response_head._cmd = _LOGOUT;
						Logout logout = {};
						recv(cli_sockfd, reinterpret_cast<char*>(&logout), sizeof(Login), 0);
#ifndef Debug
						printf("\t$%s已下线.\n", logout._userName);
#endif
						// here is 确认退出下线
						response._result = true;
					}	break;
					default: 
						break;
				}
				// 将结果返回给客户端
				send(cli_sockfd, reinterpret_cast<const char*>(&response_head), sizeof(DataHeader), 0);
				send(cli_sockfd, reinterpret_cast<const char*>(&response), sizeof(Response), 0);
			}
		}
	}
	
	// 8.关闭套接字 close
	printf("服务器即将关闭...\n\n");
	closesocket(sockfd);

	// 清除windows socket环境
	WSACleanup();
	system("pause");
	return 0;
}