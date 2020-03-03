#pragma once
#ifdef _WIN32 
	#include "../HelloSocket/MessageType.hpp"
#else
	#include "MessageType.hpp"
#endif

#include <vector>
#include <string.h>

class Client
{
public:
	Client(SOCKET fd = INVALID_SOCKET)
	{
		_sockfd = fd;
		memset(_msg_buf, 0, sizeof(_msg_buf));
		_endofmsgbf = 0;
	}
	~Client()
	{
		if (_sockfd != INVALID_SOCKET) 
		{
#ifdef _WIN32
			closesocket(_sockfd);
#else
			close(_sockfd);
#endif		
			_sockfd = INVALID_SOCKET;
		}
	}

	SOCKET GetFd()
	{
		return _sockfd;
	}

	char* MsgBuf()
	{
		return _msg_buf;
	}

	int GetEndPos()
	{
		return _endofmsgbf;
	}

	void SetEndPos(int pos)
	{
		_endofmsgbf = pos;
	}
private:
	SOCKET _sockfd;
	char _msg_buf[RECVBUFSIZE * 10];
	int _endofmsgbf;
};


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
		printf("服务器<$%d>已启动, 正在监听...\n", static_cast<int>(_sockfd));
	}
	// 接收客户端连接
	SOCKET Accept()
	{
		sockaddr_in cli_addr = {};
		int addr_len = sizeof(sockaddr_in);
#ifdef _WIN32
		SOCKET cli_sockfd = accept(_sockfd, reinterpret_cast<sockaddr*>(&cli_addr), &addr_len);// accept获取客户端通信地址，返回一个通讯套接字与客户端通信
#else
		SOCKET cli_sockfd = accept(_sockfd, reinterpret_cast<sockaddr*>(&cli_addr), reinterpret_cast<socklen_t*>(&addr_len));// accept获取客户端通信地址，返回一个通讯套接字与客户端通信
#endif		
		if (cli_sockfd == INVALID_SOCKET)
		{
			printf("接收到无效的客户端SOCKET\n");
		}
		else {
			NewJoin Coming;
			Coming._fd = cli_sockfd;
			Broad(&Coming);
			_cli_array.push_back(new Client(cli_sockfd));	// 将新的通讯套接字添加到动态数组中
			printf("新的客户端加入连接...\n\t 与其通讯的套接字 cli_sockfd = %d && ip = %s.\n",
				static_cast<int>(cli_sockfd), inet_ntoa(cli_addr.sin_addr));
		}
		return cli_sockfd;
	}
	// 关闭windows socket 2.x环境
	void CleanUp()
	{
		if (_sockfd != INVALID_SOCKET)
		{
#ifdef _WIN32 
			for (int i = static_cast<int>(_cli_array.size()) - 1; i >= 0; --i) {
				closesocket(_cli_array[i]->GetFd());
				delete _cli_array[i];
			}
			closesocket(_sockfd);
			CleanNet();
#else
			for (int i = static_cast<int>(_cli_array.size()) - 1; i >= 0; --i) {
				close(_cli_array[i]->GetFd());
				delete _cli_array[i];
			}
			close(_sockfd);
#endif
			printf("服务器<$%d>即将关闭...\n\n", static_cast<int>(_sockfd));
			_sockfd = INVALID_SOCKET;
			_cli_array.clear();
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
			// 2.将通讯套接字加入到监控集合中
			SOCKET max_sockfd = _sockfd;
			for (int i = static_cast<int>(_cli_array.size()) - 1; i >= 0; --i)
			{
				FD_SET(_cli_array[i]->GetFd(), &reads);
				max_sockfd = _cli_array[i]->GetFd() > max_sockfd ? _cli_array[i]->GetFd() : max_sockfd;
			}
			timeval timeout = { TIME_AWAKE, 0 };	// 最后一个参数表示在timeout=3s内，如果没有可用描述符到来，立即返回
										// 为0表示阻塞在select，直到有可用描述符到来才返回
			// 3.开始进行监控
			int ret = select(static_cast<int>(max_sockfd) + 1, &reads, &writes, &excepts, &timeout);
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
			for (int i = 0; i < static_cast<int>(_cli_array.size()); ++i)
			{
				if (FD_ISSET(_cli_array[i]->GetFd(), &reads))
				{
					if (RecvData(_cli_array[i]) == false)
					{
						auto iterator = _cli_array.begin() + i;
						if (iterator != _cli_array.end())
						{
							delete (*iterator);
							_cli_array.erase(iterator);
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

	// 发送响应包
	bool SendData(SOCKET cli_fd, DataHeader* response)
	{
		if (IsRunning() && cli_fd != INVALID_SOCKET)
		{
			send(cli_fd, reinterpret_cast<const char*>(response), response->_dataLength, 0);
			return true;
		}
		return false;
	}
	// 新人到来广播
	void Broad(DataHeader* request)
	{
		for (auto e : _cli_array)
			SendData(e->GetFd(), request);
	}

	/*
		引用双缓冲解决粘包问题
	*/
	char _recv_buf[RECVBUFSIZE] = {};
	// 接收请求包
	bool RecvData(Client* cli) 
	{
		int recv_len = recv(cli->GetFd(), _recv_buf, RECVBUFSIZE, 0);
		if (recv_len <= 0)
		{
			printf("<%d>退出直播间.\n", static_cast<int>(cli->GetFd()));
			return false;
		}
		// 将接收缓冲区的内容拷贝到消息缓冲区中
		memcpy(cli->MsgBuf() + cli->GetEndPos(), _recv_buf, recv_len);
		// 更新消息缓冲区的结尾位置
		cli->SetEndPos(cli->GetEndPos() + recv_len);
		
		while (cli->GetEndPos() >= sizeof(DataHeader))
		{
			// 如果消息缓冲区中有一个头部的大小，将消息缓冲区中响应包头部信息提取出来
			DataHeader* request_head = reinterpret_cast<DataHeader*>(cli->MsgBuf());
			//printf("<$%d>请求包头信息：$cmd: %d, $lenght: %d", static_cast<int>(cli_fd), request_head->_cmd, request_head->_datalength);
			// 从提取出来的请求头信息获取当前请求包的长度
			int request_size = request_head->_dataLength;
			if (cli->GetEndPos() >= request_size)
			{
				// 如果消息缓冲区中的数据大于请求包的长度，将消息缓冲区的响应包提取出来
				// 消息缓冲区中保存剩余数据的长度
				int left_size = cli->GetEndPos() - request_size;
				// 对提取出来的数据进行处理	
				MessageHandle(cli->GetFd(), request_head);
				// 更新消息缓冲区
				memcpy(cli->MsgBuf(), cli->MsgBuf() + request_size, left_size);
				// 更新消息缓冲区中数据结尾位置
				cli->SetEndPos(left_size);
			}
			else
			{
				printf("消息缓冲区数据不足一个完整包\n");
				break;
			}
		}
		return true;
	}
	// 请求包处理函数
	virtual void MessageHandle(SOCKET cli_fd, DataHeader* request)
	{	
		switch (request->_cmd)
		{
			case _LOGIN:
			{		// 登录->返回登录信息
				Login* login = reinterpret_cast<Login*>(request);
				//printf("\t$%s, $%s已上线.\n", login->_userName, login->_passwd);
				// here is 判断登录信息，构造响应包
				LoginResult result;
				result._result = true;
				SendData(cli_fd, &result);
			}	break;
			case _LOGOUT:
			{		// 注销->返回下线信息
				Logout* logout = reinterpret_cast<Logout*>(request);
				//printf("\t$%s已下线.\n", logout->_userName);
				// here is 确认退出下线，构造响应包
				LogoutResult result;
				result._result = true;
				SendData(cli_fd, &result);
			}	break;
			default:
			{		// 发送了错误的命令->返回错误头
				printf("\t$error, 接收到错误的命令\n");
				DataHeader result;
				SendData(cli_fd, &result);
			}	break;
		}
	}
	


private:
	SOCKET _sockfd;
	std::vector<Client*> _cli_array;
};