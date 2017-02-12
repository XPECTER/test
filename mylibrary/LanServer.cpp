#include "LanServer.h"

#define MAKECLIENTID(index, id) (((__int64)index << 48) | id)
#define EXTRACTINDEX(clientID) ((__int64)clientID >> 48)
#define EXTRACTCLIENTID(clientID) ((__int64)clientID & 0x00FFFFFF)

CLanServer::CLanServer()
{
	// PARAM
	_listenSock = INVALID_SOCKET;
	_iClientID = 0;
	_bWorking = false;
	_bStop = false;
	_iMaxUser = 0;
	_iClientCount = 0;
	_iTPS = 0;
	_timeStamp = time(NULL);
	aSession = NULL;

	// HANDLE
	hIOCP = NULL;
	hAcceptThread = NULL;
	hWorkerThread = NULL;
	hMonitorThread = NULL;
	//hSendThread = NULL;
	SystemErrorLogFile = NULL;

	// Monitor Param
	_iAcceptTPS = 0;
	_iAcceptCounter = 0;

	_iRecvPacketTPS = 0;
	_iRecvPacketCounter = 0;

	_iSendPacketTPS = 0;
	_iSendPacketCounter = 0;

	InitializeSRWLock(&srwLock);
}

CLanServer::~CLanServer()
{
	// What do i do?
	delete[] aSession;
}

bool CLanServer::Start(WCHAR *_ip, int _port, int _iThreadNum, bool _bNagleEnable, int _iMaxUser)
{
	if (_bWorking)
		return false;

	_bWorking = true;

	WSADATA Data;
	if (0 != WSAStartup(MAKEWORD(2, 2), &Data))
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"LanServer WSAStartUp Error. No : %d", WSAGetLastError());
		return false;
	}

	_listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (INVALID_SOCKET == _listenSock)
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"LanServer socket() Error. No : %d", WSAGetLastError());
		return false;
	}

	linger lingerOpt;
	lingerOpt.l_onoff = 1;
	lingerOpt.l_linger = 0;

	if (setsockopt(_listenSock, SOL_SOCKET, SO_LINGER, (char *)&lingerOpt, sizeof(lingerOpt)))
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"LanServer setsockopt() linger Error. No : %d", WSAGetLastError());
	}

	if (true == _bNagleEnable)
	{
		if (0 != setsockopt(_listenSock, IPPROTO_TCP, TCP_NODELAY, (char *)true, sizeof(bool)))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"LanServer setsockopt() nodelay Error. No : %d", WSAGetLastError());
			return false;
		}
	}

	ZeroMemory(&_serverAddr, sizeof(SOCKADDR_IN));
	_serverAddr.sin_family = AF_INET;
	_serverAddr.sin_port = htons(_port);
	InetPton(AF_INET, _ip, &_serverAddr.sin_addr.s_addr);

	if (0 != bind(_listenSock, (SOCKADDR *)&_serverAddr, sizeof(_serverAddr)))
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"LanServer bind() Error. No : %d", WSAGetLastError());
		return false;
	}

	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, _iWorkerThreadNum);
	if (NULL == hIOCP)
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"LanServer CreateIOCP() Error. No : %d", WSAGetLastError());
		return false;
	}
	
	Init_Session(_iMaxUser);

	if (!Init_Thread(_iThreadNum))
		return false;

	return true;
}

void CLanServer::Stop()
{
	if (_bWorking)
	{
		_bWorking = false;
		_bStop = true;

		for (int i = 0; i < _iMaxUser; ++i)
		{
			ClientDisconnect(&aSession[i]);
		}

		while (true)
		{
			if (0 != _iClientCount)
				Sleep(100);
			else
				break;
		}

		PostQueuedCompletionStatus(hIOCP, 0, NULL, NULL);
		closesocket(_listenSock);
		CloseHandle(hIOCP);

		HANDLE *hThread = new HANDLE[_iWorkerThreadNum + 1];
		hThread[0] = hAcceptThread;

		for (int i = 0; i < _iWorkerThreadNum; ++i)
			hThread[i + 1] = hWorkerThread[i];

		if (WAIT_OBJECT_0 == WaitForMultipleObjects(_iWorkerThreadNum + 1, hThread, TRUE, INFINITE))
			return;
	}
	else
		return;
}

