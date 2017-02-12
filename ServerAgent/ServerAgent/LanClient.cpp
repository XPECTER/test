#include "LanClient.h"

CLanClient::CLanClient()
{
	WSADATA wsa;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsa))
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"WSAStartUp Error");

	this->_ClientSock = INVALID_SOCKET;
	this->_iClientBindPort = 0;
	this->_iServerPort = 0;
	this->_bSending = FALSE;
	this->_iSendCount = 0;
	this->_IOCompare = (IO_RELEASE_COMPARE *)_aligned_malloc(sizeof(IO_RELEASE_COMPARE), 16);
	this->_IOCompare->IOCount = 0;
	this->_IOCompare->ReleaseFlag = 0;
	this->_hIOCP = INVALID_HANDLE_VALUE;

	wmemset(this->_szServerGroupName, 0, 32);

	for (int i = 0; i < dfWORKER_THREAD_NUM; ++i)
	{
		this->_hWorkerThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, (void *)this, NULL, NULL);
		if (0 == this->_hWorkerThread[i])
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Create thread failed");
		}
	}

	if (NULL == (this->_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, dfWORKER_THREAD_NUM)))
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"create IOCP error. ErrorNo : %d", GetLastError());
	}
}

CLanClient::~CLanClient()
{

}

//bool CLanClient::Connect(wchar_t *szClientBindIP, wchar_t *szServerIP, int iServerPort, bool bEnableNagle)
//{
//
//}

bool CLanClient::Connect(void)
{
	if (true == this->_bConnected)
		return true;

	if (!this->SetNetworkConfig())
		return false;

	if (INVALID_SOCKET == this->_ClientSock)
	{
		this->_ClientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == this->_ClientSock)
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"create socket error");
			return false;
		}

		this->_ServerAddr.sin_family = AF_INET;
		this->_ServerAddr.sin_port = htons(this->_iClientBindPort);
		InetPton(AF_INET, this->_szClientBindIP, &this->_ServerAddr.sin_addr);

		if (SOCKET_ERROR == bind(this->_ClientSock, (SOCKADDR *)&this->_ServerAddr, 16))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"client addr bind failed. ErrorNo : %d", WSAGetLastError());
			return false;
		}

		this->_ServerAddr.sin_port = htons(this->_iServerPort);
		InetPton(AF_INET, this->_szServerIP, &this->_ServerAddr.sin_addr);
	}

	if (SOCKET_ERROR == connect(this->_ClientSock, (SOCKADDR *)&this->_ServerAddr, 16))
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_WARNING, L"connect to server failed. ErrorNo : %d", WSAGetLastError());
		return false;
	}

	if (!CreateIoCompletionPort((HANDLE)this->_ClientSock, this->_hIOCP, NULL, dfWORKER_THREAD_NUM))
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"regist iocp error.", GetLastError());
		return false;
	}
	

	this->_bConnected = true;
	InterlockedIncrement64(&this->_IOCompare->IOCount);
	InterlockedExchange64(&this->_IOCompare->ReleaseFlag, 0);
	OnEnterJoinServer();

	RecvPost(FALSE);
	return true;
}

bool CLanClient::SetNetworkConfig(void)
{
	CConfigParser parser;
	wchar_t szBlock[32] = { 0, };
	wchar_t szKey[32] = { 0, };

	if(!parser.OpenConfigFile(L"AgentConfig.ini"))
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Config file open failed");
		return false;
	}

	wsprintf(szBlock, L"NETWORK");
	if (!parser.MoveConfigBlock(szBlock))
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s BLOCK in Config file", szBlock);
		return false;
	}
	else
	{
		wsprintf(szKey, L"SERVER_GROUP_NAME");
		if (!parser.GetValue(szKey, this->_szServerGroupName, 32))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
			return false;
		}

		wsprintf(szKey, L"LAN_BIND_IP");
		if (!parser.GetValue(szKey, this->_szClientBindIP, 16))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
			return false;
		}

		wsprintf(szKey, L"LAN_BIND_PORT");
		if (!parser.GetValue(szKey, &this->_iClientBindPort))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
			return false;
		}

		wsprintf(szKey, L"MONITORING_SERVER_IP");
		if (!parser.GetValue(szKey, this->_szServerIP, 16))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
			return false;
		}

		wsprintf(szKey, L"MONITORING_SERVER_PORT");
		if (!parser.GetValue(szKey, &this->_iServerPort))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
			return false;
		}

		wsprintf(szKey, L"NAGLE_OPTION");
		if (!parser.GetValue(szKey, &this->_bEnableNagle))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key. Set default value false.", szKey);
			this->_bEnableNagle = false;
		}
	}

	return true;
}

