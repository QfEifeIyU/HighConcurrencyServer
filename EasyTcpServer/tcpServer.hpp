#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#include "../Socket/MessageType.hpp"
#include "../Socket/Tools.hpp"
#include "higeResolutionTimer.hpp"

#include <vector>
#include <mutex>
#include <atomic>
#include <functional>		// mem_fn可以转换指针和引用
#include <map>


// server 管理客户端的数据类型
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
			Close(_cli_fd);
			_cli_fd = INVALID_SOCKET;
		}
	}

	// 获取当前客户端的socket fd
	SOCKET GetFd()
	{
		return _cli_fd;
	}
	// 获取当前客户端的缓冲区
	char* GetMsgBuf()
	{
		return _msg_buf;
	}
	// 获取缓冲区的结束位
	int GetEndPos()
	{
		return _endofmsgbf;
	}
	// 设置缓冲区的结束位
	void SetEndPos(int pos)
	{
		_endofmsgbf = pos;
	}

	// 发送响应包
	bool SendData(DataHeader* response)
	{
		if (_cli_fd != INVALID_SOCKET)
		{
			send(_cli_fd, reinterpret_cast<const char*>(response), response->_dataLength, 0);
			return true;
		}
		return false;
	}
private:
	SOCKET _cli_fd;
	char _msg_buf[RECVBUFSIZE * 5];
	int _endofmsgbf;
};		//end of class


// 网络事件接口
class NetEvent_Interface
{
public:
	// 客户端加入
	virtual void NetClientJoin(Client* pcli) = 0;

	// 客户端推出
	virtual void NetClientQuit(Client* pcli) = 0;
	
	// 消息处理事件
	virtual void NetMessageHandle(Client* pcli, DataHeader* header) = 0;

	//virtual void NetRecv(Client* pcli) = 0;
};


/* 将接收请求和接收消息分离开->TcpServer + MsgHandlerProc */

// MsgHandlerProc 用来处理客户端的消息
class MsgHandlerProc
{
public:
	MsgHandlerProc()
	{
		_pEvent = nullptr;
	}

	~MsgHandlerProc()
	{
		RealeaseCliArr();
	}

	void RealeaseCliArr() 
	{
		delete _pEvent;
		if (_cli_Array.size() > 0)
		{
			for (auto iter : _cli_Array)
			{
				Close(iter.second->GetFd());
				delete iter.second;
			}
			_cli_Array.clear();
		}
	}

	// 设置纯虚类成员变量，让net代理TcpServer，然后在有客户端离开时用pInterface调用NetClientQuit
	void setEventObj(NetEvent_Interface* net)
	{
		_pEvent = net;		// net代表的是TcpServer
	}

	/*       -------处理网络消息线程入口函数---------      */
	fd_set _reads;
	SOCKET _maxSockfd;

	void Task_thr_route()
	{
		FD_ZERO(&_reads);

		while (g_TaskRuning)
		{
			// 1.将缓冲队列的全部新客户端，取出来放到正式队列和set集合中，更新maxfd，清空缓冲队列
			if (_cli_ArrayBuffer.size() > 0)
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto newCli : _cli_ArrayBuffer)
				{
					_cli_Array[newCli->GetFd()] = newCli;
					//_cli_Array.push_back(newCli);
					FD_SET(newCli->GetFd(), &_reads);	
					if (_maxSockfd != INVALID_SOCKET)
					{
						_maxSockfd = newCli->GetFd() > _maxSockfd ? newCli->GetFd() : _maxSockfd;
					}
				}
				_cli_ArrayBuffer.clear();
			}

			// 2.正式队列中没有需要处理的客户端就跳过
			if (_cli_Array.empty())
			{
				std::chrono::milliseconds millSec(1);
				std::this_thread::sleep_for(millSec);	// 休眠一毫秒
				//printf("休眠一毫秒...\n");
				continue;
			}

			// 3.进行监控工作：select会改变监控集合，因此不能用全局的fd_set
			fd_set bak;
			memcpy(&bak, &_reads, sizeof(fd_set));
		
