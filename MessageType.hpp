#ifndef _MessageHeader_hpp_
#define _MessageHeader_hpp_

enum CMD
{
	_LOGIN = 0,
	_LOGOUT = 1,
	_LOGIN_RESULT = 2,
	_LOGOUT_RESULT = 3,
	_NEWUSER_JOIN = 4,
	//_USER_QUIT,
	_ERROR = 5
};

// 传输结构化数据格式
struct DataHeader
{
	DataHeader()
	{
		_dataLength = sizeof(DataHeader);
		_cmd = _ERROR;
	}

	short _dataLength;
	CMD _cmd;		// 命令
};

struct Login :public DataHeader
{
	Login()
		:_userName("lyb"), _passwd("123456"), _data("")
	{
		this->_cmd = _LOGIN;
		this->_dataLength = sizeof(Login);
	}
	char _userName[32];
	char _passwd[32];
	char _data[28];
};

struct LoginResult :public DataHeader
{
	LoginResult()
		:_data("")
	{
		this->_cmd = _LOGIN_RESULT;
		this->_dataLength = sizeof(LoginResult);
		this->_result = false;
	}
	bool _result;
	char _data[90];
};

struct Logout :public DataHeader
{
	Logout()
		:_userName("")
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
		:_data("")
	{
		this->_cmd = _ERROR;
		this->_dataLength = sizeof(Response);
		this->_result = false;
	}
	bool _result;
	char _data[90];
};

struct NewJoin :public DataHeader
{
	NewJoin()
	{
		this->_cmd = _NEWUSER_JOIN;
		this->_dataLength = sizeof(NewJoin);
		this->_fd = -1;
	}
	int _fd;
};

#endif	// end of MessageType