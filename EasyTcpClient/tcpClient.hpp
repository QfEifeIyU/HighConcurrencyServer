#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#include "../Socket/Tools.hpp"
#include "../Socket/MessageType.hpp" 
#include <string.h>


class EasyTcpClient
{
public:
	EasyTcpClient()
		:_sockfd(INVALID_SOCKET)
	{
		//memset(_recv_buf, 0, sizeof(_recv_buf));
		memset(_msg_buf, 0, sizeof(_msg_buf));
		_endofmsgbf = 0;
		_isConnect = false;
	}
	virtual ~EasyTcpClient()
	{
		CleanUp();
		_sockfd = INVALID_SOCKET;
	}

	// 初始化Socket
	void InitSocket()
	{
		// 初始化windows socket 2.x环境
#ifdef _WIN32 
		StartUp();
#endif
		if (_sockfd != INVALID_SOCKET)
		{
			printf("<%d>关闭旧的服务器连接.\n", static_cast<int>(_sockfd));
			CleanUp();
		}
		_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_sockfd == INVALID_SOCKET)
		{
			printf("错误的SOCKET.\n");
		}
		//printf("SOCKET<%d>创建成功.\n", static_cast<int>(_sockfd));
	}

	// 连接服务器
	void Connect(const char* ip, const unsigned short port)
	{
		if (_sockfd == INVALID_SOCKET)
		{
			InitSocket();
		}
		sockaddr_in addr = {};
		SetAddr(&addr, ip, port);
#ifdef _WIN32
		int addrLen = sizeof(sockaddr_in);
#else
		socklen_t addrLen = sizeof(sockaddr_in);
#endif
		if (connect(_sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr_in)) == INVALID_SOCKET)
		{
			printf("连接服务器失败\n");
			CleanUp();
			return;
		}
		//printf("<$%d>成功连接到服务器<$%s : %d>...\n", static_cast<int>(_sockfd), ip, port);
		_isConnect = true;
	}

	// 关闭Socket
	void CleanUp()
	{
		if (_sockfd != INVALID_SOCKET) {
			Close(_sockfd);
			// 关闭windows socket 2.x环境
#ifdef _WIN32 
			CleanNet();
#endif
			_sockfd = INVALID_SOCKET;
			_isConnect = false;
			//printf("正在退出...\n\n");
		}
	}

	// 开始监控工作--select
	bool ActTask()
	{
		if (IsRunning())
		{
			// 1.将服务端套接字加入到监控集合中
			fd_set reads;
			FD_ZERO(&reads);
			FD_SET(_sockfd, &reads);
			// 2.监控
			timeval timeout = { 0, 0 };
			int ret = select(static_cast<int>(_sockfd) + 1, &reads, NULL, NULL, &timeout);
			if (ret < 0)
			{
				printf("Select任务出错,<%d>与服务器断开连接...\n", static_cast<int>(_sockfd));
				CleanUp();
				return false;
			}
			else if (ret == 0)
			{
				//here is to do 其余任务
				//printf(" select 监测到无就绪的描述符.\n");
			}

			// 3.判断服务端套接字是否就绪
			if (FD_ISSET(_sockfd, &reads))
			{
				FD_CLR(_sockfd, &reads);
				if (!RecvData(_sockfd))
				{
					//printf("Proc导致通讯中断...\n");
					CleanUp();
					return false;
				}
			}
			return true;
		}
		return false;
	}
	// 套接字是否有效
	bool IsRunning()
	{
		return _sockfd != INVALID_SOCKET && _isConnect;	
	}

	// 发送请求包
	bool SendData(DataHeader* request, int size)
	{
		if (IsRunning() && request) 
		{
			send(_sockfd, reinterpret_cast<const char*>(request), request->_dataLength, 0);
			return true;
		}
		return false;
	}
	

	// 响应包接收->分包和拆包
	bool RecvData(SOCKET ser_fd) 
	{
		char* recv_buf = _msg_buf + _endofmsgbf;
		int recv_len = recv(ser_fd, recv_buf, RECVBUFSIZE- _endofmsgbf, 0);
		if (recv_len <= 0)
		{
			return false;	// 断开连接或未接收到消息
		}

		//memcpy(_msg_buf + _endofmsgbf, _recv_buf, recv_len);
		_endofmsgbf += recv_len;

		while (_endofmsgbf >= sizeof(DataHeader))
		{
			DataHeader* response_head = reinterpret_cast<DataHeader*>(_msg_buf);
			int response_size = response_head->_dataLength;
			if (_endofmsgbf >= response_size)
			{
				int rest_data_size = _endofmsgbf - response_size;
				MessageHandle(response_head);
				memcpy(_msg_buf, _msg_buf + response_size, rest_data_size);
				_endofmsgbf = rest_data_size;
			}
			else 
			{
				break;
			}
		}
		return true;
	}
	// 响应包处理函数
	virtual void MessageHandle(DataHeader* response)
	{
		switch (response->_cmd)
		{
			case _LOGIN_RESULT:			// 登录返回
			{
				LoginResult* loginRet = reinterpret_cast<LoginResult*>(response);
				//printf("\t收到login指令.\n");	
			}	break;
			case _LOGOUT_RESULT:		// 下线返回
			{	
				LogoutResult* logoutRet = reinterpret_cast<LogoutResult*>(response);
				//printf("\t收到logout指令\n");	 
			}	break;
			case _NEWUSER_JOIN:			// 新人到来
			{
				NewJoin* broad = reinterpret_cast<NewJoin*>(response);
				printf("\t有新人到场.->$%d\n", static_cast<int>(broad->_fd));
			}	break;
			case _ERROR:
			{	
				printf("\terror cmd\n"); 
				printf("<$%d>响应包头信息：$cmd: error, $lenght: %d\n", static_cast<int>(_sockfd), response->_dataLength);
			}	break;
			default:
				break;
		}
	}


private:
	SOCKET _sockfd;
	/*使用缓冲解决tcp粘包问题*/
	char _msg_buf[RECVBUFSIZE] = {};
	int _endofmsgbf = 0;

	bool _isConnect;	// 连接状态
};	// end of class


#endif		/* end of tcpClient.hpp*/