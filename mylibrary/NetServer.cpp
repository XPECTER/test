#include "NetServer.h"

CNetServer::CNetServer()
{
	_iAcceptTPS = 0;
	_iRecvPacketTPS = 0;
	_iSendPacketTPS = 0;
	_iAcceptCounter = 0;
	_iRecvPacketCounter = 0;
	_iSendPacketCounter = 0;
	_iClientCount = 0;

	_SystemErrorLogFile = NULL;

	_bWorking = false;
	_bStop = false;

	_hIOCP = NULL;
	_hAcceptThread = NULL;
	_hWorkerThread = NULL;
	_hMonitorThread = NULL;
	_iWorkerThreadNum = 0;
	_iMaxUser = 0;
	_iClientID = 0;

	_aSession = NULL;
	_listenSock = INVALID_SOCKET;
}

CNetServer::~CNetServer()
{

}


bool CNetServer::Start(WCHAR *szIP, int iPort, int iThreadNum, bool bNagleEnable, int iMaxUser)
{
	if (_bWorking)
		return false;

	_bWorking = true;

	WSADATA Data;
	if (0 != WSAStartup(MAKEWORD(2, 2), &Data))
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"WSAStartUp() Error. ErrorNo : %d", WSAGetLastError());
		return false;
	}

	_listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (INVALID_SOCKET == _listenSock)
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"socket() Error. ErrorNo : %d", WSAGetLastError());
		return false;
	}

	linger lingerOpt;
	lingerOpt.l_onoff = 1;
	lingerOpt.l_linger = 0;

	if (setsockopt(_listenSock, SOL_SOCKET, SO_LINGER, (char *)&lingerOpt, sizeof(lingerOpt)))
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"setsockopt() linger Error. ErrorNo : %d", WSAGetLastError());
	}

	if (true == bNagleEnable)
	{
		if (0 != setsockopt(_listenSock, IPPROTO_TCP, TCP_NODELAY, (char *)true, sizeof(bool)))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"setsockopt() nodelay Error. ErrorNo : %d", WSAGetLastError());
			return false;
		}
	}

	ZeroMemory(&_serverAddr, sizeof(SOCKADDR_IN));
	_serverAddr.sin_family = AF_INET;
	_serverAddr.sin_port = htons(iPort);
	InetPton(AF_INET, szIP, &_serverAddr.sin_addr.s_addr);

	if (0 != bind(_listenSock, (SOCKADDR *)&_serverAddr, sizeof(_serverAddr)))
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"bind() Error. ErrorNo : %d", WSAGetLastError());
		return false;
	}

	_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, _iWorkerThreadNum);
	if (NULL == _hIOCP)
	{
		//OnError(GetLastError(), 0, errstr_CREATE_IOCP);
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"CreateIoCompletionPort() Error. ErrorNo : %d", WSAGetLastError());
		return false;
	}

	Init_Session(iMaxUser);

	if (!Init_Thread(iThreadNum))
		return false;

	return true;
}

void CNetServer::Stop()
{
	return;
}

void CNetServer::Init_Session(int iMaxUser)
{
	this->_iMaxUser = iMaxUser;
	_aSession = new SESSION[iMaxUser];

	SESSION *pSession = NULL;

	for (int i = 0; i < iMaxUser; ++i)
	{
		pSession = &_aSession[i];
		pSession->IOReleaseCompare = (IO_RELEASE_COMPARE *)_aligned_malloc(sizeof(IO_RELEASE_COMPARE), 16);
		pSession->IOReleaseCompare->IOCount = 0;
		pSession->IOReleaseCompare->ReleaseFlag = 0;
	}

	for (int j = iMaxUser - 1; j >= 0; j--)
		_indexStack.Push(j);

	return;
}