int CLanServer::GetClientCount(void)
{
	return _iClientCount;
}

void CLanServer::Init_Session(int _iMaxUser)
{
	this->_iMaxUser = _iMaxUser;
	aSession = new SESSION[_iMaxUser];

	SESSION *pSession = NULL;

	for (int i = 0; i < _iMaxUser; ++i)
	{
		pSession = &aSession[i];
		pSession->IOReleaseCompare = (IO_RELEASE_COMPARE *)_aligned_malloc(sizeof(IO_RELEASE_COMPARE), 16);
		pSession->IOReleaseCompare->IOCount = 0;
		pSession->IOReleaseCompare->bReleaseFlag = 0;
	}

	for (int j = _iMaxUser - 1; j >= 0; j--)
		_indexStack.Push(j);

	return;
}

BOOL CLanServer::Init_Thread(int _iThreadNum)
{
	// thread 생성 실패시 0반환하고 errno와 _doserror가 설정됨
	hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThreadFunc, (void *)this, 0, NULL);
	if (NULL == hAcceptThread)
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"LanServer Create AcceptTh Error. No : %d", GetLastError());
		return FALSE;
	}

	_iWorkerThreadNum = max(dfMIN_THREAD_COUNT, _iThreadNum);
	_iWorkerThreadNum = min(dfMAX_THREAD_COUNT, _iThreadNum);
	hWorkerThread = new HANDLE[_iWorkerThreadNum];

	for (int i = 0; i < _iWorkerThreadNum; ++i)
	{
		hWorkerThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThreadFunc, (void *)this, 0, NULL);
		if (NULL == hWorkerThread[i])
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"LanServer Create WorkerTh Error. No : %d", GetLastError());
			return FALSE;;
		}
	}

	/*hSendThread = (HANDLE)_beginthreadex(NULL, 0, SendThreadFunc, (void *)this, 0, NULL);
	if (NULL == hSendThread)
	{
	OnError(GetLastError(), errstr_CREATE_THREAD);
	return false;
	}*/

	hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThreadFunc, (void *)this, 0, NULL);
	if (NULL == hMonitorThread)
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"LanServer Create MonitorTh Error. No : %d", GetLastError());
		return FALSE;
	}

	return TRUE;
}

