#pragma once
#pragma comment(lib, "ws2_32")

#include "Define.h"
#include <thread>
#include <vector>

class IOCPServer
{
public:
	IOCPServer() {}
	virtual ~IOCPServer()
	{
		WSACleanup();
	}

	// 소켓 초기화 함수
	bool InitSocket()
	{
		WSADATA WsaData;

		int ret = WSAStartup(MAKEWORD(2, 2), &WsaData);
		if(0 != ret)
		{
			printf("[에러] WSAStartup() 함수 실패: %d\n", WSAGetLastError());
			return false;
		}

		// 연결지향형 TCP, Overlapped I/O 소켓 생성
		ListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
		if(INVALID_SOCKET == ListenSocket)
		{
			printf("[에러] WSASocket() 함수 실패: %d\n", WSAGetLastError());
			return false;
		}
		printf("소켓 초기화 성공\n");
		return true;
	}

	//
	// 서버용 함수
	//
	bool BindAndListen(int BindPort)
	{
		SOCKADDR_IN ServerAddr;
		ServerAddr.sin_family = AF_INET;
		ServerAddr.sin_port = htons(BindPort); // 서버 포트 설정
		// 어떤 주소에서라도 들어오는 접속을 받아들인다.
		// 만약 한 아이피에서만 접속을 받고 싶다면 그 주소를 inet_addr함수를 이용해 넣으면 된다.
		ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		// 위에서 지정한 서버 주소 정보와 IOCompletionPort 소켓을 연결한다.
		int Ret = bind(ListenSocket, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR_IN));
		if(0 != Ret)
		{
			printf("[에러] bind() 함수 실패: %d\n", WSAGetLastError());
			return false;
		}

		// 접속 요청을 받아들이기 위해 소켓을 등록하고 접속 대기큐를 5개로 설정한다.
		Ret = listen(ListenSocket, 5);
		if(0 != Ret)
		{
			printf("[에러] listen() 함수 실패: %d\n", WSAGetLastError());
			return false;
		}

		printf("서버 등록 성공\n");
		return true;
	}
	
	bool StartServer(const UINT32 maxClientCount)
	{
		CreateClient(maxClientCount);

		IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKER_THREAD);
		if(NULL == IOCPHandle)
		{
			printf("[에러] CreateIoCompletionPort() 함수 실패: %d\n", WSAGetLastError());
			return false;
		}

		bool Ret = CreateWorkerThread();
		if(false == Ret)
		{
			return false;
		}

		Ret = CreateAcceptorThread();
		if(false == Ret)
		{
			return false;
		}
		printf("서버 시작\n");
		return true;
	}
	
	void DestroyThread()
	{
		IsWorkerRunning = false;
		CloseHandle(IOCPHandle);

		for(auto& th : IOWorkerThreads)
		{
			if(th.joinable())
			{
				th.join();
			}
		}

		// Acceptor 쓰레드 종료
		IsAcceptorRunning = false;
		closesocket(ListenSocket);
		if(AcceptorThread.joinable())
		{
			AcceptorThread.join();
		}
	}

	// 네트워크 이벤트를 처리할 함수들
	virtual void OnConnect(const UINT32 ClientIndex) {}
	virtual void OnClose(const UINT32 ClientIndex) {}
	virtual void OnReceive(const UINT32 ClientIndex, const UINT32 Size, char* Data) {}

