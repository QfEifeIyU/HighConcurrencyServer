#include "../HelloSocket/Socket.h"
#include <vector>

using namespace std;

vector<SOCKET> g_TalkWithCli_fds;

bool Proc(SOCKET cli_sockfd) {
	// 1.recv 接收客户端请求数据包
	char recv_buf[4096] = {};
	int recv_len = recv(cli_sockfd, recv_buf, sizeof(recv_buf), 0);
	if (recv_len <= 0) {
		return false;
	}
	// 2.处理客户端请求，解析信息
	DataHeader* request_head = reinterpret_cast<DataHeader*>(recv_buf);
	printf("<$%d>->请求包头信息：$cmd: %d, $lenght: %d", cli_sockfd, request_head->_cmd, request_head->_dataLength);
	Response response = {};				// 响应
	switch (request_head->_cmd) {
		case _LOGIN: {		// 登录->返回登录信息
			Login* login = reinterpret_cast<Login*>(recv_buf);	// 接收包体
			printf("\t$%s, $%s已上线.\n", login->_userName, login->_passwd);
			// here is 判断登录信息，构造响应包
			response._cmd = _LOGIN_RESULT;
			response._result = true;
		}	break;
		case _LOGOUT: {		// 下线->返回下线信息
			Logout* logout = reinterpret_cast<Logout*>(recv_buf);	// 接收包体
			printf("\t$%s已下线.\n", logout->_userName);
			// here is 确认退出下线，构造响应包
			response._cmd = _LOGOUT_RESULT;
			response._result = true;
		}	break;
		case _USER_QUIT: {
			printf("\t$quit, <%d>即将退出\n", cli_sockfd);
			response._cmd = _USER_QUIT;
		}	break;
		case _ERROR: {		// 发送了错误的命令
			printf("\t$error, 接收到错误的命令\n");
		}	break;
		default: 
			break;
	}
	// 将结果返回给客户端
	send(cli_sockfd, reinterpret_cast<const char*>(&response), sizeof(Response), 0);
	return true;
}

int main() {
	// 启动windows socket 2.x环境
	StartUp();

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
	SetAddr(&addr);
	if (bind(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr_in)) == SOCKET_ERROR) {
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
		printf("服务器<$%d>已启动, 正在监听端口：%d...\n", sockfd, PORT);
		while (true) {
			// 4-1.将服务端套接字添加到描述符结合监控中
			fd_set reads, writes, excepts;
			FD_ZERO(&reads);	 FD_ZERO(&writes);	FD_ZERO(&excepts);
			FD_SET(sockfd, &reads);		FD_SET(sockfd, &writes);	FD_SET(sockfd, &excepts);
		
			// 4-2.将通讯套接字加入到监控集合中
			for (int i = static_cast<int>(g_TalkWithCli_fds.size()) - 1; i >= 0; --i) {
				FD_SET(g_TalkWithCli_fds[i], &reads);
			}

			timeval timeout = { TIME_AWAKE, 0 };	// 最后一个参数表示在timeout=3s内，如果没有可用描述符到来，立即返回
										// 为0表示阻塞在select，直到有可用描述符到来才返回
			// 4-3.开始进行监控
			int ret = select(0, &reads, &writes, &excepts, &timeout);
			if (ret == 0) {
				//printf("  select 监控到当前无可用描述符就绪.\n");
				//here is to do 其余任务
				continue;
			}
			else if (ret < 0){
				printf("select出错，任务结束.\n");
				break;
			}
			// 4-4.判断服务端socket是否就绪->客户端有连接到来
			if (FD_ISSET(sockfd, &reads)) {		
				FD_CLR(sockfd, &reads);		// 清理监控集合
				// 4-4-1.accept 等待客户端连接到来，返回一个通讯套接字
				sockaddr_in cli_addr = {};
				int addr_len = sizeof(cli_addr);
				SOCKET cli_sockfd = INVALID_SOCKET;
				// accept获取客户端通信地址，返回一个通讯套接字与客户端通信
				cli_sockfd = accept(sockfd, reinterpret_cast<sockaddr*>(&cli_addr), &addr_len);
				if (cli_sockfd == INVALID_SOCKET) {
					printf("接收到无效的客户端SOCKET\n");
				}
				else {
					// 4-4-2.将新的通讯套接字添加到动态数组中
					// 有新客户到来，将其广播给所有已连接的客户端.
					for (int i = 0; i < g_TalkWithCli_fds.size(); ++i) {
						NewJoin msg_borad;
						msg_borad._sockfd = cli_sockfd;
						send(g_TalkWithCli_fds[i], reinterpret_cast<const char*>(&msg_borad), sizeof(NewJoin), 0);
					}
					g_TalkWithCli_fds.push_back(cli_sockfd);
					printf("新的客户端加入连接...\n\t 与其通讯的套接字 cli_sockfd = %d && ip = %s.\n",
						cli_sockfd, inet_ntoa(cli_addr.sin_addr));
				} 
			}
			// 4-5.判断监控集合中是否有剩余描述符->即就绪的通讯描述符
			for (int i = 0; i < static_cast<int>(reads.fd_count); ++i) {
				// 4-5-1.如果有即就绪的通讯描述符，对齐进行Porc调用
				if (Proc(reads.fd_array[i]) == false) {
					printf("Proc导致通信中断...\n\t客户端<$%d>已退出.\n", reads.fd_array[i]);
					// 4-5-2.Proc调用失败，将通讯描述符从数组中移除
					auto iterator = find(g_TalkWithCli_fds.begin(), g_TalkWithCli_fds.end(), reads.fd_array[i]);
					if (iterator != g_TalkWithCli_fds.end()) {
						g_TalkWithCli_fds.erase(iterator);
					}
				}
			}
		}
	}
	for (int i = g_TalkWithCli_fds.size() - 1; i >= 0; --i) {
		closesocket(g_TalkWithCli_fds[i]);
	}

	// 5.关闭套接字 close
	printf("服务器即将关闭...\n\n");
	closesocket(sockfd);

	// 清除windows socket环境
	CleanUp();

	return 0;
}