unsigned _stdcall CLanServer::AcceptThreadFunc(void *lpParam)
{
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	int Len = sizeof(SOCKADDR_IN);
	int iPort;
	WCHAR szIP[16] = { 0, };
	int index;
	CLanServer *pServer = (CLanServer *)lpParam;
	SESSION *pSession = nullptr;

	// SOMAXCONN은 7FFFFFFFF지만 200으로 되어있음.
	// 윈도우의 SYC어택 때문에 클라이언트가 connect를 오해할 수 있다.
	// TCP의 3방향 
	if (0 != listen(pServer->_listenSock, SOMAXCONN))
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"LanServer listen() Error. No : %d", GetLastError());
		return -1;
		// 서버 죽여야지
	}

	while (true)
	{
		clientSock = accept(pServer->_listenSock, (SOCKADDR *)&clientAddr, &Len);

		if (INVALID_SOCKET == clientSock)
		{
			if (pServer->_bStop)
				return 0;

			continue;
		}

		InetNtop(AF_INET, &clientAddr.sin_addr, szIP, 16);
		iPort = ntohs(clientAddr.sin_port);

		// 접속 거부된 IP이거나 Port이면 접속 불가처리
		if (!pServer->OnConnectionRequest(szIP, iPort))
		{
			closesocket(clientSock);
			continue;
		}

		// Keep alive option
		tcp_keepalive keepAlive;
		keepAlive.onoff = 1;
		keepAlive.keepalivetime = 5000;
		keepAlive.keepaliveinterval = 2000;
		WSAIoctl(clientSock, SIO_KEEPALIVE_VALS, &keepAlive, sizeof(tcp_keepalive), 0, 0, 0, NULL, NULL);


		index = -1;
		PROFILING_BEGIN(L"SearchEmptySession");
		pServer->_indexStack.Pop(&index);
		PROFILING_END(L"SearchEmptySession");

		if (-1 == index)
		{
			// 배열이 다 차서 다른 무언가를 해줘야함.
			pServer->OnError(GetLastError(), -1, L"Session Array is full");

			// 밑에 로직 추가
			closesocket(clientSock);
			continue;
		}

		pSession = &pServer->aSession[index];
		pSession->_iIndex = index;
		pSession->ClientId = MAKECLIENTID(index, pServer->_iClientID);
		pSession->ClientSock = clientSock;
		pSession->ClientAddr = clientAddr;
		pSession->RecvQ.ClearBuffer();
		//pSession->SendQ.ClearBuffer();	// SendQ가 이제 StreamQueue가 아니라 Lock-Free Queue
		//pSession->IOCount = 0;			// 0으로 초기화하는 코드는 세션 초기화할떄만 있어야 함.
		pSession->_sendCount = 0;
		pSession->_bSending = FALSE;

		if (NULL == CreateIoCompletionPort((HANDLE)pSession->ClientSock, pServer->hIOCP, index, 0))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"LanServer CreateIOCP()_resist Error. No : %d", GetLastError());
			closesocket(pSession->ClientSock);
		}

		//InterlockedIncrement((long *)&pSession->IOCount);
		// 여기서 ReleaseFlag를 0으로 만들면 된다.
		InterlockedIncrement64(&pSession->IOReleaseCompare->IOCount);
		InterlockedExchange64(&pSession->IOReleaseCompare->bReleaseFlag, 0);
		pServer->OnClientJoin(pSession->ClientId);

		PROFILING_BEGIN(L"RecvPost");
		pServer->RecvPost(pSession, FALSE);
		PROFILING_END(L"RecvPost");

		pServer->_iAcceptCounter++;
		InterlockedIncrement((long *)&pServer->_iClientCount);	// Worker Th도 접근 가능
		pServer->_iClientID++;
	}

	_endthreadex(0);
	return 0;
}

