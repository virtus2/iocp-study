#pragma once

#include "IOCPServer.h"
#include "vector"

class ChatServer: public IOCPServer
{
public:
	ChatServer() = default;
	virtual ~ChatServer() = default;

	virtual void OnConnect(const UINT32 ClientIndex) override
	{
		printf("[OnConnect] 클라이언트 Index(%d)\n", ClientIndex);
	}

	virtual void OnClose(const UINT32 ClientIndex) override
	{
		printf("[OnClose] 클라이언트 Index(%d)\n", ClientIndex);
	}

	virtual void OnReceive(const UINT32 ClientIndex, const UINT32 Size, char* Data) override
	{
		printf("[OnReceive] 클라이언트 Index(%d)\n", ClientIndex);
	}

	void Run(const UINT32 MaxClient)
	{
		StartServer(MaxClient);
	}

	void End()
	{
		DestroyThread();
	}

private:
	std::unique_ptr<PacketManager> PacketManager;
};