#pragma once
#include "higeResolutionTimer.hpp"
#include "MessageType.hpp"

#include <vector>
#include <string.h>


#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
//#include <>

class Client
{
public:
	Client(SOCKET fd = INVALID_SOCKET)
	{
		_cli_fd = fd;
		memset(_msg_buf, 0, sizeof(_msg_buf));
		_endofmsgbf = 0;
	}
	~Client()
	{
		if (_cli_fd != INVALID_SOCKET)
		{
#ifdef _WIN32
			closesocket(_cli_fd);
#else
			close(_cli_fd);
#endif		
			_cli_fd = INVALID_SOCKET;
		}
	}

	SOCKET getFd()
	{
		return _cli_fd;
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
	SOCKET _cli_fd;
	char _msg_buf[RECVBUFSIZE * 10];
	int _endofmsgbf;
};


// 将接收请求和接收消息分离开
// MessageHandler 用来处理客户端的消息
class MessageHandler
{
public:
	MessageHandler()
	{
		_pthread = nullptr;
		_recvPackages = 1;
	}
	~MessageHandler()
	{
		this->_cli_Array.clear();
	}



/*引用双缓冲解决粘包问题*/
char _recv_buf[RECVBUFSIZE] = {};
	// 接收请求包
	bool RecvData(Client* cli) 
	{
		int recv_len = recv(cli->getFd(), _recv_buf, RECVBUFSIZE, 0);
		if (recv_len <= 0)
		{
			printf("<$%d>退出直播间.\n", static_cast<int>(cli->getFd()));
			return false;
		}
		memcpy(cli->MsgBuf() + cli->GetEndPos(), _recv_buf, recv_len);
		cli->SetEndPos(cli->GetEndPos() + recv_len);
		
		while (cli->GetEndPos() >= sizeof(DataHeader))
		{
			DataHeader* request_head = reinterpret_cast<DataHeader*>(cli->MsgBuf());
			//printf("<$%d>请求包头信息：$cmd: %d, $lenght: %d", static_cast<int>(cli_fd), request_head->_cmd, request_head->_datalength);
			int request_size = request_head->_dataLength;
			if (cli->GetEndPos() >= request_size)
			{
				int left_size = cli->GetEndPos() - request_size;	
				MsgHandle(cli->getFd(), request_head);
				memcpy(cli->MsgBuf(), cli->MsgBuf() + request_size, left_size);
				cli->SetEndPos(left_size);
			}
			else
			{
				break;
			}
		}
		return true;
	}

	// 请求包处理函数
	virtual void MsgHandle(SOCKET cli_fd, DataHeader* request)
	{
		++_recvPackages;
		switch (request->_cmd)
		{
			case _LOGIN:
			{		// 登录->返回登录信息
				Login* login = reinterpret_cast<Login*>(request);
				//printf("\t$%s, $%s已上线.\n", login->_userName, login->_passwd);
				// here is 判断登录信息，构造响应包
				//LoginResult result;
				//result._result = true;
				//SendData(cli_fd, &result);
			}	break;
			case _LOGOUT:
			{		// 注销->返回下线信息
				Logout* logout = reinterpret_cast<Logout*>(request);
				//printf("\t$%s已下线.\n", logout->_userName);
				// here is 确认退出下线，构造响应包
				//LogoutResult result;
				//result._result = true;
				//SendData(cli_fd, &result);
			}	break;
			default:
			{		// 发送了错误的命令->返回错误头
				printf("\t$error, 接收到错误的命令@%d\n", request->_cmd);
				//DataHeader result;
				//SendData(cli_fd, &result);
			}	break;
		}
		return;
	}


	// 微程序的消息处理启动函数
	void HandleProc()
	{
		// 利用线程启动select监控
		// mem_fun()将函数转换为类成员函数
		_pthread = new std::thread(std::mem_fun(&MessageHandler::thr_Handler_route), this);
		_pthread->detach();
	}

	// 监控工作--线程入口函数--recv Message
	bool thr_Handler_route()
	{
		/*       ---------------------------------------      */
		while (true)
		{
			// 1.将缓冲队列的全部新客户端，取出来放到正式队列中，清空缓冲队列
			if (_cli_ArrayBuffer.size() > 0)
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto newCli : _cli_ArrayBuffer)
				{
					_cli_Array.push_back(newCli);
				}
				_cli_ArrayBuffer.clear();
			}

			// 2.正式队列中没有需要处理的客户端就跳过
			if (_cli_Array.empty())
			{
				std::chrono::milliseconds time(1);
				std::this_thread::sleep_for(time);	// 休眠一毫秒
				//printf("休眠一毫秒...\n");
				continue;
			}

			fd_set reads, writes, excepts;

			FD_ZERO(&reads);
			FD_ZERO(&writes);
			FD_ZERO(&excepts);

