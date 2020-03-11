#ifndef _NETEVENT_HPP_
#define _NETEVENT_HPP_

#include "ClientSock.hpp"
// �����¼��ӿ���
class NetEvent
{
public:
	// �ͻ��˼���
	virtual void NetCliJoin(ClientSock* pcli) = 0;

	// �ͻ����˳��¼�
	virtual void NetCliQuit(ClientSock* pcli) = 0;

	// ��Ϣ�����¼�
	virtual void NetMsgHandle(ClientSock* pcli, DataHeader* header) = 0;

	// �����¼�
	virtual void NetRecv(ClientSock* pcli) = 0;
};

#endif // !_NETEVENT_HPP_