unsigned _stdcall CLanServer::WorkerThreadFunc(void *lpParam)
{
	CLanServer *pServer = (CLanServer *)lpParam;
	SESSION *pSession = nullptr;
	BOOL _bResult;
	DWORD transferredBytes;
	OVERLAPPED *tempOv = new OVERLAPPED;
	unsigned long index = NULL;
	//int iIOCount;
	__int64 iIOCount;
	CPacket *buff = NULL;
	int iResult;
		
	while (true)
	{
		index = 0;
		transferredBytes = 0;
		ZeroMemory(tempOv, sizeof(OVERLAPPED));

		_bResult = GetQueuedCompletionStatus(pServer->hIOCP, &transferredBytes, (PULONG_PTR)&index, &tempOv, INFINITE);

		if (NULL == tempOv)
		{
			// index대신 session포인터를 넣어서 처리하자
			// if(NULL == pSession && 0 == transferredBytes)
			if (TRUE == _bResult && 0 == transferredBytes) 
			{
				PostQueuedCompletionStatus(pServer->hIOCP, 0, 0, NULL);
				return 0;
			}
			else
			{
				// IOCP에러.
				pServer->OnError(0, 0, L"Worker thread IOCP Error");
				return -1;
			}
		}
		else
		{
			pSession = &pServer->aSession[index];

			if (index != EXTRACTINDEX(pSession->ClientId))
				pServer->OnError(0, pSession->ClientId, L"Index Mismatch");

			if (tempOv == &pSession->RecvOverlap && TRUE == _bResult)
			{
				if (0 != transferredBytes)
				{
					if (!pSession->RecvQ.MoveWrite(transferredBytes))
					{
						SYSLOG(L"", LOG::LEVEL_ERROR, L"LanServer  Error. No : %d", GetLastError());
					}

					PROFILING_BEGIN(L"PacketProc");
					pServer->PacketProc(pSession);
					PROFILING_END(L"PacketProc");

					PROFILING_BEGIN(L"RecvPost");
					pServer->RecvPost(pSession, TRUE);
					PROFILING_END(L"RecvPost");
				}
				else
					pServer->ClientDisconnect(pSession->ClientId);
			}
			else if (tempOv == &pSession->SendOverlap && TRUE == _bResult)
			{
				pServer->OnSend(pSession->ClientId, transferredBytes);

				if (transferredBytes != (pSession->_sendCount * 10))
					pServer->OnError(0, pSession->ClientId, L"SendCount break");

				for (int i = 0; i < pSession->_sendCount; ++i)
				{
					iResult = pSession->SendQ.Dequeue(&buff);

					if (-1 == iResult)
						pServer->OnError(0, pSession->ClientId, L"SendQ Deqeueue Error in WorkerThread");
					
					buff->Free();
					// 여기서 buff의 refCount가 0이 아닐 수도 있다. 
					// SendPost후 바로 통지가 오고 free를 하면 refCount는 1이고
					// 그 후에 SendPacket에서 차감해서 거기서 0이 될 수도 있기 때문이다. 에러는 아니다.
				}
				
				InterlockedExchange((long *)&pSession->_bSending, FALSE);
				pServer->SendPost(pSession);
			}
			else
				pServer->ClientDisconnect(pSession->ClientId);
		}

		//iIOCount = InterlockedDecrement((long *)&pSession->IOCount);
		iIOCount = InterlockedDecrement64(&pSession->IOReleaseCompare->IOCount);
		
		if (0 == iIOCount)
			pServer->ClientRelease(pSession);

		Sleep(0);
	}
	return 0;
}


void CLanServer::SendPost(SESSION *pSession)
{
	int iWSASendResult;
	int iError;
	WSABUF wsabuf[200];
	int wsabufCount = 0;
	CPacket *buff = NULL;

	while (TRUE)
	{
		if (InterlockedCompareExchange((long *)&pSession->_bSending, TRUE, FALSE))
			return;

		if (0 == pSession->SendQ.GetUseSize())
		{
			InterlockedExchange((long *)&pSession->_bSending, FALSE);

			if (0 == pSession->SendQ.GetUseSize())
				return;
			else
				continue;
		}
		else
		{
			//InterlockedIncrement((long *)&pSession->IOCount);
			InterlockedIncrement64(&pSession->IOReleaseCompare->IOCount);
			for (int i = 0; i < 200; ++i)
			{
				if (i < pSession->SendQ.GetUseSize())
				{
					if (pSession->SendQ.Peek(&buff, i))
					{
						wsabuf[i].buf = (*buff).GetBuffPtr();
						wsabuf[i].len = (*buff).GetUseSize() + CPacket::dfHEADER_SIZE;

						if (10 != wsabuf[i].len)
						{
							OnError(0, pSession->ClientId, L"WSASend WSABUF.len error");
						}
						wsabufCount++;
					}
					else
						OnError(10000, pSession->ClientId, L"Peek Error\n");
				}
				else
					break;
			}

			DWORD lpFlag = 0;
			ZeroMemory(&pSession->SendOverlap, sizeof(OVERLAPPED));
			pSession->_sendCount = wsabufCount;

			iWSASendResult = WSASend(pSession->ClientSock, wsabuf, wsabufCount, NULL, lpFlag, &pSession->SendOverlap, NULL);

			if (SOCKET_ERROR == iWSASendResult)
			{
				iError = WSAGetLastError();
				if (WSA_IO_PENDING != iError)
				{
					if (WSAENOBUFS == iError)
						OnError(iError, pSession->ClientId, L"WSASend NoBuff.");

					//OnError(iError, errstr_SOCKET_WSASEND);
					ClientDisconnect(pSession->ClientId);

					if (0 == InterlockedDecrement64(&pSession->IOReleaseCompare->IOCount))
						ClientRelease(pSession);
					
					/*if (0 == InterlockedDecrement((long *)&pSession->IOCount))
						ClientRelease(pSession);*/
				}
				
			}

			return;
		}
	}
}