			// 4.开始进行监控，select 调用后 bak 内为就绪的描述符
			timeval timeout = { 0, 0 };
			int ret = select(static_cast<int>(_maxSockfd) + 1, &bak, nullptr, nullptr, &timeout);
			if (ret < 0) {
				RealeaseCliArr();
				printf("MessageHandlerProc 出错，重新启动监控.\n");
				return;
			}
			else if (ret == 0)
			{
				//here is to do 其余任务
				//printf("  select 监控到当前无可用描述符就绪.\n");
				continue;
			}

			// 5.使用 bak 内的就绪描述符通讯
#ifdef _WIN32		/* windows中就绪的描述符放在 bak.fd_array 中 */
			for (int n = 0; n < static_cast<int>(bak.fd_count); ++n)
			{
				SOCKET fd = bak.fd_array[n];		// cli表示就绪的套接字
				auto it = _cli_Array.find(fd);
				
				if (!RecvData(it->second))
				{
					// 通讯失败，表示有客户端离开，在全局的set中取出
					FD_CLR(fd, &_reads);
					Close(fd);
					delete it->second;
					_cli_Array.erase(it);
				}			
			}
#else			/* 其他平台 */
			std::vector<Client*> dropCliArr;
			for (auto it : _cli_Array)
			{
				Client* pcli = it.second;
				if (FD_ISSET(pcli->GetFd(), &bak))
				{
					if (!RecvData(pcli))
					{
						FD_CLR(pcli->GetFd(), &_reads);
						dropCliArr.push_back(pcli);
						Close(pcli->GetFd());
					}
				}
			}
			for (auto pcli : dropCliArr)
			{
				_cli_Array.erase(pcli->GetFd());
				delete pcli;
			}
#endif
		}
	}

	// 消息处理启动函数
	void HandleProc()
	{
		// 利用线程启动select监控
		_pthread = std::thread(std::mem_fn(&MsgHandlerProc::Task_thr_route), this);		// mem_fun()将成员函数转换为函数对象
		_pthread.detach();
	}

	/*引用双缓冲解决粘包问题*/
	char _recv_buf[RECVBUFSIZE] = {};
	// 接收请求包
	bool RecvData(Client* pcli)
	{
		//_pEvent->NetRecv(pcli);
		int recv_len = recv(pcli->GetFd(), _recv_buf, RECVBUFSIZE, 0);
		if (recv_len <= 0)
		{
			// 此时，_pEvent代表的是TcpServer
			if (_pEvent)
				_pEvent->NetClientQuit(pcli);
			//printf("<$%d>退出直播间.\n", static_cast<int>(cli->GetFd()));
			return false;
		}
		memcpy(pcli->GetMsgBuf() + pcli->GetEndPos(), _recv_buf, recv_len);
		pcli->SetEndPos(pcli->GetEndPos() + recv_len);

		while (pcli->GetEndPos() >= sizeof(DataHeader))
		{
			DataHeader* request_head = reinterpret_cast<DataHeader*>(pcli->GetMsgBuf());
			int request_size = request_head->_dataLength;
			if (pcli->GetEndPos() >= request_size)
			{
				int left_size = pcli->GetEndPos() - request_size;
				Handler(pcli, request_head);
				memcpy(pcli->GetMsgBuf(), pcli->GetMsgBuf() + request_size, left_size);
				pcli->SetEndPos(left_size);
			}
			else
			{
				break;
			}
		}
		return true;
	}

	// 请求包处理函数
	void Handler(Client* pcli, DataHeader* request)
	{
		_pEvent->NetMessageHandle(pcli, request);
	}

	// 将新用户添加到缓冲队列中
	void AddClientToBuffer(Client* pcli)
	{
		std::lock_guard<std::mutex> lock(_mutex);	// 利用RAII技术实现的自解锁
		_cli_ArrayBuffer.push_back(pcli);
	}

	// 查询消息处理线程 MsgHandlerProc 的所有客户数量
	size_t getClientAmount()
	{
		return _cli_Array.size() + _cli_ArrayBuffer.size();
	}


private:
	std::map<SOCKET, Client*> _cli_Array;		// 正式队列
	std::vector<Client*> _cli_ArrayBuffer;		// 缓冲队列

	std::mutex _mutex;		
	std::thread _pthread;
	NetEvent_Interface* _pEvent;		// 网络事件对象--JOIN、QUIT、MsgHandler
};	// #######################end of MsgHandlerProc



