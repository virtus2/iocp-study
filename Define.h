#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

#define MAX_SOCKBUF 256	// 패킷 크기
#define MAX_WORKER_THREAD 4 // 쓰레드풀에 넣을 쓰레드 수

enum class IOOperation
{
	RECV,
	SEND
};

// WSAOVERLAPPED 구조체를 확장시켜서 필요한 정보를 더 넣었다.
struct OverlappedEx
{
	WSAOVERLAPPED WsaOverlapped;		// Overlapped I/O 구조체
	SOCKET SocketClient;				// 클라이언트 소켓
	WSABUF WsaBuf;						// Overlapped I/O 작업 버퍼
	IOOperation Operation;				// 작업 동작 종류
};

