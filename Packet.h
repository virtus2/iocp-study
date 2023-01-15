#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// 클라이언트가 보낸 패킷을 저장하는 구조체
struct PacketData
{
	UINT32 SessionIndex = 0;
	UINT32 DataSize = 0;
	char* Data = nullptr;

	void Set(PacketData& Value)
	{
		SessionIndex = Value.SessionIndex;
		DataSize = Value.DataSize;
		Data = new char[Value.DataSize];
		CopyMemory(Data, Value.Data, Value.DataSize);
	}

	void Set(UINT32 Index, UINT32 Size, char* DataToSet)
	{
		SessionIndex = Index;
		DataSize = Size;
		Data = new char[Size];
		CopyMemory(Data, DataToSet, Size);
	}

	void Release()
	{
		delete Data;
	}
};