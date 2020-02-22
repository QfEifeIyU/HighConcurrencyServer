#define WIN32_LEAN_AND_MEAN			// 避免windows库和WinSock2库的宏重定义
#define _WINSOCK_DEPRECATED_NO_WARNINGS		// itoa太旧


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
#ifndef Debug
			printf("正在等待接收数据ing....\n");
#endif
			while (true) {
				// 5.recv 接收客户端请求
#define BUF_SIZE 128
				char recv_buf[BUF_SIZE] = {};

				int recv_len = recv(cli_sockfd, recv_buf, BUF_SIZE, 0);

				if (recv_len <= 0) {
					printf("接收数据失败，客户端退出，任务结束\n");
					break;
				}
				// 6.使用strcmp 处理客户端请求
				int response = 0;
				if (strcmp(recv_buf, "getName") == 0) {
					response = 1;
				}
				else if (strcmp(recv_buf, "getAge") == 0) {
					response = 2;
				}
				// 7.给客户端 send 响应
				const char* msg_buf[3] = { "What can I do for u？", "I'm Server.", "eighteen years old" };
				send(cli_sockfd, msg_buf[response], strlen(msg_buf[response]) + 1, 0);
#ifndef Debug
				printf("接收到命令：%s \n\t", recv_buf);
				printf("##response=%d ", response);
				printf(" ##%s\n", msg_buf[response]);
#endif
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