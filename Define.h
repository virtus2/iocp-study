#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>

const UINT32 MAX_SOCK_RECVBUF = 256;
const UINT32 MAX_SOCK_SENDBUF = 4096;
const UINT64 RE_USE_SESSION_WAIT_TIMESEC = 3;

enum class IOOperation
{
	RECV,
	SEND,
	ACCEPT
};

// WSAOVERLAPPED 구조체를 확장시켜서 필요한 정보를 더 넣었다.
struct OverlappedEx
{
	WSAOVERLAPPED WsaOverlapped;		// Overlapped I/O 구조체
	WSABUF WsaBuf;						// Overlapped I/O 작업 버퍼
	IOOperation Operation;				// 작업 동작 종류
	UINT32 SessionIndex = 0;
};

