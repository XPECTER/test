#include "stdafx.h"
#include "ChatServer.h"
#include "LanClient_Chat.h"

CChatServer::CChatServer()
{
	_Monitor_UpdateTPS_Counter = 0;
	_Monitor_UpdateTPS = 0;

	_hUpdateEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	_hUpdateThread = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, this, 0, 0);
	_hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorTPS_Thread, this, 0, 0);

	this->_lanclient_Chat = new CLanClient_Chat(this);
}

CChatServer::~CChatServer()
{

}

unsigned __stdcall CChatServer::UpdateThread(void *lpParam)
{
	//DWORD dwResult = 0;
	CChatServer *pServer = (CChatServer *)lpParam;

	while (true)
	{
		if (0 == pServer->_UpdateMessageQueue.GetUseSize())
		{
			Sleep(3);
		}
		else
		{
			pServer->UpdateThread_update();
		}
	}

	return 0;
}

unsigned __stdcall CChatServer::MonitorTPS_Thread(void *lpParam)
{
	CChatServer *pServer = (CChatServer *)lpParam;
	return pServer->MonitorTPS_Thread_update();
}

bool CChatServer::MonitorTPS_Thread_update(void)
{
	__int64 updateTick = time(NULL);
	__int64 currTick = 0;

	while (true)
	{
		currTick = time(NULL);

		if (currTick - updateTick > 1000)
		{
			this->_Monitor_UpdateTPS = this->_Monitor_UpdateTPS_Counter;
			this->_Monitor_UpdateTPS_Counter = 0;

			this->EnqueueMessage(dfUPDATE_MESSAGE_HEARTBEAT, 0, NULL);
			updateTick = time(NULL);
		}
		else
			Sleep(50);
	}

	return true;
}

bool CChatServer::UpdateThread_update(void)
{
	st_UPDATE_MESSAGE *msg = nullptr;
	DWORD dwResult = 0;

	while (0 != _UpdateMessageQueue.GetUseSize())
	{
		_UpdateMessageQueue.Dequeue(&msg);

		switch (msg->iMsgType)
		{
			case dfUPDATE_MESSAGE_JOIN:
			{
				JoinPlayer(msg->ClientId, nullptr);
				break;
			}
			case dfUPDATE_MESSAGE_LEAVE:
			{
				LeavePlayer(msg->ClientId);
				break;
			}
			case dfUPDATE_MESSAGE_PACKET:
			{
				if (!PacketProc(msg))
					ClientDisconnect(msg->ClientId);

				msg->pPacket->Free();
				break;
			}
			case dfUPDATE_MESSAGE_HEARTBEAT:
			{
				SendPacket_HeartBeat();
				break;
			}

			default:
				break;
		}
	
		_Monitor_UpdateTPS_Counter++;
		_MemoryPool_UpdateMsg.Free(msg);
	}

	return true;
}

bool CChatServer::Start(WCHAR *szIP, int iPort, int iThreadNum, bool bNagleEnable, int iMaxUser)
{
	if (this->_lanclient_Chat->Connect())

	CNetServer::Start(szIP, iPort, iThreadNum, bNagleEnable, iMaxUser);
}

bool CChatServer::OnConnectionRequest(WCHAR *szClientIP, int iPort)
{
	return true;
}

void CChatServer::OnClientJoin(CLIENT_ID clientID)
{
	EnqueueMessage(dfUPDATE_MESSAGE_JOIN, clientID, NULL);
	return;
}

void CChatServer::OnClientLeave(CLIENT_ID clientID)
{
	EnqueueMessage(dfUPDATE_MESSAGE_LEAVE, clientID, NULL);
	return;
}

bool CChatServer::EnqueueMessage(int iMsgType, CLIENT_ID clientID, CPacket *pPacket, LPVOID Debug)
{
	auto msg = _MemoryPool_UpdateMsg.Alloc();
	msg->pPacket = nullptr;

	if (nullptr != pPacket)
		pPacket->IncrementRefCount();

	msg->iMsgType = iMsgType;
	msg->ClientId = clientID;
	msg->Debug = Debug;
	msg->pPacket = pPacket;

	_UpdateMessageQueue.Enqueue(msg);
	//SetEvent(_hUpdateEvent);
	return true;
}

void CChatServer::OnRecv(CLIENT_ID clientID, CPacket *pPacket)
{
	EnqueueMessage(dfUPDATE_MESSAGE_PACKET, clientID, pPacket);
	return;
}

void CChatServer::OnSend(CLIENT_ID clientId, int sendsize)
{
	return;
}

