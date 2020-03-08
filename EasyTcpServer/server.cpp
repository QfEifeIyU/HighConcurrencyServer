#include "tcpServer.hpp"

// 子定制服务器
class MyServer : public TcpServer
{
public:

	// 客户端加入
	virtual void NetClientJoin(Client* pcli)
	{
		++_clientAmount;
		printf("cli<%d> JOIN.\n", pcli->GetFd());
	}

	// 客户端退出
	virtual void NetClientQuit(Client* pcli)
	{
		--_clientAmount;
		printf("cli<%d> QUIT.\n", pcli->GetFd());
	}

	// 消息处理事件
	virtual void NetMessageHandle(Client* pcli, DataHeader* header)
	{
		++_packageAmount;
		switch (header->_cmd)
		{
			case _LOGIN:
			{		// 登录->返回登录信息
				Login* login = reinterpret_cast<Login*>(header);
				// here is 判断登录信息，构造响应包
				LoginResult result;
				result._result = true;
				pcli->SendData(&result);

			}	break;
			case _LOGOUT:
			{		// 注销->返回下线信息
				Logout* logout = reinterpret_cast<Logout*>(header);
				// here is 确认退出下线，构造响应包
				LogoutResult result;
				result._result = true;
				pcli->SendData(&result);
			}	break;
			default:
			{		// 发送了错误的命令->返回错误头
				printf("\t$error, 接收到错误的命令@%d\n", header->_cmd);
				DataHeader result;
				pcli->SendData(&result);
			}	break;
		}
	}
private:

};



int main() {

	MyServer* s = new MyServer();
	s->Bind("127.0.0.1", 0x4567);
	s->Listen(5);
	s->CreateHandleMessageProcess(4);		// 创建线程微程序	CPU_THREAD_AMOUNT

	std::thread ui(th_route);
	ui.detach();
	
	while (g_TaskRuning)
	{
		s->ActTask();
	}
	s->CleanUp();

	return 0;
}