void CLanServer::RecvPost(SESSION *pSession, bool incrementFlag)
{
	WSABUF wsabuf[2];
	int wsabufCount;
	int iWSARecvResult;
	int iError;

	if (TRUE == incrementFlag)
	{
		//InterlockedIncrement((long *)&pSession->IOCount);
		InterlockedIncrement64(&pSession->IOReleaseCompare->IOCount);
	}
		

	wsabuf[0].len = pSession->RecvQ.GetNotBrokenPutsize();
	wsabuf[0].buf = pSession->RecvQ.GetWriteBufferPtr();
	wsabufCount = 1;

	int isBrokenLen = pSession->RecvQ.GetFreeSize() - pSession->RecvQ.GetNotBrokenPutsize();
	if (0 < isBrokenLen)
	{
		wsabuf[1].len = isBrokenLen;
		wsabuf[1].buf = pSession->RecvQ.GetBufferPtr();
		wsabufCount = 2;
	}

	DWORD lpFlag = 0;

	ZeroMemory(&pSession->RecvOverlap, sizeof(OVERLAPPED));
	iWSARecvResult = WSARecv(pSession->ClientSock, wsabuf, wsabufCount, NULL, &lpFlag, &pSession->RecvOverlap, NULL);

	if (SOCKET_ERROR == iWSARecvResult)
	{
		iError = WSAGetLastError();
		if (WSA_IO_PENDING != iError)
		{
			ClientDisconnect(pSession->ClientId);

			if (0 == InterlockedDecrement64(&pSession->IOReleaseCompare->IOCount))
				ClientRelease(pSession);

			/*if (0 == InterlockedDecrement((long *)&pSession->IOCount))
				ClientRelease(pSession);*/
		}
	}

	return;
}

int CLanServer::PacketProc(SESSION *pSession)
{
	CPacket *packet = CPacket::Alloc();
	st_HEADER header;
	unsigned int headerSize = sizeof(st_HEADER);

	while (0 < pSession->RecvQ.GetUseSize())
	{
		pSession->RecvQ.Peek((char *)&header, headerSize);

		if (header._wSize + headerSize > pSession->RecvQ.GetUseSize())
			break;

		// 컨텐츠에게 헤더를 뺀 정보만을 주기 위함
		packet->Clear();
		if (!pSession->RecvQ.RemoveData(headerSize))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"LanServer RecvQ RemoveData Error.");
		}
			
		if (-1 == pSession->RecvQ.Dequeue((char *)packet->GetPayloadPtr(), header._wSize))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"LanServer RecvQ Dequeue Error");
		}

		OnRecv(pSession->ClientId, packet);
		InterlockedIncrement((long *)&_iRecvPacketCounter);
	}

	// return false경우 처리가 필요함.
	packet->Free();
	return true;
}

void CLanServer::SendPacket(ClientID clientID, CPacket *pSendPacket)
{
	SESSION *pSession = AcquireSessionLock(clientID);

	if (nullptr == pSession)
		return;

	pSendPacket->IncrementRefCount();
	pSession->SendQ.Enqueue(pSendPacket);

	SendPost(pSession);
	InterlockedIncrement((long *)&_iSendPacketCounter);

	ReleaseSessionLock(clientID);
	return;
}