bool CNetServer::Init_Thread(int _iThreadNum)
{
	_iWorkerThreadNum = max(dfMIN_THREAD_COUNT, _iThreadNum);
	_iWorkerThreadNum = min(dfMAX_THREAD_COUNT, _iThreadNum);
	_hWorkerThread = new HANDLE[_iWorkerThreadNum];

	// thread 생성 실패시 0반환하고 errno와 _doserror가 설정됨
	_hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThreadFunc, (void *)this, 0, NULL);
	if (NULL == _hAcceptThread)
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Create AcceptTh Error. ErrorNo : %d", WSAGetLastError());
		return FALSE;
	}

	for (int i = 0; i < _iWorkerThreadNum; ++i)
	{
		_hWorkerThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThreadFunc, (void *)this, 0, NULL);
		if (NULL == _hWorkerThread[i])
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Create WorkerTh Error. ErrorNo : %d", WSAGetLastError());
			return FALSE;;
		}
	}

	_hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThreadFunc, (void *)this, 0, NULL);
	if (NULL == _hMonitorThread)
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Create MonitorTh Error. ErrorNo : %d", WSAGetLastError());
		return FALSE;
	}

	return TRUE;
}

int CNetServer::GetClientCount(void) { return _iClientCount; }


unsigned _stdcall CNetServer::MonitorThreadFunc(void *lpParam)
{
	CNetServer *pServer = (CNetServer *)lpParam;

	while (true)
	{
		Sleep(998);
		if (pServer->_bStop)
			return 0;

		pServer->_iAcceptTPS		= pServer->_iAcceptCounter;
		pServer->_iRecvPacketTPS	= pServer->_iRecvPacketCounter;
		pServer->_iSendPacketTPS	= pServer->_iSendPacketCounter;

		pServer->_iAcceptCounter		= 0;
		pServer->_iRecvPacketCounter	= 0;
		pServer->_iSendPacketCounter	= 0;
	}

	return 0;
}

unsigned _stdcall CNetServer::AcceptThreadFunc(void *lpParam)
{
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	int Len = sizeof(SOCKADDR_IN);
	int iPort;
	WCHAR szIP[16] = { 0, };
	int index;
	CNetServer *pServer = (CNetServer *)lpParam;
	SESSION *pSession = nullptr;

	// SOMAXCONN은 7FFFFFFFF지만 200으로 되어있음.
	// 윈도우의 SYC어택 때문에 클라이언트가 connect를 오해할 수 있다.
	// TCP의 3방향 
	if (0 != listen(pServer->_listenSock, SOMAXCONN))
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"listen() Error. ErrorNo : %d", WSAGetLastError());
		return -1;
		// 서버 죽여야지
	}

	while (true)
	{
		clientSock = accept(pServer->_listenSock, (SOCKADDR *)&clientAddr, &Len);
		pServer->_iAcceptTotal++;

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

		pSession = &pServer->_aSession[index];
		pSession->ClientId = MAKECLIENTID(index, pServer->_iClientID);
		pSession->ClientSock = clientSock;
		pSession->ClientAddr = clientAddr;
		pSession->RecvQ.ClearBuffer();
		pSession->SendCount = 0;
		pSession->Sending = FALSE;

		if (NULL == CreateIoCompletionPort((HANDLE)pSession->ClientSock, pServer->_hIOCP, index, 0))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"CreateIoCompletionPort() Error. ErrorNo : %d", WSAGetLastError());
			closesocket(pSession->ClientSock);
		}

		InterlockedIncrement64(&pSession->IOReleaseCompare->IOCount);
		InterlockedExchange64(&pSession->IOReleaseCompare->ReleaseFlag, 0);
		pServer->OnClientJoin(pSession->ClientId);

		PROFILING_BEGIN(L"RecvPost");
		pServer->RecvPost(pSession, FALSE);
		PROFILING_END(L"RecvPost");

		pServer->_iAcceptCounter++;
		
		pServer->_iClientID++;
		InterlockedIncrement((long *)&pServer->_iClientCount);	// Worker Th도 접근 가능
	}

	return 0;
}

