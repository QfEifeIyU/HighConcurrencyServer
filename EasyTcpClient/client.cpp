#include "tcpClient.hpp"

// 线程入口函数--构造请求
void th_route(TcpClient* cli) 
{
	while (true) 
	{
		string request = "";
		cin >> request;

		if (request == "login") 
		{
			Login login;
			strcpy(login._userName, "lyb");
			strcpy(login._passwd, "123456");
			cli->SendData(&login);	
		}
		else if (request == "logout") 
		{
			Logout logout;
			strcpy(logout._userName, "lyb");
			cli->SendData(&logout);
		}
		else if (request == "exit") 
		{
			DataHeader request;
			request._cmd = _USER_QUIT;
			cli->SendData(&request);
			cout << "线程 thread 退出\n";
			cli->CleanUp();
			break;
		}
		else 
		{
			DataHeader request;
			cli->SendData(&request);
		}
	}
}

int main() 
{
	TcpClient cli;
	cli.Connect("106.15.187.148", 0x4567);
	thread t(th_route, &cli);
	t.detach();
	while (cli.IsRunning()) 
	{
		cli.StartSelect();
	}	
	cli.CleanUp();
	return 0;
}