unsigned __stdcall CLanClient::WorkerThread(void *lpParam)
{
	((CLanClient*)lpParam)->WorkerThread_update();
	return 0;
}

bool CLanClient::WorkerThread_update(void)
{
	DWORD transferredBytes;
	BOOL bResult;
	OVERLAPPED *overlapped = new OVERLAPPED;
	CPacket *pPacket = nullptr;
	SOCKET sock;
	__int64 iIOCount;

	while (true)
	{
		transferredBytes = 0;
		ZeroMemory(overlapped, sizeof(OVERLAPPED));

		bResult = GetQueuedCompletionStatus(this->_hIOCP, &transferredBytes, &sock, &overlapped, INFINITE);

		if (NULL == overlapped)
		{
			// index대신 session포인터를 넣어서 처리하자
			// if(NULL == pSession && 0 == transferredBytes)
			if (TRUE == bResult && 0 == transferredBytes)
			{
				PostQueuedCompletionStatus(this->_hIOCP, 0, 0, NULL);
				return false;
			}
			else
			{
				// IOCP에러.
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"IOCP error. ErrorNo : %d", GetLastError());
			}
		}
		else
		{
			if (0 == transferredBytes)
				Disconnect();
			else if (overlapped == &this->_RecvOverlap)
			{
				if (!this->_RecvQ.MoveWrite(transferredBytes))
				{
					SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"RecvQ movewrite error");
				}

				PROFILING_BEGIN(L"PacketProc");
				this->PacketProc();
				PROFILING_END(L"PacketProc");

				PROFILING_BEGIN(L"RecvPost");
				this->RecvPost(TRUE);
				PROFILING_END(L"RecvPost");
			}
			else if (overlapped == &this->_SendOverlap)
			{
				this->OnSend(transferredBytes);

				for (int i = 0; i < this->_iSendCount; ++i)
				{
					if (false == this->_SendQ.Dequeue(&pPacket))
						SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"SendQ Deqeueue Error in WorkerThread");

					pPacket->Free();
				}

				InterlockedExchange((long *)&this->_bSending, FALSE);
				this->SendPost();
			}
			/*else
			pServer->ClientDisconnect(pSession->ClientId);*/
		}

		iIOCount = InterlockedDecrement64(&this->_IOCompare->IOCount);

		if (0 == iIOCount)
			this->SessionRelease();

		Sleep(0);
	}
	return 0;
}

void CLanClient::SendPacket(CPacket *pPacket)
{
	// 내부 용도이므로 Encode는 필요 없다.
	pPacket->IncrementRefCount();
	_SendQ.Enqueue(pPacket);

	SendPost();
	return;
}

void CLanClient::SendPost(void)
{
	int iWSASendResult;
	int iError;
	WSABUF wsabuf[dfWSABUF_MAX];
	int wsabufCount = 0;
	CPacket *buff = NULL;

	while (TRUE)
	{
		if (InterlockedCompareExchange((long *)&this->_bSending, TRUE, FALSE))
			return;

		if (0 == this->_SendQ.GetUseSize())
		{
			InterlockedExchange((long *)&this->_bSending, FALSE);

			if (0 == this->_SendQ.GetUseSize())
				return;
			else
				continue;
		}
		else
		{
			InterlockedIncrement64(&this->_IOCompare->IOCount);
			for (int i = 0; i < 200; ++i)
			{
				if (i < this->_SendQ.GetUseSize())
				{
					if (this->_SendQ.Peek(&buff, i))
					{
						wsabuf[i].buf = (*buff).GetBuffPtr();
						wsabuf[i].len = (*buff).GetUseSize();
						wsabufCount++;
					}
					else
					{
						SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"_SendQ Peek Error");
					}
				}
				else
					break;
			}

			DWORD lpFlag = 0;
			ZeroMemory(&this->_SendOverlap, sizeof(OVERLAPPED));
			this->_iSendCount = wsabufCount;

			iWSASendResult = WSASend(this->_ClientSock, wsabuf, wsabufCount, NULL, lpFlag, &this->_SendOverlap, NULL);

			if (SOCKET_ERROR == iWSASendResult)
			{
				iError = WSAGetLastError();
				if (WSA_IO_PENDING != iError)
				{
					if (WSAENOBUFS == iError)
						SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"WSAENOBUFS.");

					Disconnect();

					if (0 == InterlockedDecrement64(&this->_IOCompare->IOCount))
						SessionRelease();
				}
			}

			return;
		}
	}
}

