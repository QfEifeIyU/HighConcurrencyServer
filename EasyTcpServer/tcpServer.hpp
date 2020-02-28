#pragma once
#ifdef _WIN32 
	#include "../HelloSocket/MessageType.hpp"
#else
	#include "MessageType.hpp"
#endif

#include <vector>
class TcpServer
{
public:
	TcpServer()
		:_sockfd(INVALID_SOCKET)
	{}
	virtual ~TcpServer()
	{
		CleanUp();
	}

	// 初始化windows socket 2.x环境
	void InitSocket()
	{
#ifdef _WIN32
		StartUp();
#endif
		if (_sockfd != INVALID_SOCKET)
		{
			printf("重新进行初始化.\n");
			CleanUp();
		}
		_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_sockfd == INVALID_SOCKET)
		{
			printf("错误的SOCKET.\n");
			return;
		}
		printf("SOCKET创建成功.\n");
		// 解决Time_Wait问题
#ifdef _WIN32
		BOOL opt = TRUE;
		setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(BOOL));
#else
		int opt = 1;
		setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)& opt, sizeof(opt));
#endif
	}
	// 绑定端口地址
	void Bind(const char* ip, const unsigned short port)
	{
		if (_sockfd == INVALID_SOCKET)
		{
			InitSocket();
		}
		sockaddr_in addr = {};
		SetAddr(&addr, ip, port);
		if (bind(_sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr_in)) == SOCKET_ERROR)
		{
			printf("绑定端口失败!\n");
			return;
		}
		printf("绑定端口$%d成功...\n", port);
	}
	// 监听
	void Listen(int maxSize)
	{
		if (listen(_sockfd, maxSize) == SOCKET_ERROR)
		{
			printf("监听错误! \n");
			return;
		}
		printf("服务器<$%d>已启动, 正在监听...\n", _sockfd);
	}
	// 接收客户端连接
	SOCKET Accept()
	{
		sockaddr_in cli_addr = {};
		SOCKET cli_sockfd = INVALID_SOCKET;
#ifdef _WIN32
		int addr_len = sizeof(cli_addr);
#else
		socklen_t addr_len = sizeof(sockaddr_in);
#endif
		// accept获取客户端通信地址，返回一个通讯套接字与客户端通信
		cli_sockfd = accept(_sockfd, reinterpret_cast<sockaddr*>(&cli_addr), &addr_len);
		if (cli_sockfd == INVALID_SOCKET)
		{
			printf("接收到无效的客户端SOCKET\n");
		}
		else {
			NewJoin Coming;
			Coming._sockfd = cli_sockfd;
			Broad(&Coming);
			cli_array.push_back(cli_sockfd);	// 将新的通讯套接字添加到动态数组中
			printf("新的客户端加入连接...\n\t 与其通讯的套接字 cli_sockfd = %d && ip = %s.\n",
				cli_sockfd, inet_ntoa(cli_addr.sin_addr));
		}
		return cli_sockfd;
	}
	// 关闭windows socket 2.x环境
	void CleanUp()
	{
		if (_sockfd != INVALID_SOCKET)
		{
#ifdef _WIN32 
			for (int i = static_cast<int>(cli_array.size()) - 1; i >= 0; --i) {
				closesocket(cli_array[i]);
			}
			closesocket(_sockfd);
			CleanNet();
#else
			for (int i = static_cast<int>(cli_array.size()) - 1; i >= 0; --i) {
				close(cli_array[i]);
			}
			close(_sockfd);
#endif
			printf("服务器<$%d>即将关闭...\n\n", _sockfd);
			_sockfd = INVALID_SOCKET;
		}

	}
	// 监控工作
	bool StartSelect()
	{
		if (IsRunning())
		{
			// 1.将服务端套接字添加到描述符结合监控中
			fd_set reads, writes, excepts;
			FD_ZERO(&reads);	 FD_ZERO(&writes);	FD_ZERO(&excepts);
			FD_SET(_sockfd, &reads);		FD_SET(_sockfd, &writes);	FD_SET(_sockfd, &excepts);
			SOCKET max_sockfd = _sockfd;
			// 2.将通讯套接字加入到监控集合中
			for (int i = static_cast<int>(cli_array.size()) - 1; i >= 0; --i)
			{
				FD_SET(cli_array[i], &reads);
				max_sockfd = cli_array[i] > max_sockfd ? cli_array[i] : max_sockfd;
			}
			timeval timeout = { TIME_AWAKE, 0 };	// 最后一个参数表示在timeout=3s内，如果没有可用描述符到来，立即返回
										// 为0表示阻塞在select，直到有可用描述符到来才返回
			// 3.开始进行监控
			int ret = select(max_sockfd + 1, &reads, &writes, &excepts, &timeout);
			if (ret < 0) {
				printf("select出错，任务结束.\n");
				CleanUp();
				return false;
			}
			else if (ret == 0)
			{
				//here is to do 其余任务
				//printf("  select 监控到当前无可用描述符就绪.\n");
				return false;
			}
			// 4.判断服务端socket是否就绪
			if (FD_ISSET(_sockfd, &reads))
			{
				FD_CLR(_sockfd, &reads);
				Accept();	
			}
			// 5.判断监控集合中是否有剩余描述符
			for (int i = 0; i < static_cast<int>(cli_array.size()); ++i)
			{
				if (FD_ISSET(cli_array[i], &reads))
				{
					if (RecvData(cli_array[i]) == false)
					{
						auto iterator = cli_array.begin() + i;
						if (iterator != cli_array.end())
						{
							cli_array.erase(iterator);
						}
					}
				}
			}
			return true;
		}
		return false;
	}
	// 判断是否有效
	bool IsRunning()
	{
		return _sockfd != INVALID_SOCKET;
	}

	// 接收请求包
	bool RecvData(SOCKET cli_sockfd) {
		char recv_buf[4096] = {};
		if (recv(cli_sockfd, recv_buf, sizeof(recv_buf), 0) <= 0)
		{
			printf("<%d>退出直播间.\n", cli_sockfd);
			return false;
		}
		DataHeader* request = reinterpret_cast<DataHeader*>(recv_buf);
		printf("<$%d>请求包头信息：$cmd: %d, $lenght: %d", cli_sockfd, request->_cmd, request->_dataLength);
		MessageHandle(cli_sockfd, request);
		return true;
	}
	// 请求包处理函数
	virtual void MessageHandle(SOCKET sockfd, DataHeader* request)
	{	
		Response response = {};
		switch (request->_cmd)
		{
			case _LOGIN:
			{		// 登录->返回登录信息
				Login* login = reinterpret_cast<Login*>(request);
				printf("\t$%s, $%s已上线.\n", login->_userName, login->_passwd);
				// here is 判断登录信息，构造响应包
				response._cmd = _LOGIN_RESULT;
				response._result = true;
			}	break;
			case _LOGOUT:
			{		// 下线->返回下线信息
				Logout* logout = reinterpret_cast<Logout*>(request);
				printf("\t$%s已下线.\n", logout->_userName);
				// here is 确认退出下线，构造响应包
				response._cmd = _LOGOUT_RESULT;
				response._result = true;
			}	break;
			case _USER_QUIT:
			{		// 有人离开
				printf("\t$quit, <%d>即将退出\n", sockfd);
				response._cmd = _USER_QUIT;
			}	break;
			case _ERROR: 
			{		// 发送了错误的命令
				printf("\t$error, 接收到错误的命令\n");
			}	break;
			default:
				break;
		}
		SendData(sockfd, &response);
	}
	// 发送响应包
	bool SendData(SOCKET sockfd, DataHeader* response)
	{
		if (IsRunning() && sockfd != INVALID_SOCKET)
		{
			send(sockfd, reinterpret_cast<const char*>(response), response->_dataLength, 0);
			return true;
		}
		return false;
	}
	// 新人到来广播
	void Broad(DataHeader* request)
	{
		for (auto e : cli_array)
			SendData(e, request);
	}


private:
	SOCKET _sockfd;
	std::vector<SOCKET> cli_array;
};