			//FD_SET(_sockfd, &reads);		
			//FD_SET(_sockfd, &writes);	
			//FD_SET(_sockfd, &excepts);

			// 3.监控准备动作---将正式队列中的套接字加入到监控集合中
			SOCKET max_clifd = _cli_Array[0]->getFd();
			for (int i = static_cast<int>(_cli_Array.size()) - 1; i >= 0; --i)
			{
				FD_SET(_cli_Array[i]->getFd(), &reads);
				max_clifd = _cli_Array[i]->getFd() > max_clifd ? _cli_Array[i]->getFd() : max_clifd;
			}
			// 最后一个参数表示在timeout=3s内，如果没有可用描述符到来，立即返回
			// 为0表示阻塞在select，直到有可用描述符到来才返回
			timeval timeout = { TIME_AWAKE, 0 };

			// 4.开始进行监控
			int ret = select(static_cast<int>(max_clifd) + 1, &reads, &writes, &excepts, &timeout);
			if (ret < 0) {
				printf("select出错，重新启动监控.\n");
				return false;
			}
			else if (ret == 0)
			{
				//here is to do 其余任务
				//printf("  select 监控到当前无可用描述符就绪.\n");
				continue;
			}

			// 5.和正式队列中的就绪描述符进行通讯，如果通讯失败移出正式队列
			for (int i = 0; i < static_cast<int>(_cli_Array.size()); ++i)
			{
				if (FD_ISSET(_cli_Array[i]->getFd(), &reads) && !RecvData(_cli_Array[i]))
				{
					auto iterator = _cli_Array.begin() + i;
					if (iterator != _cli_Array.end())
					{
						delete (*iterator);
						_cli_Array.erase(iterator);
					}
				}
			}
		}
	}

	// 将新用户添加到缓冲队列中
	void AddClientToBuffer(Client* pcli)
	{
		// 利用RAII技术实现的自解锁
		std::lock_guard<std::mutex> lock(_mutex);
		_cli_ArrayBuffer.push_back(pcli);
	}


	// 查询微程序的所有客户数量
	size_t getClientCount()
	{
		return _cli_Array.size() + _cli_ArrayBuffer.size();
	}

private:
	SOCKET _sockfd;
	std::vector<Client*> _cli_Array;
	std::vector<Client*> _cli_ArrayBuffer;
	std::mutex _mutex;
	std::thread* _pthread;
public:
	std::atomic_int _recvPackages;
};	// end of MessageHandler




class TcpServer
{
public:
	TcpServer()
		:_ser_fd(INVALID_SOCKET)
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
		if (_ser_fd != INVALID_SOCKET)
		{
			printf("重新进行初始化.\n");
			CleanUp();
		}
		_ser_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_ser_fd == INVALID_SOCKET)
		{
			printf("错误的SOCKET.\n");
			return;
		}
		//printf("SOCKET创建成功.\n");
		
		
#ifdef _WIN32
		// 解决Time_Wait问题
		BOOL opt = TRUE;
		setsockopt(_ser_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(BOOL));
#else
		int opt = 1;
		setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)& opt, sizeof(opt));