unsigned _stdcall CNetServer::WorkerThreadFunc(void *lpParam)
{
	CNetServer *pServer = (CNetServer *)lpParam;
	SESSION *pSession = nullptr;
	BOOL _bResult;
	DWORD transferredBytes;
	OVERLAPPED *tempOv = new OVERLAPPED;
	unsigned long index = NULL;
	__int64 iIOCount;
	CPacket *pPacket = NULL;
	int iResult;

	while (true)
	{
		index = 0;
		transferredBytes = 0;
		ZeroMemory(tempOv, sizeof(OVERLAPPED));

		_bResult = GetQueuedCompletionStatus(pServer->_hIOCP, &transferredBytes, (PULONG_PTR)&index, &tempOv, INFINITE);

		if (NULL == tempOv)
		{
			// index대신 session포인터를 넣어서 처리하자
			// if(NULL == pSession && 0 == transferredBytes)
			if (TRUE == _bResult && 0 == transferredBytes)
			{
				PostQueuedCompletionStatus(pServer->_hIOCP, 0, 0, NULL);
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
			pSession = &pServer->_aSession[index];

			if (index != EXTRACTCLIENTINDEX(pSession->ClientId))
				pServer->OnError(0, pSession->ClientId, L"Index Mismatch");

			if (0 == transferredBytes)
				pServer->ClientDisconnect(pSession->ClientId);
			else if (tempOv == &pSession->RecvOverlap)
			{
					if (!pSession->RecvQ.MoveWrite(transferredBytes))
					SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"RecvQ MoveWrite() Error.");

					PROFILING_BEGIN(L"PacketProc");
					pServer->PacketProc(pSession);
					PROFILING_END(L"PacketProc");

					PROFILING_BEGIN(L"RecvPost");
					pServer->RecvPost(pSession, TRUE);
					PROFILING_END(L"RecvPost");
			}
			else if (tempOv == &pSession->SendOverlap)
			{
				pServer->OnSend(pSession->ClientId, transferredBytes);

				for (int i = 0; i < pSession->SendCount; ++i)
				{
					iResult = pSession->SendQ.Dequeue(&pPacket);

					if (-1 == iResult)
						SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"[CLIENT_ID : %d]SendQ Deqeueue Error in WorkerThread", pSession->ClientId);

					pPacket->Free();
				}

				InterlockedExchange((long *)&pSession->Sending, FALSE);
				pServer->SendPost(pSession);
			}
			/*else
				pServer->ClientDisconnect(pSession->ClientId);*/
		}

		iIOCount = InterlockedDecrement64(&pSession->IOReleaseCompare->IOCount);

		if (0 == iIOCount)
			pServer->ClientRelease(pSession);

		Sleep(0);
	}
	return 0;
}

