#ifndef _NETEVENT_HPP_
#define _NETEVENT_HPP_

#include "ClientSock.hpp"
// 网络事件接口类
class NetEvent
{
public:
	// 客户端加入
	virtual void NetCliJoin(ClientSock* pcli) = 0;

	// 客户端退出事件
	virtual void NetCliQuit(ClientSock* pcli) = 0;

	// 消息处理事件
	virtual void NetMsgHandle(ClientSock* pcli, DataHeader* header) = 0;

	// 接收事件
	virtual void NetRecv(ClientSock* pcli) = 0;
};

#endif // !_NETEVENT_HPP_