void CChatServer::OnError(int errorNo, CLIENT_ID clientId, WCHAR *errStr)
{
	AcquireSRWLockExclusive(&_SrwLock);
	_wfopen_s(&_SystemErrorLogFile, L"Network_System_Error_Log.txt", L"a, ccs=UNICODE");
	fwprintf_s(_SystemErrorLogFile, L"[ERROR NO : %d, CLIENTID : %d]%s\n", errorNo, EXTRACTCLIENTID(clientId), errStr);
	fclose(_SystemErrorLogFile);
	ReleaseSRWLockExclusive(&_SrwLock);
	return;
}

void CChatServer::OnWorkerThreadBegin()
{
	return;
}

void CChatServer::OnWorkerThreadEnd()
{
	return;
}

st_PLAYER* CChatServer::FindPlayer(CLIENT_ID clientID)
{
	std::map<CLIENT_ID, st_PLAYER*>::iterator iter;
	iter = _PlayerMap.find(clientID);
	
	if (iter == _PlayerMap.end())
		return nullptr;
	else
		return iter->second;
}


bool CChatServer::JoinPlayer(CLIENT_ID clientID, LPVOID Debug)
{
	st_PLAYER *pPlayer = _MemoryPool_Player.Alloc();
	pPlayer->ClientID = clientID;
	pPlayer->Sector.shSectorX = dfSECTOR_X_DEFAULT;
	pPlayer->Sector.shSectorY = dfSECTOR_Y_DEFAULT;
	pPlayer->bDisconnectChatRecv = false;
	_time64(&pPlayer->LastRecvPacket);

	_PlayerMap.insert(std::pair<CLIENT_ID, st_PLAYER*>(clientID, pPlayer));
	return true;
}


bool CChatServer::LeavePlayer(CLIENT_ID clientID)
{
	auto iter = _PlayerMap.find(clientID);

	if (iter != _PlayerMap.end())
	{
		st_PLAYER *pPlayer = iter->second;
		LeaveSector(clientID, pPlayer->Sector.shSectorX, pPlayer->Sector.shSectorY);
		_PlayerMap.erase(iter);
		_MemoryPool_Player.Free(pPlayer);
		return true;
	}
	else
		return false;
}


bool CChatServer::PacketProc(st_UPDATE_MESSAGE *msg)
{
	st_PLAYER *pPlayer = nullptr;
	pPlayer = FindPlayer(msg->ClientId);
	bool bFailed = true;
	WORD type = 0;

	if (nullptr == pPlayer)
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"[CLIENT_ID : %d] Can not find player", msg->ClientId);
		return false;
	}
	else
	{
		try
		{
			*(msg->pPacket) >> type;

			switch (type)
			{
			case en_PACKET_CS_CHAT_REQ_LOGIN:
			{
				bFailed = PacketProc_LoginRequest(pPlayer, msg->pPacket);
				break;
			}
			case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
			{
				bFailed = PacketProc_MoveSector(pPlayer, msg->pPacket);
				break;
			}
			case en_PACKET_CS_CHAT_REQ_MESSAGE:
			{
				bFailed = PacketProc_ChatMessage(pPlayer, msg->pPacket);
				break;
			}
			case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
			{
				bFailed = PacketProc_Heartbeat(pPlayer, msg->pPacket);
				break;
			}
			default:
			{
				// 패킷 버려야 함. 해당 유저는 return false해서 밖에서 끊음
				break;
			}
			}
		}
		catch (CPacket::exception_PacketOut eOut)
		{
			SYSLOG(L"PACKET", LOG::LEVEL_DEBUG, eOut.szStr);
			bFailed = false;
		}
	}
	
	return bFailed;
}

bool CChatServer::PacketProc_LoginRequest(st_PLAYER *pPlayer, CPacket *pPacket)
{
	/*
	AccountNo, ID, NickName, SessionKey를 세팅한다. 응답처리도 여기서 보낸다.
	로그인이 실패할 경우는 언제인가??
	*/
	_time64(&pPlayer->LastRecvPacket);

	__int64 AccountNo = 0;
	*pPacket >> AccountNo;

	if (0 > AccountNo)
	{
		SYSLOG(L"LOGIN", LOG::LEVEL_DEBUG, L"LoginRequest AccountNo Error : %d", AccountNo);
		return false;
	}
		
	pPlayer->AccountNo = AccountNo;
	pPacket->Dequeue((char*)pPlayer->szID, sizeof(WCHAR) * dfID_MAX_LEN);
	pPacket->Dequeue((char*)pPlayer->szNick, sizeof(WCHAR) * dfNICK_MAX_LEN);
	pPacket->Dequeue((char*)pPlayer->SessionKey, sizeof(char) * dfSESSION_KEY_BYTE_LEN);

	CPacket *pResPacket = MakePacket_LoginResponse(pPlayer, 1);
	SendPacket_Unicast(pPlayer->ClientID, pResPacket);
	pResPacket->Free();
	return true;
}

