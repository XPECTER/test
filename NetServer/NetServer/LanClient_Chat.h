#pragma once

class CLanClient_Chat : public CLanClient
{
public:
	CLanClient_Chat(CChatServer *pServerPtr);
	virtual ~CLanClient_Chat();

	virtual void OnEnterJoinServer(void) override;										// �������� ���� ���� ��
	virtual void OnLeaveServer(void) override;											// �������� ������ �������� ��

	virtual void OnRecv(CPacket *pPacket) override;										// ��Ŷ(�޽���) ����, 1 Message �� 1 �̺�Ʈ.
	virtual void OnSend(int iSendSize) override;

	//virtual void OnWorkerThreadBegin(void) override;										// ��Ŀ������ 1ȸ ���� ����.
	//virtual void OnWorkerThreadEnd(void) override;										// ��Ŀ������ 1ȸ ���� ���� ��

	//virtual void OnError(int iErrorCode, WCHAR *szError) override;						// ���� 

private:
	CChatServer *_pChatServer;
};