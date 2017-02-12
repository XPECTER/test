#pragma once

class CMonitoringClient : public CLanClient
{
public:
	virtual void OnEnterJoinServer(void) override;										// 서버와의 연결 성공 후
	virtual void OnLeaveServer(void) override;											// 서버와의 연결이 끊어졌을 때

	virtual void OnRecv(CPacket *pPacket) override;										// 패킷(메시지) 수신, 1 Message 당 1 이벤트.
	virtual void OnSend(int iSendSize) override;											// 패킷(메시지) 송신 완료 이벤트.

	//virtual void OnWorkerThreadBegin(void) = 0;										// 워커스레드 1회 돌기 직전.
	//virtual void OnWorkerThreadEnd(void) = 0;										// 워커스레드 1회 돌기 종료 후

private:
	void SendPacket_MonitorServerLogin(void);
	void MakePacket_MonitorServerLogin(CPacket *pInPacket);

	void PacketProc_ServerControl(CPacket *pRecvPacket);
};