void CLanClient::RecvPost(bool bIncrementFlag)
{
	WSABUF wsabuf[2];
	int wsabufCount;
	int iWSARecvResult;
	int iError;

	if (TRUE == bIncrementFlag)
		InterlockedIncrement64(&this->_IOCompare->IOCount);

	wsabuf[0].len = this->_RecvQ.GetNotBrokenPutsize();
	wsabuf[0].buf = this->_RecvQ.GetWriteBufferPtr();
	wsabufCount = 1;

	int isBrokenLen = this->_RecvQ.GetFreeSize() - this->_RecvQ.GetNotBrokenPutsize();
	if (0 < isBrokenLen)
	{
		wsabuf[1].len = isBrokenLen;
		wsabuf[1].buf = this->_RecvQ.GetBufferPtr();
		wsabufCount = 2;
	}

	DWORD lpFlag = 0;

	ZeroMemory(&this->_RecvOverlap, sizeof(OVERLAPPED));
	iWSARecvResult = WSARecv(this->_ClientSock, wsabuf, wsabufCount, NULL, &lpFlag, &this->_RecvOverlap, NULL);

	if (SOCKET_ERROR == iWSARecvResult)
	{
		iError = WSAGetLastError();
		if (WSA_IO_PENDING != iError)
		{
			if (WSAENOBUFS == iError)
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"WSAENOBUFS.");

			Disconnect();

			if (0 == InterlockedDecrement64(&this->_IOCompare->IOCount))
				SessionRelease();
		}
	}

	return;
}

void CLanClient::PacketProc(void)
{
	CPacket *pPacket = CPacket::Alloc();
	HEADER header;
	unsigned int headerSize = sizeof(HEADER);

	while (0 < this->_RecvQ.GetUseSize())
	{
		this->_RecvQ.Peek((char *)&header, headerSize);

		if (header.wSize + headerSize > this->_RecvQ.GetUseSize())
			break;

		// 컨텐츠에게 헤더를 뺀 정보만을 주기 위함
		pPacket->Clear();
		if (!this->_RecvQ.RemoveData(headerSize))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"RecvQ removedata error");
			this->Disconnect();
			break;
		}

		if (-1 == this->_RecvQ.Dequeue((char *)pPacket->GetPayloadPtr(), header.wSize))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"RecvQ dequeue error");
			this->Disconnect(); 
			break;
		}

		OnRecv(pPacket);
	}

	// return false경우 처리가 필요함.
	pPacket->Free();
	return;
}

void CLanClient::Disconnect(void)
{
	shutdown(this->_ClientSock, SD_BOTH);
	return;
}

void CLanClient::SessionRelease(void)
{
	CPacket *pPacket = nullptr;
	
	IO_RELEASE_COMPARE compare;
	compare.IOCount = 0;
	compare.ReleaseFlag = 0;

	if (!InterlockedCompareExchange128((__int64 *)this->_IOCompare, (__int64)1, (__int64)0, (__int64 *)&compare))
		return;

	this->_RecvQ.ClearBuffer();
	
	while (this->_SendQ.GetUseSize() > 0)
	{
		_SendQ.Dequeue(&pPacket);
		pPacket->Free();
	}

	closesocket(this->_ClientSock);
	OnLeaveServer();
	this->_ClientSock = INVALID_SOCKET;
	this->_bConnected = false;
	return;
}