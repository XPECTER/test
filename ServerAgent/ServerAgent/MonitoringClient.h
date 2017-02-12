#pragma once

class CMonitoringClient : public CLanClient
{
public:
	virtual void OnEnterJoinServer(void) override;										// �������� ���� ���� ��
	virtual void OnLeaveServer(void) override;											// �������� ������ �������� ��

	virtual void OnRecv(CPacket *pPacket) override;										// ��Ŷ(�޽���) ����, 1 Message �� 1 �̺�Ʈ.
	virtual void OnSend(int iSendSize) override;											// ��Ŷ(�޽���) �۽� �Ϸ� �̺�Ʈ.

	//virtual void OnWorkerThreadBegin(void) = 0;										// ��Ŀ������ 1ȸ ���� ����.
	//virtual void OnWorkerThreadEnd(void) = 0;										// ��Ŀ������ 1ȸ ���� ���� ��

private:
	void SendPacket_MonitorServerLogin(void);
	void MakePacket_MonitorServerLogin(CPacket *pInPacket);

	void PacketProc_ServerControl(CPacket *pRecvPacket);
};