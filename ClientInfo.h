#pragma once

#include <mutex>

#include "Define.h"
#include <stdio.h>
#include <mutex>
#include <queue>

class ClientInfo
{
public:
	ClientInfo()
	{
		ZeroMemory(&RecvOverlappedEx, sizeof(RecvOverlappedEx)); 
		SocketClient = INVALID_SOCKET;
	}

	void Init(const UINT32 Idx, HANDLE IocpHandle)
	{
		Index = Idx;
		IOCPHandle = IocpHandle;
	}

	UINT32 GetIndex() { return Index; }
	bool IsConnected() { return SocketClient != INVALID_SOCKET; }
	SOCKET GetSocket() { return SocketClient; }
	char* GetRecvBuffer() { return RecvBuf; }
	UINT64 GetLatestClosedTimeSec() { return LatestClosedTimeSec; }

	bool OnConnect(HANDLE IocpHandle, SOCKET Socket)
	{
		SocketClient = Socket;
		IsConnect = 1;
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
		IsConnect = 0;
		LatestClosedTimeSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		closesocket(SocketClient);
		SocketClient = INVALID_SOCKET;
	}

	void Clear()
	{
	}

	bool PostAccept(SOCKET ListenSocket, const UINT64 CurrentTimeSec)
	{
		printf("Post Accept. client Index :%d\n", GetIndex());
		LatestClosedTimeSec = UINT32_MAX;

		SocketClient = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if(INVALID_SOCKET == SocketClient)
		{
			printf("client socket WSASocket Error: %d\n", GetLastError());
		}

		ZeroMemory(&AcceptContext, sizeof(OverlappedEx));

		DWORD bytes = 0;
		DWORD flags = 0;
		AcceptContext.WsaBuf.len = 0;
		AcceptContext.WsaBuf.buf = nullptr;
		AcceptContext.Operation = IOOperation::ACCEPT;
		AcceptContext.SessionIndex = Index;

		if (FALSE == AcceptEx(ListenSocket, SocketClient, AcceptBuf, 0,
			sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytes, (LPWSAOVERLAPPED) & (AcceptContext)))
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				printf("AcceptEx Error: %d\n", GetLastError());
				return false;
			}
		}
		return true;
	}

	bool AcceptCompletion()
	{
		printf("AcceptCompletion: SessionIndex(%d)\n", Index);
		if(OnConnect(IOCPHandle, SocketClient) == false)
		{
			return false;
		}
		SOCKADDR_IN ClientAddr;
		int AddrLen = sizeof(SOCKADDR_IN);
		char clientIP[32] = { 0, };
		inet_ntop(AF_INET, &(ClientAddr.sin_addr), clientIP, 32 - 1);
		printf("클라이언트 접속: IP(%s) SOCKET(%d)\n", clientIP, (int)SocketClient);
		return true;
	}

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

		RecvOverlappedEx.WsaBuf.len = MAX_SOCK_RECVBUF;
		RecvOverlappedEx.WsaBuf.buf = RecvBuf;
		RecvOverlappedEx.Operation = IOOperation::RECV;

		int Ret = WSARecv(SocketClient,
			&(RecvOverlappedEx.WsaBuf),
			1,
			&RecvNumBytes,
			&Flag,
			(LPWSAOVERLAPPED) &RecvOverlappedEx,
			NULL);

		if (Ret == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			printf("[에러] WSARecv()함수 실패: %d\n", WSAGetLastError());
			return false;
		}
		return true;
	}

	bool SendMsg(const UINT32 DataSize, char* Msg)
	{
		auto sendOverlappedEx = new OverlappedEx;
		ZeroMemory(sendOverlappedEx, sizeof(OverlappedEx));
		sendOverlappedEx->WsaBuf.len = DataSize;
		sendOverlappedEx->WsaBuf.buf = new char[DataSize];
		CopyMemory(sendOverlappedEx->WsaBuf.buf, Msg, DataSize);
		sendOverlappedEx->Operation = IOOperation::SEND;

		std::lock_guard<std::mutex> guard(SendLock);
		SendDataQueue.push(sendOverlappedEx);
		if(SendDataQueue.size() == 1)
		{
			SendIO();
		}
		return true;
	}

	
	void SendCompleted(const UINT32 DataSize)
	{
		printf("[송신 완료] bytes: %d\n", DataSize);
		std::lock_guard<std::mutex> guard(SendLock);
		delete[] SendDataQueue.front()->WsaBuf.buf;
		delete SendDataQueue.front();
		SendDataQueue.pop();
		if(SendDataQueue.empty()==false)
		{
			SendIO();
		}
	}

private:
	bool SendIO()
	{
		auto SendOverlappedEx = SendDataQueue.front();
		
		DWORD SendNumBytes = 0;
		int Ret = WSASend(SocketClient,
			&(SendOverlappedEx->WsaBuf),
			1,
			&SendNumBytes,
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

	INT32 Index = 0;
	HANDLE IOCPHandle = INVALID_HANDLE_VALUE;

	INT64 IsConnect = 0;
	UINT64 LatestClosedTimeSec = 0;

	SOCKET SocketClient = 0;

	OverlappedEx AcceptContext;
	char AcceptBuf[64];

	OverlappedEx RecvOverlappedEx;		// RECV Overlapped I/O 작업을 위한 변수
	char RecvBuf[MAX_SOCK_RECVBUF];			// 데이터 버퍼

	std::queue<OverlappedEx*> SendDataQueue;
	std::mutex SendLock;

};