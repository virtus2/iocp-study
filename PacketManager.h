#pragma once

#include "Packet.h"

#include <unordered_map>
#include <deque>
#include <functional>
#include <thread>
#include <mutex>

class PacketManager
{
public:
	PacketManager() = default;
	~PacketManager() = default;

	void Init(const UINT32 MaxClient);
	bool Run();
	void End();
	void ReceivePacketData(const UINT32 ClientIndex, const UINT32 Size, char* Data);
	void PushSystempacket(PacketInfo Packet);
	std::function<void(UINT32, UINT32, char*)> SendPacketFunc;

private:
};
