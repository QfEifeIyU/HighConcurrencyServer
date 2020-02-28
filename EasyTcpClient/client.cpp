#include "tcpClient.hpp"
#include <thread>
// 线程入口函数--构造请求
void th_route(TcpClient* cli) 
{
	while (cli->IsRunning()) 
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
			printf("<%d>线程 thread 退出\n", cli->GetFd());
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
	cli.Connect("127.0.0.1", 0x4567);

	Login login;
	strcpy(login._userName, "lyb");
	strcpy(login._passwd, "123456");

	while (cli.IsRunning()) 
	{
		cli.StartSelect();
		cli.SendData(&login);
	}	
	cli.CleanUp();

	getchar();
	return 0;
}