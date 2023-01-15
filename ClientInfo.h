#pragma once

#include "Define.h"
#include <stdio.h>

class ClientInfo
{
public:
	ClientInfo()
	{
		ZeroMemory(&RecvOverlappedEx, sizeof(RecvOverlappedEx));
		SocketClient = INVALID_SOCKET;
	}

	void Init(const UINT32 Idx)
	{
		Index = Idx;
	}

	UINT32 GetIndex() { return Index; }
	bool IsConnected() { return SocketClient != INVALID_SOCKET; }
	SOCKET GetSocket() { return SocketClient; }
	char* GetRecvBuffer() { return RecvBuf; }

	bool OnConnect(HANDLE IocpHandle, SOCKET Socket)
	{
		SocketClient = Socket;
		Clear();

		if(BindIoCompletionPort(IocpHandle) == false)
		{
			return false;
		}

		return BindRecv();
	}

	void Close(bool Force=false)
	{
		struct linger Linger = { 0,0 };
		if(Force)
		{
			Linger.l_onoff = 1;
		}

		shutdown(SocketClient, SD_BOTH);
		setsockopt(SocketClient, SOL_SOCKET, SO_LINGER, (char*)&Linger, sizeof(Linger));
		closesocket(SocketClient);
		SocketClient = INVALID_SOCKET;
	}

	void Clear() {}

	bool BindIoCompletionPort(HANDLE IocpHandle)
	{
		auto Handle = CreateIoCompletionPort((HANDLE)SocketClient, IocpHandle, (ULONG_PTR)(this), 0);
		if(Handle == INVALID_HANDLE_VALUE)
		{
			printf("[에러] CreateIoCompletionPort() 함수 실패: %d\n", WSAGetLastError());
			return false;
		}
		return true;
	}

	bool BindRecv()
	{
		DWORD Flag = 0;
		DWORD RecvNumBytes = 0;

		RecvOverlappedEx.WsaBuf.len = MAX_SOCKBUF;
		RecvOverlappedEx.WsaBuf.buf = RecvBuf;
		RecvOverlappedEx.Operation = IOOperation::RECV;

		int Ret = WSARecv(SocketClient,
			&(RecvOverlappedEx.WsaBuf),
			1,
			&RecvNumBytes,
			&Flag,
			(LPWSAOVERLAPPED) & (RecvOverlappedEx),
			NULL);

		if (Ret == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			printf("[에러] WSARecv()함수 실패: %d\n", WSAGetLastError());
			return false;
		}
		return true;
	}

	bool SendMsg(const UINT32 dataSize, char* Msg)
	{
		auto SendOverlappedEx = new OverlappedEx;
		ZeroMemory(SendOverlappedEx, sizeof(OverlappedEx));
		SendOverlappedEx->WsaBuf.len = dataSize;
		SendOverlappedEx->WsaBuf.buf = new char[dataSize];
		CopyMemory(SendOverlappedEx->WsaBuf.buf, Msg, dataSize);
		SendOverlappedEx->Operation = IOOperation::SEND;

		DWORD RecvNumBytes = 0;
		int Ret = WSASend(SocketClient,
			&(SendOverlappedEx->WsaBuf),
			1,
			&RecvNumBytes,
			0,
			(LPWSAOVERLAPPED)SendOverlappedEx,
			NULL);

		if (Ret == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			printf("[에러] WSASend()함수 실패: %d\n", WSAGetLastError());
			return false;
		}
		return true;
	}

	void SendCompleted(const UINT32 dataSize)
	{
		printf("[송신 완료] bytes: %d\n", dataSize);
	}

private:

	INT32 Index = 0;
	SOCKET SocketClient;				// Client와 연결되는 소켓
	OverlappedEx RecvOverlappedEx;		// RECV Overlapped I/O 작업을 위한 변수
	char RecvBuf[MAX_SOCKBUF];			// 데이터 버퍼
	
};