void CNetServer::SendPost(SESSION *pSession)
{
	int iWSASendResult;
	int iError;
	WSABUF wsabuf[200];
	int wsabufCount = 0;
	CPacket *buff = NULL;

	while (TRUE)
	{
		if (InterlockedCompareExchange((long *)&pSession->Sending, TRUE, FALSE))
			return;

		if (0 == pSession->SendQ.GetUseSize())
		{
			InterlockedExchange((long *)&pSession->Sending, FALSE);

			if (0 == pSession->SendQ.GetUseSize())
				return;
			else
				continue;
		}
		else
		{
			InterlockedIncrement64(&pSession->IOReleaseCompare->IOCount);
			for (int i = 0; i < 200; ++i)
			{
				if (i < pSession->SendQ.GetUseSize())
				{
					if (pSession->SendQ.Peek(&buff, i))
					{
						wsabuf[i].buf = (*buff).GetBuffPtr();
						wsabuf[i].len = (*buff).GetUseSize();
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
			pSession->SendCount = wsabufCount;

			iWSASendResult = WSASend(pSession->ClientSock, wsabuf, wsabufCount, NULL, lpFlag, &pSession->SendOverlap, NULL);

			if (SOCKET_ERROR == iWSASendResult)
			{
				iError = WSAGetLastError();
				if (WSA_IO_PENDING != iError)
				{
					if (WSAENOBUFS == iError)
						OnError(iError, pSession->ClientId, L"WSASend NoBuff.");

					ClientDisconnect(pSession->ClientId);

					if (0 == InterlockedDecrement64(&pSession->IOReleaseCompare->IOCount))
						ClientRelease(pSession);
				}
			}

			return;
		}
	}
}

void CNetServer::RecvPost(SESSION *pSession, bool incrementFlag)
{
	WSABUF wsabuf[2];
	int wsabufCount;
	int iWSARecvResult;
	int iError;

	if (TRUE == incrementFlag)
		InterlockedIncrement64(&pSession->IOReleaseCompare->IOCount);

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
		}
	}

	return;
}

bool CNetServer::PacketProc(SESSION *pSession)
{
	PACKET_HEADER header;

	while (0 < pSession->RecvQ.GetUseSize())
	{
		pSession->RecvQ.Peek((char *)&header, sizeof(PACKET_HEADER));

		if (CPacket::_packetCode != header.byCode)
		{
			ClientDisconnect(pSession);
			SYSLOG(L"PACKET", LOG::LEVEL_DEBUG, L"[CLIENT_ID : %d] Header Code Error", pSession->ClientId);
			break;
		}

		if (header.shLen > CPacket::dfDEFUALT_BUFF_SIZE)
		{
			ClientDisconnect(pSession);
			SYSLOG(L"PACKET", LOG::LEVEL_DEBUG, L"[CLIENT_ID : %d] Header Len Error", pSession->ClientId);
			break;
		}

		if (header.shLen + sizeof(PACKET_HEADER) > pSession->RecvQ.GetUseSize())
			break;

		CPacket *packet = CPacket::Alloc();
		if (-1 == pSession->RecvQ.Dequeue((char *)packet->GetBuffPtr(), header.shLen + sizeof(PACKET_HEADER)))
			SYSLOG(L"SESSION", LOG::LEVEL_ERROR, L"[CLIENT_ID : %d] RecvQ Dequeue error", EXTRACTCLIENTID(pSession->ClientId));
		else
			packet->MoveWrite(header.shLen);// + sizeof(PACKET_HEADER));

		if (!packet->Decode())
		{
			SYSLOG(L"PACKET", LOG::LEVEL_DEBUG, L"[CLIENT_ID : %d] CheckSum Error", pSession->ClientId);
			ClientDisconnect(pSession);
			packet->Free();
			return false;
		}

		try
		{
			OnRecv(pSession->ClientId, packet);
			InterlockedIncrement((long *)&_iRecvPacketCounter);
		}
		catch (CPacket::exception_PacketOut e)
		{
			SYSLOG(L"PACKET", LOG::LEVEL_ERROR, e.szStr);
			ClientDisconnect(pSession);
			packet->Free();
			return false;
		}

		packet->Free();
	}

	// return false경우 처리가 필요함.
	return true;
}

void CNetServer::SendPacket(CLIENT_ID _clientId, CPacket *_sendBuff)
{
	int iResult;
	SESSION *pSession = AcquireSessionLock(_clientId);

	if (nullptr == pSession)
		return;

	_sendBuff->IncrementRefCount();
	_sendBuff->Encode();
	iResult = pSession->SendQ.Enqueue(_sendBuff);

	if (-1 == iResult)
	{
		SYSLOG(L"QUEUE", LOG::LEVEL_DEBUG, L"[CLIENT_ID : %d] SendQ is full", _clientId);
		_sendBuff->Free();
		ClientDisconnect(_clientId);
	}

	SendPost(pSession);
	InterlockedIncrement((long *)&_iSendPacketCounter);

	ReleaseSessionLock(_clientId);
	return;
}


CNetServer::SESSION* CNetServer::AcquireSessionLock(CLIENT_ID clientId)
{
	SESSION *pSession = &_aSession[EXTRACTCLIENTINDEX(clientId)];

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

		if (1 == pSession->IOReleaseCompare->ReleaseFlag)
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

void CNetServer::ReleaseSessionLock(CLIENT_ID clientId)
{
	SESSION *pSession = &_aSession[EXTRACTCLIENTINDEX(clientId)];

	if (0 == InterlockedDecrement((long *)&pSession->IOReleaseCompare->IOCount))
		ClientRelease(pSession);

	return;
}

void CNetServer::ClientDisconnect(CLIENT_ID clientId)
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

void CNetServer::ClientDisconnect(SESSION *pSession)
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

void CNetServer::ClientRelease(SESSION *pSession)
{
	IO_RELEASE_COMPARE compare;
	compare.IOCount = 0;
	compare.ReleaseFlag = 0;

	if (!InterlockedCompareExchange128((LONG64 *)pSession->IOReleaseCompare, (LONG64)1, (LONG64)0, (LONG64 *)&compare))
		return;

	OnClientLeave(pSession->ClientId);

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
	_indexStack.Push(EXTRACTCLIENTINDEX(pSession->ClientId));
	InterlockedDecrement((long *)&this->_iClientCount);
	return;
}





