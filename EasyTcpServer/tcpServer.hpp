#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

/* 将接收请求和接收消息分离开-> TcpServer + RecvMsgProc */
#include "higeResolutionTimer.hpp"
#include "RecvMsgProc.hpp"
#include <atomic>


// TcpServer 用于接收客户端连接请求
class EasyTcpServer : public NetEvent
{
public:
	EasyTcpServer()
		:_ser_fd(INVALID_SOCKET), 
		_clientAmount(0), _recvAmount(0), _packageAmount(0)
	{}

	virtual ~EasyTcpServer()
	{
		if (_ser_fd != INVALID_SOCKET)
		{
			Close(_ser_fd);
#ifdef _WIN32 
			CleanNet();
#endif
			printf("服务器<$%d>即将关闭...\n\n", static_cast<int>(_ser_fd));
			_ser_fd = INVALID_SOCKET;
			this->_recvMsgProc_Array.clear();
		}
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
		}
		_ser_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_ser_fd == INVALID_SOCKET)
		{
			printf("错误的SOCKET.\n");
			return;
		}
		//printf("SOCKET创建成功.\n");

#ifdef _WIN32	// 解决Time_Wait问题
		BOOL opt = TRUE;
		setsockopt(_ser_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(BOOL));
#else
		int opt = 1;
		setsockopt(_ser_fd, SOL_SOCKET, SO_REUSEADDR, (void*)& opt, sizeof(opt));
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

	// 创建消息处理线程
	void CreateRecvMsgProc(int thrProcAmount)
	{
		for (int n = 0; n < thrProcAmount; ++n)
		{
			/* 使用代理在线程内通知主线程有客户端退出*/
			// 1.设计一个纯虚类NetEventInterface -- CliQuit、CliJoin、RecvMsgProc
			// 2.在主线程 TcpServer类继承纯虚类的抽象接口并定义
			// 3.在消息处理线程 MsgHandlerProc 内添加一个pEvent成员,并对其进行设置初始化
			// 4.在适当的地方触发事件，返回给 TcpServer
			auto proc = new RecvMsgProc(this);		// 启动一个接收消息的线程
			_recvMsgProc_Array.push_back(proc);
			proc->CreateThread();		// 启动消息处理线程
		}
	}

	// 接收客户端连接
	SOCKET Accept()
	{
		sockaddr_in cli_addr = {};
		int addr_len = sizeof(sockaddr_in);
#ifdef _WIN32
		SOCKET cli_fd = accept(_ser_fd, reinterpret_cast<sockaddr*>(&cli_addr), &addr_len);
#else
		SOCKET cli_fd = accept(_ser_fd, reinterpret_cast<sockaddr*>(&cli_addr), reinterpret_cast<socklen_t*>(&addr_len));
#endif		
		if (cli_fd == INVALID_SOCKET)
		{
			printf("接收到无效的客户端SOCKET\n");
		}
		else {
			/*将新客户发送给接收消息线程中客户最少的一个*/
			AddClientToRecvMsgProc(new ClientSock(cli_fd));
			// 客户端ip->inet_ntoa(cli_addr.sin_addr)
		}
		return cli_fd;
	}

	// 将新客户端分配给总客户端最少的一个消息处理线程 
	void AddClientToRecvMsgProc(ClientSock* pNewCli)
	{
		// 获取当前总客户端最少的微程序--MsgHandlerProc
		auto minCountProc = _recvMsgProc_Array[0];
		for (auto proc : _recvMsgProc_Array)
		{
			if (proc->getClientAmount() < minCountProc->getClientAmount())
			{
				minCountProc = proc;
			}
		}
		// 将新客户加入到最少连接的微程序管理中
		minCountProc->AddClientToBuffer(pNewCli);
		EasyTcpServer::NetCliJoin(pNewCli);			// 触发客户加入网络事件
	}


	/*       -------主线程 处理客户连接请求函数---------      */
	bool AcceptConProc()
	{
		if (IsRunning())
		{
			CalcPackage();			
			// 1.将服务端套接字添加到描述符结合监控中
			fd_set reads;
			FD_ZERO(&reads);	 
			FD_SET(_ser_fd, &reads);		
			
			// 2.计算select的最大描述符
			SOCKET max_sockfd = _ser_fd;
			
			// 3.开始进行监控
			timeval timeout = { 0, 10 };
			int ret = select(static_cast<int>(max_sockfd) + 1, &reads, nullptr, nullptr, &timeout);
			if (ret < 0) {
				printf("AcceptConProc出错，重新启动监控.\n");
				this->~EasyTcpServer();
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
			}
			return true;
		}
		return false;
	}

	// 判断是否有效
	bool IsRunning()
	{
		return _ser_fd != INVALID_SOCKET && g_TaskRuning;
	}
	
	// 消息包计数函数
	void CalcPackage()
	{	
		auto Sec = _Timer.getSecond();
		if (Sec >= 1.0)// && fabs(Sec - 1.0) >= 1e-6)
		{
			printf("<$%lfs内> thread{#%d} recvd cli{#%d} recvAmount[%.lf] packageAmount[%.lf]\n", 
				Sec, (int)_recvMsgProc_Array.size(), (int)_clientAmount, (int)_recvAmount / Sec, (int)_packageAmount / Sec);
			_packageAmount = 0;
			_recvAmount = 0;
			_Timer.update();
		}		
	}
	


	// 客户端加入
	virtual void NetCliJoin(ClientSock* pcli)
	{
		_clientAmount++;
		//printf("cli<%d> JOIN.\n", pcli->GetFd());
	}

	// 客户端退出
	virtual void NetCliQuit(ClientSock* pcli)
	{
		_clientAmount--;
		//printf("cli<%d> QUIT.\n", pcli->GetFd());
	}

	// 消息处理事件
	virtual void NetMsgHandle(ClientSock* pcli, DataHeader* header)
	{
		++_packageAmount;
	}

	// 接收消息事件
	virtual void NetRecv(ClientSock* pcli)
	{
		++_recvAmount;
	}

private:
	SOCKET _ser_fd;

	/*引入高精度计数器*/
	HigeResolutionTimer _Timer;
	/*引入*/
	std::vector<RecvMsgProc*> _recvMsgProc_Array;

protected:
	std::atomic_int _recvAmount;		// 收到消息计数
	std::atomic_int _clientAmount;		// 客户端计数
	std::atomic_int _packageAmount;			// 包计数
};

#endif