CLanServer::SESSION* CLanServer::AcquireSessionLock(ClientID clientId)
{
	SESSION *pSession = &aSession[EXTRACTINDEX(clientId)];

	if (clientId == pSession->ClientId)
	{
		if (1 == InterlockedIncrement((long *)&pSession->IOReleaseCompare->IOCount))
		{
			if (0 == InterlockedDecrement((long *)&pSession->IOReleaseCompare->IOCount))
			{
				ClientRelease(pSession);
				return nullptr;
			}
			else
				return nullptr;
		}

		if (1 == pSession->IOReleaseCompare->bReleaseFlag)
		{
			if (0 == InterlockedDecrement((long *)&pSession->IOReleaseCompare->IOCount))
			{
				ClientRelease(pSession);
				return nullptr;
			}
			else
				return nullptr;
		}

		if (clientId == pSession->ClientId)
			return pSession;
		else
		{
			if (0 == InterlockedDecrement((long *)&pSession->IOReleaseCompare->IOCount))
			{
				ClientRelease(pSession);
				return nullptr;
			}
			else
				return nullptr;
		}
	}
	else
		return nullptr;
}

void CLanServer::ReleaseSessionLock(ClientID clientId)
{
	SESSION *pSession = &aSession[EXTRACTINDEX(clientId)];

	if (0 == InterlockedDecrement((long *)&pSession->IOReleaseCompare->IOCount))
		ClientRelease(pSession);
	
	return;
}


void CLanServer::ClientDisconnect(ClientID clientId)
{
	SESSION *pSession = AcquireSessionLock(clientId);

	if (nullptr == pSession)
		return;
	else
	{
		shutdown(pSession->ClientSock, SD_BOTH);
		ReleaseSessionLock(clientId);
		return;
	}
}

void CLanServer::ClientDisconnect(SESSION *pSession)
{
	SESSION *_pSession = AcquireSessionLock(pSession->ClientId);

	if (nullptr == _pSession)
		return;
	else
	{
		shutdown(_pSession->ClientSock, SD_BOTH);
		ReleaseSessionLock(_pSession->ClientId);
		return;
	}
}

void CLanServer::ClientRelease(SESSION *pSession)
{
	IO_RELEASE_COMPARE compare;
	compare.IOCount = 0;
	compare.bReleaseFlag = 0;

	if (!InterlockedCompareExchange128((LONG64 *)pSession->IOReleaseCompare, (LONG64)1, (LONG64)0, (LONG64 *)&compare))
		return;

	CPacket *buff = NULL;
	while (true)
	{
		if (0 < pSession->SendQ.GetUseSize())
		{
			pSession->SendQ.Dequeue(&buff);
			buff->Free();
		}
		else
			break;
	}

	if (0 < pSession->SendQ.GetUseSize())
	{
		OnError(0, pSession->ClientId, L"Data remain in SendQ");
	}

	closesocket(pSession->ClientSock);
	_indexStack.Push(EXTRACTINDEX(pSession->ClientId));
	InterlockedDecrement((long *)&this->_iClientCount);
	return;
}

unsigned _stdcall CLanServer::MonitorThreadFunc(void *lpParam)
{
	CLanServer *pServer = (CLanServer *)lpParam;

	while (true)
	{
		Sleep(999);
		if (pServer->_bStop)
			return 0;

		pServer->_iAcceptTPS = pServer->_iAcceptCounter;
		pServer->_iRecvPacketTPS = pServer->_iRecvPacketCounter;
		pServer->_iSendPacketTPS = pServer->_iSendPacketCounter;

		pServer->_iAcceptCounter = 0;
		pServer->_iRecvPacketCounter = 0;
		pServer->_iSendPacketCounter = 0;
	}
}