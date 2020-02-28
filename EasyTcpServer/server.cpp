#include "tcpServer.hpp"


int main() {

	TcpServer s;
	//s.InitSocket();
	s.Bind("127.0.0.1", 0x4567);
	s.Listen(5);

	while (s.IsRunning()) {
		s.StartSelect();
	}
	s.CleanUp();
	getchar();
	return 0;
}
