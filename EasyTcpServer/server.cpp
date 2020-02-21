#define WIN32_LEAN_AND_MEAN			// 避免windows库和WinSock2库的宏重定义
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>

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
		printf("正在监听端口：%d...\n", PORT);
	}
	// 配置网络端口地址
	sockaddr_in cli_addr = {};		
	int addr_len = sizeof(cli_addr);
	
	char msg_buf[] = "I'm Server.";
	int size_msg = strlen(msg_buf) + 1;
	
	while (true) {
		// 4.accept 等待客户端连接到来
		SOCKET cli_sockfd = INVALID_SOCKET;
		cli_sockfd = accept(sockfd, reinterpret_cast<sockaddr*>(&cli_addr), &addr_len);
		if (cli_sockfd == INVALID_SOCKET) {
			printf("接收到无效的客户端SOCKET\n");
		}
		else {
			printf("新的客户端加入连接...&& ip = %s.\n", inet_ntoa(cli_addr.sin_addr));
		}
	
		// 5.send 向客户端发送数据
		send(cli_sockfd, msg_buf, size_msg, 0);
	}
	// 6.关闭套接字 close
	closesocket(sockfd);

	// 清除windows socket环境
	WSACleanup();
	return 0;
}