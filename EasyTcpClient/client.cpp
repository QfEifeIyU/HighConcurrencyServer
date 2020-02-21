#define WIN32_LEAN_AND_MEAN			// 避免windows库和WinSock2库的宏重定义
#define _WINSOCK_DEPRECATED_NO_WARNINGS

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
	}
	// 3.send 向客户端发送数据
	char msg_recv[256] = {};
	int size_recv = recv(sockfd, msg_recv, sizeof(msg_recv), 0);
	if (size_recv > 0) {
		printf("接收到服务端的数据：%s\n", msg_recv);
	}
	// 4.关闭套接字 close
	closesocket(sockfd);

	// 清除windows socket环境
	WSACleanup();
	system("pause");
	return 0;
}