#endif
	}
	// 绑定端口地址
	void Bind(const char* ip, const unsigned short port)
	{
		if (_ser_fd == INVALID_SOCKET)
		{
			InitSocket();
		}
		sockaddr_in addr = {};
		SetAddr(&addr, ip, port);
		if (bind(_ser_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr_in)) == SOCKET_ERROR)
		{
			printf("绑定端口失败!\n");
			return;
		}
		printf("<$ip：%s>绑定端口#%d...\n", ip, port);
	}
	// 监听
	void Listen(int maxSize)
	{
		if (listen(_ser_fd, maxSize) == SOCKET_ERROR)
		{
			printf("监听错误! \n");
			return;
		}
		//printf("服务器<$%d>已启动, 正在监听...\n", static_cast<int>(_sockfd));
	}

	// 创建消息处理微程序
	void CreateHandleMessageProcess()
	{
		for (int n = 0; n < CPU_CREATE_THREAD; ++n)
		{
			auto proc = new MessageHandler();
			_handlerProcs.push_back(proc);
			proc->HandleProc();
		}
	}

	// 接收客户端连接
	SOCKET Accept()
	{
		sockaddr_in cli_addr = {};
		int addr_len = sizeof(sockaddr_in);
#ifdef _WIN32
		SOCKET cli_fd = accept(_ser_fd, reinterpret_cast<sockaddr*>(&cli_addr), &addr_len);// accept获取客户端通信地址，返回一个通讯套接字与客户端通信
#else
		SOCKET cli_fd = accept(_sockfd, reinterpret_cast<sockaddr*>(&cli_addr), reinterpret_cast<socklen_t*>(&addr_len));// accept获取客户端通信地址，返回一个通讯套接字与客户端通信
#endif		
		if (cli_fd == INVALID_SOCKET)
		{
			printf("接收到无效的客户端SOCKET\n");
		}
		else {
			//NewJoin Coming;
			//Coming._fd = cli_fd;
			//Broad(&Coming);

			/*将新客户端发送给微服务程序中消息队列最少的一个*/
			AddClientToHandle_Buffer(new Client(cli_fd));
			//printf("新的客户端<%d><ip = %s>加入连接...共计已连接<%d>.\n", static_cast<int>(cli_sockfd), inet_ntoa(cli_addr.sin_addr), static_cast<int>(_cli_Array.size()));
		}
		return cli_fd;
	}

	// 添加新客户连接到最少连接的微程序的缓冲队列中
	void AddClientToHandle_Buffer(Client* pNewCli)
	{
		_cli_Array.push_back(pNewCli);
		
		// 获取当前总客户端最少的微程序--MessageHandler
		auto minCountProc = _handlerProcs[0];
		for (auto proc : _handlerProcs)
		{
			if (proc->getClientCount() < minCountProc->getClientCount())
			{
				minCountProc = proc;
			}
		}
		// 将新客户加入到最少连接的微程序管理中
		minCountProc->AddClientToBuffer(pNewCli);
	}

	// 关闭windows socket 2.x环境
	void CleanUp()
	{
		if (_ser_fd != INVALID_SOCKET)
		{
#ifdef _WIN32 
			for (int i = static_cast<int>(_cli_Array.size()) - 1; i >= 0; --i) {
				closesocket(_cli_Array[i]->getFd());
				delete _cli_Array[i];
			}
			closesocket(_ser_fd);
			CleanNet();
#else
			for (int i = static_cast<int>(_cli_Array.size()) - 1; i >= 0; --i) {
				close(_cli_Array[i]->getFd());
				delete _cli_Array[i];
			}
			close(_sockfd);
#endif
			printf("服务器<$%d>即将关闭...\n\n", static_cast<int>(_ser_fd));
			_ser_fd = INVALID_SOCKET;
			_cli_Array.clear();
		}
	}

	// 监控工作--仅处理请求
	bool StartSelect()
	{
		if (IsRunning())
		{
			CalcPackage();
			// 1.将服务端套接字添加到描述符结合监控中
			fd_set reads, writes, excepts;
			FD_ZERO(&reads);	 
			FD_ZERO(&writes);	
			FD_ZERO(&excepts);
			
			FD_SET(_ser_fd, &reads);		
			FD_SET(_ser_fd, &writes);	
			FD_SET(_ser_fd, &excepts);
			
			// 2.将通讯套接字加入到监控集合中
			SOCKET max_sockfd = _ser_fd;
		
			timeval timeout = { 0, 10 };	// 最后一个参数表示在timeout=3s内，如果没有可用描述符到来，立即返回
													// 为0表示阻塞在select，直到有可用描述符到来才返回
			// 3.开始进行监控
			int ret = select(static_cast<int>(max_sockfd) + 1, &reads, &writes, &excepts, &timeout);
			if (ret < 0) {
				printf("select出错，重新启动监控.\n");
				//CleanUp();
				return false;
			}
			else if (ret == 0)
			{
				//here is to do 其余任务
				//printf("  select 监控到当前无可用描述符就绪.\n");
				return false;
			}
			// 4.判断服务端socket是否就绪
			if (FD_ISSET(_ser_fd, &reads))
			{
				FD_CLR(_ser_fd, &reads);
				Accept();	
				return true;
			}
			return true;
		}
		return false;
	}
	// 判断是否有效
	bool IsRunning()
	{
		return _ser_fd != INVALID_SOCKET;
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
		for (auto e : _cli_Array)
			SendData(e->getFd(), request);
	}

	// 消息包计数函数
	void CalcPackage()
	{	
		auto Timer = _Timer.getSecond();
		if (Timer > 1.0 && fabs(Timer - 1.0) >= 1e-6)
		{
			int packages = 0;
			for (auto proc : _handlerProcs)
			{
				packages += proc->_recvPackages;
				//printf("主线程: packages=%d, 子线程：recvPackages=%d", packages, proc->_recvPackages);
				proc->_recvPackages = 0;
			}
			printf("<%lfs内> recvd cli{%d} sum of recv package：[%d]\n", Timer, (int)_cli_Array.size(), (int)(packages / Timer));
			_Timer.update();
		}		
	}
	


private:
	SOCKET _ser_fd;
	std::vector<Client*> _cli_Array;

	/*引入高精度计数器*/
	HigeResolutionTimer _Timer;
	/*引入微程序--线程消息处理数组*/
	std::vector<MessageHandler*> _handlerProcs;
	//MessageHandler _msgHandle;
};