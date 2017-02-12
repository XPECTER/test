#pragma once

/* -- 암호화 프로토콜 ---------------------------------------------------------------------

현재 CPacket 은 헤더영역 eBUFFER_HEADER_SIZE (5)를 기본으로 내장하고 있으며 다음의 헤더 구조를 가지도록 만들어져 있음
이는 Encode, Decode 함수를 호출할 때 진행됨.

Encode, Decode 를 호출하지 않는다면 사용되지 않음.

-------------------------------------------------------------------------------------------


- 1 byte 고정 PacketCode		(패킷 시작을 확인하는 고정 값)	- 생성자에서 전달

- 1 byte 고정 XOR Code 1		(패킷 암호화에 쓰는 고정 값) - 생성자에서 전달
- 1 byte 고정 XOR Code 2		(패킷 암호화에 쓰는 고정 값) - 생성자에서 전달

Code(1byte) - Len(2byte) - Rand XOR Code(1byte) - CheckSum(1byte) - Payload(Len byte)


@ CheckSum

Payload 부분을 1byte 씩 모두 더해서 % 256 한 unsigned char 값

* 고정 XOR Code 1, 2 는 클라와 서버간 고정 XOR 값.
* Rand XOR Code 는 보내는 이가 랜덤하게 1byte 코드를 생성

= 보내기 암호화 과정
1. Rand XOR Code 생성
2. Payload 의 checksum 계산
2. Rand XOR Code 로 [ CheckSum , Payload ] 바이트 단위 xor
3. 고정 XOR Code 1 로 [ Rand XOR Code , CheckSum , Payload ] 를 XOR
4. 고정 XOR Code 2 로 [ Rand XOR Code , CheckSum , Payload ] 를 XOR

= 받기 복호화 과정
1. 고정 XOR Code 2 로 [ Rand XOR Code , CheckSum , Payload ] 를 XOR
2. 고정 XOR Code 1 로 [ Rand XOR Code , CheckSum , Payload ] 를 XOR
3. Rand XOR Code 를 파악.
4. Rand XOR Code 로 [ CheckSum - Payload ] 바이트 단위 xor
5. Payload 를 checksum 공식으로 계산 후 패킷의 checksum 과 비교

Len 은 XOR 하지 않음.

-------------------------------------------------------------------- */

#include <Windows.h>

#pragma pack(push, 1)   
typedef struct st_PACKET_HEADER
{
	BYTE	byCode;
	WORD	shLen;
	BYTE	RandKey;
	BYTE	CheckSum;
}PACKET_HEADER;
#pragma pack(pop)

class CPacket
{
public:
	enum eDEFINE
	{
		dfDEFUALT_BUFF_SIZE = 2000,
		dfHEADER_SIZE = 5 // 버퍼의 크기는 헤더를 포함한 크기.
	};

public:
	struct exception_PacketOut
	{
		int iSize;
		WCHAR szStr[100];
	};

	struct exception_PacketIn
	{
		int iSize;
		WCHAR szStr[100];
	};

	struct exception_PacketFree
	{
		int iSize;
		WCHAR szStr[100];
	};

	struct exception_PacketAlloc
	{
		int iSize;
		WCHAR szStr[100];
	};

private:
	typedef struct st_ENCRYPT_BUFF
	{
		char *buff;
		unsigned long length;
	}ENCRYPT_BUFF;

public:
	CPacket(void);
	CPacket(int _headerSize);
	CPacket(int _headerSize, int _bufferSize);

	~CPacket(void);

	// 버퍼 할당
	void Init(int _headerSize = CPacket::dfHEADER_SIZE,
		int _bufferSize = CPacket::dfDEFUALT_BUFF_SIZE);

	void SetHeaderSize(int headerSize);

	// 버퍼 크기 알아내기
	int GetBuffSize(void);

	// 사용 중인 버퍼 크기
	int GetUseSize(void);

	// 사용 중인 버퍼 크기(헤더 포함)
	int GetPayloadUseSize(void);

	// 사용할 수 있는 버퍼 크기
	int GetFreeSize(void);

	// 버퍼 청소
	void Clear(void);

	// 버퍼 포인터 반환
	char *GetBuffPtr(void);
	char *GetPayloadPtr(void);

	// 현재 읽을 위치의 포인터를 반환
	char *GetCurrPtr(void);

	// 버퍼에 데이터 넣기
	int Enqueue(char *_chpSrc, int _iSrcSize);

	// 버퍼에 데이터 빼기
	int Dequeue(char *_chpDest, int _iDestSize);

	// 버퍼에 헤더 넣기
	void InputHeader(char *_chpSrc, int _iSrcSize);

    // Write 옮기기
    bool MoveWrite(int _iSize);

	/////////////////////////////////////////////////////////////
	// Memory Pool
	static CPacket *Alloc();
	int Free();
	int IncrementRefCount(void);
	int GetRefCount(void);

	/////////////////////////////////////////////////////////////
	// 암호화
	static void SetEncryptCode(BYTE packetCode, BYTE xorCode_1, BYTE xorCode_2);
	void Encode(void);
	bool Decode(PACKET_HEADER *pHeader = nullptr);

public:
	// 연산자 오버로딩
	// BYTE
	CPacket& operator << (BYTE byValue);
	CPacket& operator >> (BYTE &byValue);

	// char
	CPacket& operator << (char *chValue);
	CPacket& operator >> (char *chValue);

	// WCHAR
	CPacket& operator << (WCHAR *wchValue);
	CPacket& operator >> (WCHAR *wchValue);

	// short
	CPacket& operator << (short shValue);
	CPacket& operator >> (short &shValue);

	// int
	CPacket& operator << (int iValue);
	CPacket& operator >> (int &iValue);

	// unsigned int
	CPacket& operator << (unsigned int uValue);
	CPacket& operator >> (unsigned int &uValue);
	
	// double
	CPacket& operator << (double dValue);
	CPacket& operator >> (double &dValue);

	// WORD
	CPacket& operator << (WORD wValue);
	CPacket& operator >> (WORD &wValue);

	// DWORD
	CPacket& operator << (DWORD dwValue);
	CPacket& operator >> (DWORD &dwValue);

    // UINT64
    CPacket& operator << (UINT64 u64Value);
    CPacket& operator >> (UINT64 &u64Value);

	// INT64
	CPacket& operator << (__int64 i64Value);
	CPacket& operator >> (__int64 &i64Value);

public :
	//static CMemoryPool<CPacket> PacketPool;
	static CMemoryPoolTLS<CPacket> PacketPool;

	//- 1 byte 고정 XOR Code 1		(패킷 암호화에 쓰는 고정 값) - 생성자에서 전달
	//- 1 byte 고정 XOR Code 2		(패킷 암호화에 쓰는 고정 값) - 생성자에서 전달
	static BYTE _packetCode;
	static BYTE _packetKey_1;
	static BYTE _packetKey_2;

protected:
	// 버퍼 포인터
	char *m_chpBuff;

	// 버퍼 사이즈
	int m_iBuffSize;

	// 버퍼의 읽기, 쓰기 위치 
	int m_iReadPos;
	int m_iWritePos;

	// 헤더 사이즈 크기
	int m_iHeaderSize;

	// 패킷 카운터
	long m_refCount;

	// Encode가 한 번만 되야한다.
	bool m_bEncode;
};

