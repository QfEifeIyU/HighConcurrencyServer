#ifndef _RECVMSGPROC_HPP_
#define _RECVMSGPROC_HPP_

#include <vector>
#include <map>
#include <mutex>
#include <functional>		// mem_fn����ת��ָ�������
#include "NetEvent.hpp"

// RecvMsgProc �������տͻ��˵���Ϣ
class RecvMsgProc
{
public:
	RecvMsgProc(NetEvent* net)
	{
		/* ��net����EasyTcpServer��Ȼ������Ҫ�ĵط����������¼� */
		_pEvent = net;		// ע�������¼�����
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

	/*       -------������Ϣ�߳���ں���---------      */
	fd_set _reads;
	SOCKET _maxSockfd;
	void Thr_Route()
	{
		while (g_TaskRuning)
		{
			// 1.��������еĿͻ��ˣ�ȡ�����ŵ���ʽ���в���أ�����maxfd����ջ������
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

			// 2.��ʽ������û�пͻ�������
			if (_cli_Array.empty())
			{
				std::chrono::milliseconds millSec(1);
				std::this_thread::sleep_for(millSec);	// ����һ����
				continue;
			}

			// 3.���м�ع�����select��ı��ؼ��ϣ���˲�����ȫ�ֵ�fd_set
			fd_set bak;
			memcpy(&bak, &_reads, sizeof(fd_set));
			timeval timeout = { 0, 0 };
			// select ���ú� bak ��Ϊ������������
			int ret = select(static_cast<int>(_maxSockfd) + 1, &bak, nullptr, nullptr, &timeout);
			if (ret < 0) {
				printf("RecvMsgProc ���������������.\n");
				this->~RecvMsgProc();
				return;
			}
			else if (ret == 0)
			{
				//here is to do ��������
				continue;
			}

			// 5.ʹ�� bak �ھ�����������
#ifdef _WIN32		/* windowsƽ̨�о��������������� bak.fd_array �� */
			for (int n = 0; n < static_cast<int>(bak.fd_count); ++n)
			{
				SOCKET readyfd = bak.fd_array[n];
				auto it = _cli_Array.find(readyfd);

				if (!RecvData(it->second))
				{
					// ͨѶʧ�ܣ���ʾ�пͻ��뿪����ȫ�ֵ�set��ȡ��
					FD_CLR(readyfd, &_reads);
					Close(readyfd);
					delete it->second;
					_cli_Array.erase(it);
				}
			}
#else			   /* ����ƽ̨ */
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

	// ������Ϣ�߳���������
	void CreateThread()
	{
		// �����߳�����select���
		auto route = std::mem_fn(&RecvMsgProc::Thr_Route);	// mem_fun()����Ա����ת��Ϊ��������
		_pthread = std::thread(route, this);
		_pthread.detach();
	}

	/*       -------���û�����ճ������---------      */
	// ��������������յ�������ֱ�ӷŵ��ÿͻ�����Ϣ��������
	bool RecvData(ClientSock* pcli)
	{
		char* msgbf = pcli->GetMsgBuf();	// ��ȡ������
		int pos = pcli->GetEndPos();		// ��ȡ�������Ľ�βλ��

		char* recv_buf = msgbf + pos;		// �ӻ������Ľ�βλ�ý���
		int recv_len = recv(pcli->GetFd(), recv_buf, RECVBUFSIZE - pos, 0);
		_pEvent->NetRecv(pcli);		// ����������Ϣ�����¼�
		if (recv_len <= 0)
		{
			if (_pEvent)
				_pEvent->NetCliQuit(pcli);		// �����ͻ��뿪�����¼�
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
				_pEvent->NetMsgHandle(pcli, header);		// ������Ϣ�����¼�
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

	// �����û���ӵ����������
	void AddClientToBuffer(ClientSock* pcli)
	{
		std::lock_guard<std::mutex> lock(_mutex);	// ����RAII����ʵ�ֵ��Խ���
		_cli_ArrayBuffer.push_back(pcli);
	}

	// ��ѯ��Ϣ�����߳� MsgHandlerProc �����пͻ�����
	size_t getClientAmount()
	{
		return _cli_Array.size() + _cli_ArrayBuffer.size();
	}


private:
	std::map<SOCKET, ClientSock*> _cli_Array;		// ��ʽ����
	std::vector<ClientSock*> _cli_ArrayBuffer;		// �������<�ٽ���>

	std::mutex _mutex;
	std::thread _pthread;
	NetEvent* _pEvent;		// �����¼�����--JOIN��QUIT��MsgHandler
};



#endif // !_RECVMSGPROC_HPP_
