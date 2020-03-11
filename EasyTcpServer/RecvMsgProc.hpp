#ifndef _RECVMSGPROC_HPP_
#define _RECVMSGPROC_HPP_

#include <vector>
#include <map>
#include <mutex>
#include <functional>		// mem_fn可以转换指针和引用
#include "NetEvent.hpp"

// RecvMsgProc 用来接收客户端的消息
class RecvMsgProc
{
public:
	RecvMsgProc(NetEvent* net)
	{
		/* 让net代理EasyTcpServer，然后在需要的地方触发网络事件 */
		_pEvent = net;		// 注册网络事件对象
		FD_ZERO(&_reads);
		_maxSockfd = -1;
	}

	~RecvMsgProc()
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

	/*       -------接收消息线程入口函数---------      */
	fd_set _reads;
	SOCKET _maxSockfd;
	void Thr_Route()
	{
		while (g_TaskRuning)
		{
			// 1.将缓冲队列的客户端，取出来放到正式队列并监控，更新maxfd，清空缓冲队列
			if (_cli_ArrayBuffer.size() > 0)
			{
				std::lock_guard<std::mutex> lock(_mutex);
				SOCKET clifd = INVALID_SOCKET;
				for (auto newClient : _cli_ArrayBuffer)
				{
					clifd = newClient->GetFd();
					_cli_Array[clifd] = newClient;
					FD_SET(clifd, &_reads);
					if (_maxSockfd != INVALID_SOCKET)
					{
						_maxSockfd = clifd > _maxSockfd ? clifd : _maxSockfd;
					}
				}
				_cli_ArrayBuffer.clear();
			}

			// 2.正式队列中没有客户就跳过
			if (_cli_Array.empty())
			{
				std::chrono::milliseconds millSec(1);
				std::this_thread::sleep_for(millSec);	// 休眠一毫秒
				continue;
			}

			// 3.进行监控工作：select会改变监控集合，因此不能用全局的fd_set
			fd_set bak;
			memcpy(&bak, &_reads, sizeof(fd_set));
			timeval timeout = { 0, 0 };
			// select 调用后 bak 内为就绪的描述符
			int ret = select(static_cast<int>(_maxSockfd) + 1, &bak, nullptr, nullptr, &timeout);
			if (ret < 0) {
				printf("RecvMsgProc 出错，重新启动监控.\n");
				this->~RecvMsgProc();
				return;
			}
			else if (ret == 0)
			{
				//here is to do 其余任务
				continue;
			}

			// 5.使用 bak 内就绪的描述符
#ifdef _WIN32		/* windows平台中就绪的描述符放在 bak.fd_array 中 */
			for (int n = 0; n < static_cast<int>(bak.fd_count); ++n)
			{
				SOCKET readyfd = bak.fd_array[n];
				auto it = _cli_Array.find(readyfd);

				if (!RecvData(it->second))
				{
					// 通讯失败，表示有客户离开，在全局的set中取出
					FD_CLR(readyfd, &_reads);
					Close(readyfd);
					delete it->second;
					_cli_Array.erase(it);
				}
			}
#else			   /* 其他平台 */
			std::vector<ClientSock*> dropCli_Arr;
			for (auto it : _cli_Array)
			{
				ClientSock* pcli = it.second;
				if (FD_ISSET(pcli->GetFd(), &bak))
				{
					if (!RecvData(pcli))
					{
						FD_CLR(pcli->GetFd(), &_reads);
						dropCli_Arr.push_back(pcli);
						Close(pcli->GetFd());
					}
				}
			}
			for (auto pcli : dropCli_Arr)
			{
				_cli_Array.erase(pcli->GetFd());
				delete pcli;
			}
#endif
		}
	}

	// 接收消息线程启动函数
	void CreateThread()
	{
		// 创建线程启动select监控
		auto route = std::mem_fn(&RecvMsgProc::Thr_Route);	// mem_fun()将成员函数转换为函数对象
		_pthread = std::thread(route, this);
		_pthread.detach();
	}

	/*       -------引用缓冲解决粘包问题---------      */
	// 接收请求包，接收到的数据直接放到该客户的消息缓冲区内
	bool RecvData(ClientSock* pcli)
	{
		char* msgbf = pcli->GetMsgBuf();	// 获取缓冲区
		int pos = pcli->GetEndPos();		// 获取缓冲区的结尾位置

		char* recv_buf = msgbf + pos;		// 从缓冲区的结尾位置接收
		int recv_len = recv(pcli->GetFd(), recv_buf, RECVBUFSIZE - pos, 0);
		_pEvent->NetRecv(pcli);		// 触发接收消息网络事件
		if (recv_len <= 0)
		{
			if (_pEvent)
				_pEvent->NetCliQuit(pcli);		// 触发客户离开网络事件
			return false;
		}
		pcli->SetEndPos(pos + recv_len);

		while (pcli->GetEndPos() >= sizeof(DataHeader))
		{
			DataHeader* header = reinterpret_cast<DataHeader*>(pcli->GetMsgBuf());
			int body = header->_dataLength;
			if (pcli->GetEndPos() >= body)
			{
				int new_pos = pcli->GetEndPos() - body;
				_pEvent->NetMsgHandle(pcli, header);		// 触发消息处理事件
				memcpy(pcli->GetMsgBuf(), pcli->GetMsgBuf() + body, new_pos);
				pcli->SetEndPos(new_pos);
			}
			else
			{
				break;
			}
		}
		return true;
	}

	// 将新用户添加到缓冲队列中
	void AddClientToBuffer(ClientSock* pcli)
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
	std::map<SOCKET, ClientSock*> _cli_Array;		// 正式队列
	std::vector<ClientSock*> _cli_ArrayBuffer;		// 缓冲队列<临界区>

	std::mutex _mutex;
	std::thread _pthread;
	NetEvent* _pEvent;		// 网络事件对象--JOIN、QUIT、MsgHandler
};



#endif // !_RECVMSGPROC_HPP_
