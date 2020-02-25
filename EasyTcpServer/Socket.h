#pragma once

#define WIN32_LEAN_AND_MEAN			// ����windows���WinSock2��ĺ��ض���
#define _WINSOCK_DEPRECATED_NO_WARNINGS			// itoaת�������汾����
#define PORT 4567			// ����˰󶨶˿ں�
#define TIME_AWAKE 1		// select��ѯʱ��
//#pragma comment(lib, "ws2_32.lib")     // ������->����->��������������ӿ�����

#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

enum CMD {
	_LOGIN = 1,
	_LOGOUT,
	_LOGIN_RESULT = 10,
	_LOGOUT_RESULT,
	_NEWUSER_JOIN = 100,
	_ERROR = 1000
};

// ����ṹ�����ݸ�ʽ
struct DataHeader {
	short _dataLength;
	CMD _cmd;		// ����
};

struct Login :public DataHeader {
	Login() {
		this->_cmd = _LOGIN;
		this->_dataLength = sizeof(Login);
	}
	char _userName[32];
	char _passwd[32];
};

struct LoginResult :public DataHeader {
	LoginResult() {
		this->_cmd = _LOGIN_RESULT;
		this->_dataLength = sizeof(LoginResult);
		this->_result = false;
	}
	bool _result;
};

struct Logout :public DataHeader {
	Logout() {
		this->_cmd = _LOGOUT;
		this->_dataLength = sizeof(Logout);
	}
	char _userName[32];
};

struct LogoutResult :public DataHeader {
	LogoutResult() {
		this->_cmd = _LOGOUT_RESULT;
		this->_dataLength = sizeof(LogoutResult);
		this->_result = false;
	}
	bool _result;
};

struct Response : public DataHeader {
	Response() {
		this->_cmd = _ERROR;
		this->_dataLength = sizeof(Response);
		this->_result = false;
	}
	bool _result;
};

struct NewJoin :public DataHeader {
	NewJoin() {
		this->_cmd = _NEWUSER_JOIN;
		this->_dataLength = sizeof(NewJoin);
		this->_sockfd = INVALID_SOCKET;
	}
	SOCKET _sockfd;
};