#include "stdafx.h"
#include "ChatServer.h"
#include "LanClient_Chat.h"


CLanClient_Chat::CLanClient_Chat(CChatServer *pServerPtr)
{
	this->_pChatServer = pServerPtr;
}

CLanClient_Chat::~CLanClient_Chat()
{

}

void CLanClient_Chat::OnEnterJoinServer(void)
{

}

void CLanClient_Chat::OnLeaveServer(void)
{

}

void CLanClient_Chat::OnRecv(CPacket *pPacket)
{

}

void CLanClient_Chat::OnSend(int iSendSize)
{

}