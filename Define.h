#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

#define MAX_SOCKBUF 1024	// 패킷 크기
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

struct ClientInfo
{
	SOCKET SocketClient;				// Client와 연결되는 소켓
	OverlappedEx RecvOverlappedEx;		// RECV Overlapped I/O 작업을 위한 변수
	OverlappedEx SendOverlappedEx;		// SEND Overlapped I/O 작업을 위한 변수

	char RecvBuf[MAX_SOCKBUF];			// 수신 데이터 버퍼
	char SendBuf[MAX_SOCKBUF];			// 송신 데이터 버퍼

	ClientInfo()
	{
		ZeroMemory(&RecvOverlappedEx, sizeof(RecvOverlappedEx));
		ZeroMemory(&SendOverlappedEx, sizeof(SendOverlappedEx));
		SocketClient = INVALID_SOCKET;
	}
};