bool CChatServer::PacketProc_MoveSector(st_PLAYER *pPlayer, CPacket *pPacket)
{
	/* 
	처음 로그인 하면 섹터는 (-1, -1)상태이며 최초 1회 이 메시지를 받아야 
	플레이어의 섹터를 세팅함 
	*/

	__int64 AccountNo = 0;
	WORD wX = 0;
	WORD wY = 0;

	*pPacket >> AccountNo;
	if (AccountNo != pPlayer->AccountNo)
	{
		SYSLOG(L"SECTOR", LOG::LEVEL_DEBUG, L"[AccountNo : %d] Player Mismatch", AccountNo);
		return false;
	}

	*pPacket >> wX;
	*pPacket >> wY;

	CLIENT_ID clientID = pPlayer->ClientID;

	_time64(&pPlayer->LastRecvPacket);
	
	if (LeaveSector(clientID, pPlayer->Sector.shSectorX, pPlayer->Sector.shSectorY))
	{
		if (EnterSector(clientID, wX, wY))
		{
			CPacket *pResPacket = MakePacket_MoveSector(pPlayer);
			SendPacket_Unicast(clientID, pResPacket);
			pResPacket->Free();
			return true;
		}
		else
			return false;
	}
	else
		return false;
}

bool CChatServer::PacketProc_ChatMessage(st_PLAYER *pPlayer, CPacket *pPacket)
{
	__int64 AccountNo;
	*pPacket >> AccountNo;
	
	if (AccountNo != pPlayer->AccountNo)
	{
		SYSLOG(L"SECTOR", LOG::LEVEL_DEBUG, L"[AccountNo : %d] Player Mismatch", AccountNo);
		return false;
	}

	WORD wMessageLen;
	*pPacket >> wMessageLen;
	
	CPacket *pResPacket = MakePacket_ChatMessage(pPlayer, (wchar_t *)pPacket->GetCurrPtr(), wMessageLen);
	SendPacket_Around(pPlayer->ClientID, pResPacket, true);
	pResPacket->Free();
	return true;
}

bool CChatServer::PacketProc_Heartbeat(st_PLAYER *pPlayer, CPacket *pPacket)
{
	/*
	유저의 네트워크 시간 갱신만 하고 버림
	*/
	_time64(&pPlayer->LastRecvPacket);
	return true;
}

CPacket* CChatServer::MakePacket_LoginResponse(st_PLAYER *pPlayer, BYTE Status)
{
	CPacket *pPacket = CPacket::Alloc();
	*pPacket << (WORD)en_PACKET_CS_CHAT_RES_LOGIN;
	*pPacket << Status;
	*pPacket << pPlayer->AccountNo;
	return pPacket;
}

CPacket* CChatServer::MakePacket_MoveSector(st_PLAYER *pPlayer)
{
	CPacket *pPacket = CPacket::Alloc();
	*pPacket << (WORD)en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
	*pPacket << pPlayer->AccountNo;
	*pPacket << pPlayer->Sector.shSectorX;
	*pPacket << pPlayer->Sector.shSectorY;
	return pPacket;
}

CPacket* CChatServer::MakePacket_ChatMessage(st_PLAYER *pPlayer, WCHAR *szMessage, WORD iMessageSize)
{
	CPacket *pPacket = CPacket::Alloc();
	*pPacket << (WORD)en_PACKET_CS_CHAT_RES_MESSAGE;
	*pPacket << pPlayer->AccountNo;
	pPacket->Enqueue((char *)pPlayer->szID, sizeof(WCHAR) * dfID_MAX_LEN);
	pPacket->Enqueue((char *)pPlayer->szNick, sizeof(WCHAR) * dfNICK_MAX_LEN);
	*pPacket << iMessageSize;
	pPacket->Enqueue((char *)szMessage, iMessageSize);
	return pPacket;
}

CPacket* CChatServer::MakePacket_HeartBeat(void)
{
	CPacket *pPacket = CPacket::Alloc();
	*pPacket << (WORD)en_PACKET_SS_HEARTBEAT;
	*pPacket << (BYTE)dfTHREAD_TYPE_WORKER;
	return pPacket;
}

