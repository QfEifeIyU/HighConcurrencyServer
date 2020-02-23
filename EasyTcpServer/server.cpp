#define WIN32_LEAN_AND_MEAN			// 避免windows库和WinSock2库的宏重定义
#define _WINSOCK_DEPRECATED_NO_WARNINGS		// itoa太旧

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

struct Login :public DataHeader{
	Login()	{
		this->_cmd = _LOGIN;
		this->_dataLength = sizeof(Login);
	}
	char _userName[32];
	char _passwd[32];
};

struct LoginResult :public DataHeader{
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

struct LogoutResult :public DataHeader{
	LogoutResult() {
		this->_cmd = _LOGOUT_RESULT;
		this->_dataLength = sizeof(LogoutResult);
		this->_result = false;
	}
	bool _result;
};

struct Response : public DataHeader{
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
				DataHeader request_head = {};	// 接收请求头信息
				int recv_len = recv(cli_sockfd, reinterpret_cast<char*>(&request_head), sizeof(DataHeader), 0);
				if (recv_len <= 0) {
					printf("接收数据失败，客户端退出，任务结束\n");
					break;
				}

				// 6.处理客户端请求，解析信息
				Response response = {};				// 响应

				switch (request_head._cmd) {
					case _LOGIN: {
						// 登录->返回登录信息
						Login login = {};	// 接收包体
						recv(cli_sockfd, reinterpret_cast<char*>(&login) + sizeof(DataHeader), 
							sizeof(Login) - sizeof(DataHeader), 0);
#ifndef Debug
						printf("包头信息：$cmd: %d, $lenght: %d\n", login._cmd, login._dataLength);
						printf("\t$%s, $%s已上线.\n", login._userName, login._passwd);
#endif
						// here is 判断登录信息，构造响应包
						response._cmd = _LOGIN_RESULT;
						response._result = true;
					}	break;
					case _LOGOUT: {
						// 下线->返回下线信息
						Logout logout = {};		// 接收包体
						recv(cli_sockfd, reinterpret_cast<char*>(&logout) + sizeof(DataHeader), 
							sizeof(Login) - sizeof(DataHeader), 0);
#ifndef Debug
						printf("包头信息：$cmd: %d, $lenght: %d\n", logout._cmd, logout._dataLength);
						printf("\t$%s已下线.\n", logout._userName);
#endif
						// here is 确认退出下线，构造响应包
						response._cmd = _LOGOUT_RESULT;
						response._result = true;
					}	break;
					default: {
						response._cmd = _ERROR;
						response._result = false;
					}
						break;
				}
				// 将结果返回给客户端
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