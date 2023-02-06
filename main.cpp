#include "EchoServer.h"
#include <string>
#include <iostream>

const UINT16 SERVER_PORT = 11021;
const UINT16 MAX_CLIENT = 100;		//총 접속할수 있는 클라이언트 수
const UINT32 MAX_IO_WORKER_THREAD = 4;

int main()
{
	EchoServer Server;

	//소켓을 초기화
	Server.InitSocket(MAX_IO_WORKER_THREAD);

	//소켓과 서버 주소를 연결하고 등록 시킨다.
	Server.BindAndListen(SERVER_PORT);

	Server.Run(MAX_CLIENT);

	printf("종료하려면 [quit]를 입력하세요.\n");
	while(true)
	{
		std::string InputCmd;
		std::getline(std::cin, InputCmd);
		if(InputCmd == "quit")
		{
			break;
		}
	}

	Server.End();
	return 0;
}
