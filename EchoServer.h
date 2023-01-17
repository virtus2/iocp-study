#pragma once

#include "IOCPServer.h"
#include "Packet.h"

#include <vector>
#include <deque>
#include <thread>
#include <mutex>

class EchoServer : public IOCPServer
{
public:
	EchoServer() = default;
	virtual ~EchoServer() = default;

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
		printf("[OnReceive] 클라이언트 수신: Index %d, Size: %d\n", ClientIndex, Size);

		PacketData Packet;
		Packet.Set(ClientIndex, Size, Data);

		std::lock_guard<std::mutex> guard(Lock);
		PacketDataQueue.push_back(Packet);
	}

	void Run(const UINT32 MaxClients)
	{
		IsRunningProcessThread = true;
		ProcessThread = std::thread([this]() {ProcessPacket(); });

		StartServer(MaxClients);
	}

	void End()
	{
		IsRunningProcessThread = false;
		if(ProcessThread.joinable())
		{
			ProcessThread.join();
		}
		DestroyThread();
	}

private:
	void ProcessPacket()
	{
		while (true)
		{
			auto PacketData = DequeuePacketData();
			if(PacketData.DataSize != 0)
			{
				SendMsg(PacketData.SessionIndex, PacketData.DataSize, PacketData.Data);
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
	}

	PacketData DequeuePacketData()
	{
		PacketData Packet;

		std::lock_guard<std::mutex> guard(Lock);
		if(PacketDataQueue.empty())
		{
			return PacketData();
		}
		Packet.Set(PacketDataQueue.front());
		PacketDataQueue.pop_front();
		return Packet;
	}

	std::thread ProcessThread;
	std::mutex Lock;
	std::deque<PacketData> PacketDataQueue;
	bool IsRunningProcessThread = false;
};