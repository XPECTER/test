#pragma once

/* -- ��ȣȭ �������� ---------------------------------------------------------------------

���� CPacket �� ������� eBUFFER_HEADER_SIZE (5)�� �⺻���� �����ϰ� ������ ������ ��� ������ �������� ������� ����
�̴� Encode, Decode �Լ��� ȣ���� �� �����.

Encode, Decode �� ȣ������ �ʴ´ٸ� ������ ����.

-------------------------------------------------------------------------------------------


- 1 byte ���� PacketCode		(��Ŷ ������ Ȯ���ϴ� ���� ��)	- �����ڿ��� ����

- 1 byte ���� XOR Code 1		(��Ŷ ��ȣȭ�� ���� ���� ��) - �����ڿ��� ����
- 1 byte ���� XOR Code 2		(��Ŷ ��ȣȭ�� ���� ���� ��) - �����ڿ��� ����

Code(1byte) - Len(2byte) - Rand XOR Code(1byte) - CheckSum(1byte) - Payload(Len byte)


@ CheckSum

Payload �κ��� 1byte �� ��� ���ؼ� % 256 �� unsigned char ��

* ���� XOR Code 1, 2 �� Ŭ��� ������ ���� XOR ��.
* Rand XOR Code �� ������ �̰� �����ϰ� 1byte �ڵ带 ����

= ������ ��ȣȭ ����
1. Rand XOR Code ����
2. Payload �� checksum ���
2. Rand XOR Code �� [ CheckSum , Payload ] ����Ʈ ���� xor
3. ���� XOR Code 1 �� [ Rand XOR Code , CheckSum , Payload ] �� XOR
4. ���� XOR Code 2 �� [ Rand XOR Code , CheckSum , Payload ] �� XOR

= �ޱ� ��ȣȭ ����
1. ���� XOR Code 2 �� [ Rand XOR Code , CheckSum , Payload ] �� XOR
2. ���� XOR Code 1 �� [ Rand XOR Code , CheckSum , Payload ] �� XOR
3. Rand XOR Code �� �ľ�.
4. Rand XOR Code �� [ CheckSum - Payload ] ����Ʈ ���� xor
5. Payload �� checksum �������� ��� �� ��Ŷ�� checksum �� ��

Len �� XOR ���� ����.

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
		dfHEADER_SIZE = 5 // ������ ũ��� ����� ������ ũ��.
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

	// ���� �Ҵ�
	void Init(int _headerSize = CPacket::dfHEADER_SIZE,
		int _bufferSize = CPacket::dfDEFUALT_BUFF_SIZE);

	void SetHeaderSize(int headerSize);

	// ���� ũ�� �˾Ƴ���
	int GetBuffSize(void);

	// ��� ���� ���� ũ��
	int GetUseSize(void);

	// ��� ���� ���� ũ��(��� ����)
	int GetPayloadUseSize(void);

	// ����� �� �ִ� ���� ũ��
	int GetFreeSize(void);

	// ���� û��
	void Clear(void);

	// ���� ������ ��ȯ
	char *GetBuffPtr(void);
	char *GetPayloadPtr(void);

	// ���� ���� ��ġ�� �����͸� ��ȯ
	char *GetCurrPtr(void);

	// ���ۿ� ������ �ֱ�
	int Enqueue(char *_chpSrc, int _iSrcSize);

	// ���ۿ� ������ ����
	int Dequeue(char *_chpDest, int _iDestSize);

	// ���ۿ� ��� �ֱ�
	void InputHeader(char *_chpSrc, int _iSrcSize);

    // Write �ű��
    bool MoveWrite(int _iSize);

	/////////////////////////////////////////////////////////////
	// Memory Pool
	static CPacket *Alloc();
	int Free();
	int IncrementRefCount(void);
	int GetRefCount(void);

	/////////////////////////////////////////////////////////////
	// ��ȣȭ
	static void SetEncryptCode(BYTE packetCode, BYTE xorCode_1, BYTE xorCode_2);
	void Encode(void);
	bool Decode(PACKET_HEADER *pHeader = nullptr);

public:
	// ������ �����ε�
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

	//- 1 byte ���� XOR Code 1		(��Ŷ ��ȣȭ�� ���� ���� ��) - �����ڿ��� ����
	//- 1 byte ���� XOR Code 2		(��Ŷ ��ȣȭ�� ���� ���� ��) - �����ڿ��� ����
	static BYTE _packetCode;
	static BYTE _packetKey_1;
	static BYTE _packetKey_2;

protected:
	// ���� ������
	char *m_chpBuff;

	// ���� ������
	int m_iBuffSize;

	// ������ �б�, ���� ��ġ 
	int m_iReadPos;
	int m_iWritePos;

	// ��� ������ ũ��
	int m_iHeaderSize;

	// ��Ŷ ī����
	long m_refCount;

	// Encode�� �� ���� �Ǿ��Ѵ�.
	bool m_bEncode;
};

