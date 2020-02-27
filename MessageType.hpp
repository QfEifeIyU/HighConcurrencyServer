#pragma once

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN			// ����windows���WinSock2��ĺ��ض���
	#define _WINSOCK_DEPRECATED_NO_WARNINGS			// itoaת�������汾����
	#include <windows.h>
	#include <WinSock2.h>
	//#pragma comment(lib, "ws2_32.lib")     // ������->����->��������������ӿ�����
#else
	#include <unistd.h>     // std 
	#include <arpa/inet.h>  // socket
	#include <string.h>     // strcpy
	typedef int SOCKET;     // ��Ӧsockfd
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif 

#define TIME_AWAKE 0		// select��ѯʱ��
//const unsigned short PORT = 0x4567;		// ����˰󶨶˿ں�

enum CMD 
{
	_LOGIN = 0,		_LOGOUT = 1,
	_LOGIN_RESULT = 10,		_LOGOUT_RESULT = 11,
	_NEWUSER_JOIN = 100,	_USER_QUIT = 111,
	_ERROR = 1000
};

// ����ṹ�����ݸ�ʽ
struct DataHeader 
{
	DataHeader() 
	{
		_dataLength = sizeof(DataHeader);
		_cmd = _ERROR;
	}
	
	short _dataLength;
	CMD _cmd;		// ����
};

struct Login :public DataHeader 
{
	Login() 
	{
		this->_cmd = _LOGIN;
		this->_dataLength = sizeof(Login);
	}
	char _userName[32];
	char _passwd[32];
};

struct LoginResult :public DataHeader 
{
	LoginResult() 
	{
		this->_cmd = _LOGIN_RESULT;
		this->_dataLength = sizeof(LoginResult);
		this->_result = false;
	}
	bool _result;
};

struct Logout :public DataHeader 
{
	Logout() 
	{
		this->_cmd = _LOGOUT;
		this->_dataLength = sizeof(Logout);
	}
	char _userName[32];
};

struct LogoutResult :public DataHeader 
{
	LogoutResult() 
	{
		this->_cmd = _LOGOUT_RESULT;
		this->_dataLength = sizeof(LogoutResult);
		this->_result = false;
	}
	bool _result;
};

struct Response : public DataHeader 
{
	Response() 
	{
		this->_cmd = _ERROR;
		this->_dataLength = sizeof(Response);
		this->_result = false;
	}
	bool _result;
};

struct NewJoin :public DataHeader 
{
	NewJoin() 
	{
		this->_cmd = _NEWUSER_JOIN;
		this->_dataLength = sizeof(NewJoin);
		this->_sockfd = INVALID_SOCKET;
	}
	SOCKET _sockfd;
};


#ifdef _WIN32
// �windows socket 2.x����
void StartUp() 
{
	WORD ver = MAKEWORD(2, 2);
	WSADATA data;
	WSAStartup(ver, &data);
}
void CleanNet() 
{
	WSACleanup();
}
#endif

// ���õ�ַ��Ϣ
void SetAddr(sockaddr_in* addr, const char* ip, const unsigned short port) 
{
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
#ifdef _WIN32 
	addr->sin_addr.S_un.S_addr = inet_addr(ip);
#else
	addr->sin_addr.s_addr = inet_addr(ip);
#endif
}