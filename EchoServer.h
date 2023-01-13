#pragma once

#include "IOCPServer.h"

class EchoServer : public IOCPServer
{
	virtual void OnConnect(const UINT32 ClientIndex) override
	{
		printf("[OnConnect] 클라이언트 접속: Index %d\n", ClientIndex);
	}
	virtual void OnClose(const UINT32 ClientIndex) override
	{
		printf("[OnClose] 클라이언트 연결 해제: Index %d\n", ClientIndex);
	}
	virtual void OnReceive(const UINT32 ClientIndex, const UINT32 Size, char* Data) override
	{
		printf("[OnReceive] 클라이언트 수신: Index %d, Size: %d, Data: %s\n", ClientIndex, Size, Data);
	}
};