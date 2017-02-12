#pragma once

class CLanClient_Chat : public CLanClient
{
public:
	CLanClient_Chat(CChatServer *pServerPtr);
	virtual ~CLanClient_Chat();

	virtual void OnEnterJoinServer(void) override;										// 서버와의 연결 성공 후
	virtual void OnLeaveServer(void) override;											// 서버와의 연결이 끊어졌을 때

	virtual void OnRecv(CPacket *pPacket) override;										// 패킷(메시지) 수신, 1 Message 당 1 이벤트.
	virtual void OnSend(int iSendSize) override;

	//virtual void OnWorkerThreadBegin(void) override;										// 워커스레드 1회 돌기 직전.
	//virtual void OnWorkerThreadEnd(void) override;										// 워커스레드 1회 돌기 종료 후

	//virtual void OnError(int iErrorCode, WCHAR *szError) override;						// 에러 

private:
	CChatServer *_pChatServer;
};