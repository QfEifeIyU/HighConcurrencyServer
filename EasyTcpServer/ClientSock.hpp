#ifndef _CLIENTSOCK_HPP_
#define	_CLIENTSOCK_HPP_

// server ����ͻ��˵���������
#ifdef _WIN32
	#include "../Socket/MessageType.hpp"
	#include "../Socket/Tools.hpp"
#else
	#include "MessageType.hpp"
	#include "Tools.hpp"
#endif


#include <string.h>
class ClientSock
{
public:
	ClientSock(SOCKET fd = INVALID_SOCKET)
	{
		_cli_fd = fd;
		memset(_msg_recvBf, 0, RECVBUFSIZE);
		_endof_recvBf = 0;
	}
	~ClientSock()
	{
		if (_cli_fd != INVALID_SOCKET)
		{
			Close(_cli_fd);
			_cli_fd = INVALID_SOCKET;
		}
	}

	// ��ȡ��ǰ�ͻ��˵�socket fd
	SOCKET GetFd()
	{
		return _cli_fd;
	}
	// ��ȡ��ǰ�ͻ��˵Ļ�����
	char* GetMsgBuf()
	{
		return _msg_recvBf;
	}
	// ��ȡ�������Ľ���λ
	int GetEndPos()
	{
		return _endof_recvBf;
	}
	// ���û������Ľ���λ
	void SetEndPos(int pos)
	{
		_endof_recvBf = pos;
	}

	// ������Ӧ��
	bool SendData(DataHeader* header)
	{
		//const char* data = reinterpret_cast<const char*>(header);
		//int dataLen = header->_dataLength;
		//while (_cli_fd != INVALID_SOCKET)
		//{
		//	// ������������
		//	if (_endof_sendBf + dataLen >= SENDBUFSIZE)
		//	{
		//		int copyLen = SENDBUFSIZE - _endof_sendBf;	// ���Կ����ĳ���
		//		memcpy(_msg_sendBf + _endof_sendBf, data, copyLen);
		//		data += copyLen;
		//		dataLen -= copyLen;
		//		send(_cli_fd, _msg_sendBf, SENDBUFSIZE, 0);
		//		_endof_sendBf = 0;
		//	}
		//	else
		//	{
		//		memcpy(_msg_sendBf + _endof_sendBf, data, dataLen);
		//		_endof_sendBf += dataLen;
		//		break;
		//	}
		//}
		//return true;
		if (_cli_fd != INVALID_SOCKET)
		{
			int body = header->_dataLength;
			// ��β����һ��body���ȷ��ͻ�����
			if (_endof_sendBf + body > SENDBUFSIZE)
			{
				send(_cli_fd, _msg_sendBf, _endof_sendBf, 0);		// �Ƚ����л���������ȫ�����ͳ�ȥ
				//std::cout << _endof_sendBf << "\n";
				_endof_sendBf = 0;
			}
			memcpy(_msg_sendBf + _endof_sendBf, header, body);
			_endof_sendBf += body;
			return true;
		}
		return false;
		



		if (_cli_fd != INVALID_SOCKET && header)
		{
			send(_cli_fd, reinterpret_cast<const char*>(header), header->_dataLength, 0);
			return true;
		}
		return false;
	}

private:
	SOCKET _cli_fd;
	char _msg_recvBf[RECVBUFSIZE];
	char _msg_sendBf[SENDBUFSIZE];
	int _endof_recvBf;
	int _endof_sendBf;
};


#endif // _CLIENTSOCK_HPP_