private:
	void CreateClient(const UINT32 maxClientCount)
	{
		for(UINT32 i = 0; i<maxClientCount; ++i)
		{
			ClientInfos.emplace_back();
		}
	}

	bool CreateWorkerThread()
	{
		unsigned int ThreadId = 0;

		for(int i=0; i< MAX_WORKER_THREAD; ++i)
		{
			IOWorkerThreads.emplace_back([this]() { IOWorker(); });
		}
		printf("WorkerThread 시작\n");
		return true;
	}

	bool CreateAcceptorThread()
	{
		AcceptorThread = std::thread([this]() { Acceptor(); });

		printf("AcceptorThread 시작\n");
		return true;
	}

	ClientInfo* GetEmptyClientInfo()
	{
		for(auto& client : ClientInfos)
		{
			if(INVALID_SOCKET == client.SocketClient)
			{
				return &client;
			}
		}
		return nullptr;
	}

	bool bindIOCompletionPort(ClientInfo* Info)
	{
		auto hIOCP = CreateIoCompletionPort((HANDLE)Info->SocketClient, IOCPHandle, (ULONG_PTR)(Info), 0);
		if(NULL == hIOCP || IOCPHandle != hIOCP)
		{
			printf("[에러] CreateIoCompletionPort() 함수 실패: %d\n", WSAGetLastError());
			return false;
		}
		return true;
	}

	bool BindRecv(ClientInfo* Info)
	{
		DWORD Flag = 0;
		DWORD RecvNumBytes = 0;

		Info->RecvOverlappedEx.WsaBuf.len = MAX_SOCKBUF;
		Info->RecvOverlappedEx.WsaBuf.buf = Info->RecvBuf;
		Info->RecvOverlappedEx.Operation = IOOperation::RECV;

		int Ret = WSARecv(Info->SocketClient,
			&(Info->RecvOverlappedEx.WsaBuf),
			1,
			&RecvNumBytes,
			&Flag,
			(LPWSAOVERLAPPED) & (Info->RecvOverlappedEx),
			NULL);

		if(SOCKET_ERROR == Ret && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			printf("[에러] WSARecv()함수 실패: %d\n", WSAGetLastError());
			return false;
		}
		return true;
	}

	bool SendMsg(ClientInfo* Info, char* Msg, int Len)
	{
		DWORD RecvNumBytes = 0;

		CopyMemory(Info->SendBuf, Msg, Len);
		Info->SendBuf[Len] = '\0';

		Info->SendOverlappedEx.WsaBuf.len = Len;
		Info->SendOverlappedEx.WsaBuf.buf = Info->SendBuf;
		Info->SendOverlappedEx.Operation = IOOperation::SEND;

		int Ret = WSASend(Info->SocketClient,
			&(Info->SendOverlappedEx.WsaBuf),
			1,
			&RecvNumBytes,
			0,
			(LPWSAOVERLAPPED) & (Info->SendOverlappedEx),
			NULL);

		if(SOCKET_ERROR == Ret && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			printf("[에러] WSASend()함수 실패: %d\n", WSAGetLastError());
			return false;
		}
		return true;
	}

	void IOWorker()
	{
		ClientInfo* Info = NULL;
		BOOL Success = TRUE;
		DWORD IoSize = 0;
		LPOVERLAPPED Overlapped = NULL;

		while(IsWorkerRunning)
		{
			Success = GetQueuedCompletionStatus(IOCPHandle,
				&IoSize,					// 실제로 전송된 바이트
				(PULONG_PTR)&Info,			// Completion Key
				&Overlapped,				// Overlapped I/O 객체
				INFINITE);		// 대기할 시간

			if(TRUE == Success && 0 == IoSize && NULL == Overlapped)
			{
				IsWorkerRunning = false;
				continue;
			}
			if(NULL == Overlapped)
			{
				continue;
			}
			if(FALSE == Success || (0 == IoSize && TRUE == Success))
			{
				CloseSocket(Info);
				continue;
			}

			OverlappedEx* ov = (OverlappedEx*)Overlapped;
			if(IOOperation::RECV == ov->Operation)
			{
				OnReceive(Info->Index, IoSize, Info->RecvBuf);
				
				// 클라이언트에 메세지 echo
				SendMsg(Info, Info->RecvBuf, IoSize);
				BindRecv(Info);
			}
			else if(IOOperation::SEND == ov->Operation)
			{
				printf("[송신] bytes: %d, msg: %s\n", IoSize, Info->SendBuf);
			}
			else
			{
				printf("socket(%d)에서 예외상황\n", (int)Info->SocketClient);
			}
		}
	}

	void Acceptor()
	{
		SOCKADDR_IN ClientAddr;
		int AddrLen = sizeof(SOCKADDR_IN);

		while(IsAcceptorRunning)
		{
			ClientInfo* Info = GetEmptyClientInfo();
			if(NULL == Info)
			{
				printf("[에러] Client Full\n");
				return;
			}

			Info->SocketClient = accept(ListenSocket, (SOCKADDR*)&ClientAddr, &AddrLen);
			if(INVALID_SOCKET == Info->SocketClient)
			{
				continue;
			}

			bool Ret = bindIOCompletionPort(Info);
			if(false == Ret)
			{
				return;
			}

			Ret = BindRecv(Info);
			if(false == Ret)
			{
				return;
			}

			Info->Index = ClientCount;
			OnConnect(Info->Index);

			++ClientCount;
		}
	}

	void CloseSocket(ClientInfo* Info, bool Force = false)
	{
		struct linger Linger = { 0,0 };

		if(true == Force)
		{
			Linger.l_onoff = 1;
		}

		shutdown(Info->SocketClient, SD_BOTH);

		setsockopt(Info->SocketClient, SOL_SOCKET, SO_LINGER, (char*)&Linger, sizeof(Linger));

		closesocket(Info->SocketClient);

		Info->SocketClient = INVALID_SOCKET;

		OnClose(Info->Index);
		--ClientCount;
	}
	// 클라이언트 정보
	std::vector<ClientInfo> ClientInfos;

	// 클라이언트 접속을 받기 위한 리슨 소켓
	SOCKET ListenSocket = INVALID_SOCKET;

	// 접속되어있는 클라이언트 수
	int ClientCount = 0;

	// IO Worker 쓰레드
	std::vector<std::thread> IOWorkerThreads;

	// Accept 쓰레드
	std::thread AcceptorThread;

	// CompletionPort 객체 핸들
	HANDLE IOCPHandle = INVALID_HANDLE_VALUE;

	// 작업 쓰레드 동작 플래그
	bool IsWorkerRunning = true;

	// 접속 쓰레드 동작 플래그
	bool IsAcceptorRunning = true;

	// 소켓 버퍼
	char SocketBuffer[1024] = { 0, };

};