bool CChatServer::EnterSector(CLIENT_ID clientID, short iSectorX, short iSectorY)
{
	if (true != ValidSector(iSectorX, iSectorY))
	{
		SYSLOG(L"SECTOR", LOG::LEVEL_DEBUG, L"[CLIENT_ID : %d] EnterSector Fail", EXTRACTCLIENTID(clientID));
		return false;
	}

	st_PLAYER *pPlayer = FindPlayer(clientID);
	pPlayer->Sector.shSectorX = iSectorX;
	pPlayer->Sector.shSectorY = iSectorY;
	_Sector[iSectorY][iSectorX].push_back(clientID);
	return true;
}

bool CChatServer::LeaveSector(CLIENT_ID clientID, short iSectorX, short iSectorY)
{
	if (dfSECTOR_X_DEFAULT != iSectorX || dfSECTOR_Y_DEFAULT != iSectorY)
	{
		if (true != ValidSector(iSectorX, iSectorY))
		{
			SYSLOG(L"SECTOR", LOG::LEVEL_DEBUG, L"[CLIENT_ID : %d] LeaveSector Fail", EXTRACTCLIENTID(clientID));
			return false;
		}
	}
	else
		return true;

	std::list<CLIENT_ID> *sector = &_Sector[iSectorY][iSectorX];
	auto iter = sector->begin();
	st_PLAYER *pPlayer = nullptr;

	while (iter != sector->end())
	{
		if ((*iter) == clientID)
		{
			sector->erase(iter++);
			pPlayer = FindPlayer(clientID);
			pPlayer->Sector.shSectorX = dfSECTOR_X_DEFAULT;
			pPlayer->Sector.shSectorY = dfSECTOR_Y_DEFAULT;
			return true;
		}
		else
			iter++;
	}

	return false;
}

bool CChatServer::ValidSector(short iSectorX, short iSectorY)
{
	if (0 > iSectorX || dfSECTOR_X_MAX - 1 < iSectorX || 0 > iSectorY || dfSECTOR_Y_MAX - 1 < iSectorY)
		return false;
	else
		return true;
}


void CChatServer::SendPacket_HeartBeat(void)
{
	CPacket *pPacket = MakePacket_HeartBeat();
}

void CChatServer::SendPacket_SectorOne(short iSectorX, short iSectorY, CPacket *pPacket, CLIENT_ID ExceptClientID)
{ 
	std::list<CLIENT_ID> *sector = &_Sector[iSectorY][iSectorX];
	auto iter = sector->begin();

	for (iter; iter != sector->end(); ++iter)
	{
		if ((*iter) == ExceptClientID)
			continue;
		else
			SendPacket_Unicast((*iter), pPacket);
	}

	return;
}

void CChatServer::SendPacket_Unicast(CLIENT_ID TargetClientID, CPacket *pPacket)
{
	SendPacket(TargetClientID, pPacket);
	return;
}

void CChatServer::SendPacket_Around(CLIENT_ID TargetClientID, CPacket *pPacket, bool bSendMe)
{
	SECTOR_AROUND sectorAround;
	st_PLAYER *pPlayer = FindPlayer(TargetClientID);
	GetSectorAround(pPlayer->Sector.shSectorX, pPlayer->Sector.shSectorY, &sectorAround);

	for (int iCnt = 0; iCnt < sectorAround.iCount; ++iCnt)
	{
		if (bSendMe)
			SendPacket_SectorOne(sectorAround.Around[iCnt].shSectorX, sectorAround.Around[iCnt].shSectorY, pPacket, NULL);
		else
			SendPacket_SectorOne(sectorAround.Around[iCnt].shSectorX, sectorAround.Around[iCnt].shSectorY, pPacket, TargetClientID);
	}

	return;
}

void CChatServer::SendPacket_Broadcast(CPacket *pPacket)
{
	for (int iY = 0; iY < dfSECTOR_Y_MAX; ++iY)
	{
		for (int iX = 0; iX < dfSECTOR_X_MAX; ++iX)
		{
			std::list<CLIENT_ID> *sector = &_Sector[iY][iX];
			SendPacket_SectorOne(iX, iY, pPacket, NULL);
		}
	}
	return;
}

void CChatServer::GetSectorAround(short iSectorX, short iSectorY, SECTOR_AROUND *pAround)
{
	iSectorX--;
	iSectorY--;

	for (int iY = 0; iY < 3; ++iY)
	{
		if (0 > iSectorY + iY || dfSECTOR_Y_MAX - 1 < iSectorY + iY)
			continue;

		for (int iX = 0; iX < 3; ++iX)
		{
			if (0 > iSectorX + iX || dfSECTOR_X_MAX - 1 < iSectorX + iX)
				continue;

			pAround->Around[pAround->iCount].shSectorX = iSectorX + iX;
			pAround->Around[pAround->iCount].shSectorY = iSectorY + iY;
			pAround->iCount++;
		}
	}

	return;
}