// TcpServer 用于接收客户端连接请求
class TcpServer : public NetEvent_Interface
{
public:
	TcpServer()
		:_ser_fd(INVALID_SOCKET), _clientAmount(0), _recvAmount(0), _packageAmount(0)
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
		

#ifdef _WIN32	// 解决Time_Wait问题
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

	// 创建消息处理线程
	void CreateHandleMessageProcess(int thrProcAmount)
	{
		for (int n = 0; n < thrProcAmount; ++n)
		{
			/* 使用代理在线程内通知主线程有客户端退出*/
			// 1.设计一个纯虚类NetEventInterface -- ClientQuit、ClientJoin、NetMessageHandler
			// 2.在主线程 TcpServer类继承纯虚类的抽象接口并定义
			// 3.在消息处理线程 MsgHandlerProc 内添加一个pEvent成员,并对其进行设置初始化
			// 4.在适当的地方触发事件，返回给 TcpServer
			auto proc = new MsgHandlerProc();
			_handlerProc_Array.push_back(proc);
			// 注册网络事件对象
			proc->setEventObj(this);	
			// 启动消息处理线程
			proc->HandleProc();
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
		SOCKET cli_fd = accept(_sockfd, reinterpret_cast<sockaddr*>(&cli_addr), reinterpret_cast<socklen_t*>(&addr_len));
#endif		
		if (cli_fd == INVALID_SOCKET)
		{
			printf("接收到无效的客户端SOCKET\n");
		}
		else {
			/*将新客户端发送给微服务程序中消息队列最少的一个*/
			AddClientToHandlerProc(new Client(cli_fd));
			// 客户端ip->inet_ntoa(cli_addr.sin_addr)
		}
		return cli_fd;
	}

	// 将新客户端分配给客户端最少的一个消息处理线程 
	void AddClientToHandlerProc(Client* pNewCli)
	{
		// 获取当前总客户端最少的微程序--MsgHandlerProc
		auto minCountProc = _handlerProc_Array[0];
		for (auto proc : _handlerProc_Array)
		{
			if (proc->getClientAmount() < minCountProc->getClientAmount())
			{
				minCountProc = proc;
			}
		}
		// 将新客户加入到最少连接的微程序管理中
		minCountProc->AddClientToBuffer(pNewCli);
		NetClientJoin(pNewCli);
	}

	// 关闭windows socket 2.x环境
	void CleanUp()
	{
		if (_ser_fd != INVALID_SOCKET)
		{
			Close(_ser_fd);
#ifdef _WIN32 
			CleanNet();
#endif
			printf("服务器<$%d>即将关闭...\n\n", static_cast<int>(_ser_fd));
			_ser_fd = INVALID_SOCKET;
		}
	}


	/*       -------处理客户端请求线程函数---------      */
	bool ActTask()
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
	
	// 数据广播
	//void Broad(DataHeader* request)
	//{
	//	for (auto e : _cli_Array)
	//		SendData(e->GetFd(), request);
	//}

	// 消息包计数函数
	void CalcPackage()
	{	
		auto Sec = _Timer.getSecond();
		if (Sec >= 1.0)// && fabs(Sec - 1.0) >= 1e-6)
		{
			printf("<$%lfs内> thread{#%d} recvd cli{#%d}\t packageAmount[#%.1lf]\n", 
				Sec, (int)_handlerProc_Array.size(), (int)_clientAmount, (int)_packageAmount / Sec);
			_packageAmount = 0;
			_Timer.update();
		}		
	}
	


	// 客户端加入
	virtual void NetClientJoin(Client* pcli)
	{
		//_clientAmount++;
	}

	// 客户端退出
	virtual void NetClientQuit(Client* pcli)
	{
		//_clientAmount--;
	}

	// 消息处理事件
	virtual void NetMessageHandle(Client* pcli, DataHeader* header)
	{
		//++_packageAmount;
	}


private:
	SOCKET _ser_fd;

	/*引入高精度计数器*/
	HigeResolutionTimer _Timer;
	/*引入微程序--线程消息处理数组*/
	std::vector<MsgHandlerProc*> _handlerProc_Array;

protected:
	// 收到消息计数
	std::atomic_int _recvAmount;
	// 客户端计数
	std::atomic_int _clientAmount;
	// 包计数
	std::atomic_int _packageAmount;
};

#endif