#include <new>
#include <Windows.h>
#include <wchar.h>
#include "MemoryPool.h"
#include "MemoryPoolTLS.h"
#include "Packet.h"

CPacket::CPacket(void)
{
	Init();
}


CPacket::CPacket(int _headerSize)
{
	Init(_headerSize);
}


CPacket::CPacket(int _headerSize, int _bufferSize)
{
	Init(_headerSize, _bufferSize);
}


CPacket::~CPacket(void)
{
	if (m_chpBuff != NULL)
		free(m_chpBuff);
}

// �Է¹��� ũ�⸸ŭ�� ���۸� �Ҵ��Ѵ�. �Է¹��� ũ�Ⱑ ���ٸ� �⺻ ũ��� �Ҵ��Ѵ�.
void CPacket::Init(int _headerSize, int _bufferSize)
{
	m_iHeaderSize = _headerSize;
	m_iBuffSize = _bufferSize;
	m_chpBuff = (char *)malloc(_bufferSize);
	
	memset(m_chpBuff, 0, m_iBuffSize);

	m_iWritePos = _headerSize;
	m_iReadPos = _headerSize;
	m_refCount = 0;
	m_bEncode = false;
}

void CPacket::SetHeaderSize(int headerSize)
{
	// ���� : ��ġ�� �ʱ�ȭ ��Ű�� ������ ��Ŷ�� ����ϱ� ���� ȣ���ؾ���.
	// ���� ��Ŷ�� ����ϰ� �ٲٸ� ������ ��������..
	this->m_iHeaderSize = headerSize;
	this->m_iReadPos = headerSize;
	this->m_iWritePos = headerSize;

	return;
}

///////////////////////////////////////////////////
// ���� ��ü�� ���� ũ�⸦ ��ȯ�Ѵ�.
///////////////////////////////////////////////////
int CPacket::GetBuffSize(void)
{
	return m_iBuffSize;
}


///////////////////////////////////////////////////
// ��ü ���� ���۰� ����ϰ� �ִ� ũ�⸦ ��ȯ�Ѵ�.
///////////////////////////////////////////////////
int CPacket::GetUseSize(void)
{
	return m_iWritePos;
}

int CPacket::GetPayloadUseSize(void)
{
	return m_iWritePos - m_iReadPos;
}


///////////////////////////////////////////////////
// ��ü ���� ���۰� ����� �� �ִ� ũ�⸦ ��ȯ�Ѵ�.
///////////////////////////////////////////////////
int CPacket::GetFreeSize(void)
{
	return m_iBuffSize - m_iWritePos;
}


///////////////////////////////////////////////////
// ������ ������ġ �����͸� ��ȯ�Ѵ�.
///////////////////////////////////////////////////
char* CPacket::GetBuffPtr(void)
{
	return m_chpBuff;
}


///////////////////////////////////////////////////
// ������ Payload�κ��� �����͸� ��ȯ�Ѵ�.
///////////////////////////////////////////////////
char* CPacket::GetPayloadPtr(void)
{
	return m_chpBuff + m_iHeaderSize;
}


char* CPacket::GetCurrPtr(void)
{
	return m_chpBuff + m_iReadPos;
}



///////////////////////////////////////////////////
// ��ü�� ���۰� ������ �ִ� �����͸� ����.
///////////////////////////////////////////////////
void CPacket::Clear(void)
{
	m_iReadPos = m_iHeaderSize;
	m_iWritePos = m_iHeaderSize;
	m_bEncode = false;
}


///////////////////////////////////////////////////
// ���ۿ� �����͸� �ִ´�. ��ȯ�ϴ� ���� ������ ������
///////////////////////////////////////////////////
int CPacket::Enqueue(char *_chpSrc, int _iSrcSize)
{
	if (m_iWritePos + _iSrcSize > m_iBuffSize)
	{
		exception_PacketIn e;
		e.iSize = _iSrcSize;
		wmemcpy_s(e.szStr, 100, L"Buffer OverRun", 100);

		throw e;
		return 0;
	}

	memcpy_s(m_chpBuff + m_iWritePos, m_iBuffSize, _chpSrc, _iSrcSize);
	m_iWritePos += _iSrcSize;

	return _iSrcSize;
}


///////////////////////////////////////////////////
// ���ۿ��� �����͸� ����. ��ȯ�ϴ� ���� ������ ������
///////////////////////////////////////////////////
int CPacket::Dequeue(char *_chpDest, int _iDestSize)
{
	if (GetFreeSize() < _iDestSize)
	{
		exception_PacketOut e;
		e.iSize = _iDestSize;
		wmemcpy_s(e.szStr, 100, L"No Data", 100);

		throw e;
		return 0;
	}

	memcpy_s(_chpDest, _iDestSize, m_chpBuff + m_iReadPos, _iDestSize);
	m_iReadPos += _iDestSize;

	return _iDestSize;
}


void CPacket::InputHeader(char *_chpSrc, int _iSrcSize)
{
	memcpy_s(m_chpBuff, m_iBuffSize, _chpSrc, _iSrcSize);
	return;
}

// Write�� ��ġ�� �ű�
bool CPacket::MoveWrite(int _iSize)
{
    if (m_iWritePos + _iSize > m_iBuffSize)
        return false;
    else
    {
        this->m_iWritePos += _iSize;
        return true;
    }
}


CPacket* CPacket::Alloc()
{
	CPacket *pPacket = PacketPool.Alloc();
	pPacket->Clear();
	
	if (1 != InterlockedIncrement(&pPacket->m_refCount))
	{
		exception_PacketAlloc e;
		e.iSize = 0;
		wmemcpy_s(e.szStr, 100, L"Miss match Alloc refcount", 100);
		throw e;
	}

	return pPacket;
}

int CPacket::Free()
{
	int count;
	count = InterlockedDecrement(&this->m_refCount);

	if (0 == count)
		PacketPool.Free(this);
	else if (0 > count)
	{
		exception_PacketFree e;
		e.iSize = 0;
		wmemcpy_s(e.szStr, 100, L"Miss match Free refcount", 100);
		throw e;
	}
		
	return count;
}

int CPacket::IncrementRefCount(void)
{
	int RefCount;
	RefCount = InterlockedIncrement(&this->m_refCount);
	return RefCount;
}

int CPacket::GetRefCount(void)
{
	return this->m_refCount;
}

void CPacket::SetEncryptCode(BYTE packetCode, BYTE packetKey_1, BYTE packetKey_2)
{
	_packetCode = packetCode;
	_packetKey_1 = packetKey_1;
	_packetKey_2 = packetKey_2;

	return;
}

void CPacket::Encode(void)
{
	if (true == this->m_bEncode)
		return;
	else
	{
		this->m_bEncode = true;
		ENCRYPT_BUFF buff[3];

		// payload�� ������ ������� ����. payload�� �ִٴ� �����Ͽ� Encodeȣ��
		BYTE randXorCode = rand() % 256; // 0x00 ~ 0xff

		// ���̷ε� ������
		buff[0].buff = m_chpBuff + CPacket::dfHEADER_SIZE;
		buff[0].length = this->GetPayloadUseSize();

		// Payload�� 1����Ʈ�� ���ؼ� checkSum ����
		int checkSum = 0;
		for (int i = m_iReadPos; i < m_iWritePos; ++i)
			checkSum += *(m_chpBuff + i);

		BYTE resultCheckSum = (BYTE)(checkSum % 256);

		// üũ�� ������
		buff[1].buff = (char *)&resultCheckSum;
		buff[1].length = sizeof(resultCheckSum);

		// RandKey ������
		buff[2].buff = (char *)&randXorCode;
		buff[2].length = sizeof(randXorCode);

		// Payload�� checkSum�� randXorCode�� ����Ʈ ���� XOR
		for (int i = 0; i < 2; ++i)
		{
			for (unsigned int j = 0; j < buff[i].length; ++j)
				*(buff[i].buff + j) = *(buff[i].buff + j) ^ randXorCode;
		}

		// Payload�� checkSum, randXorCode�� ���� XOR Code1�� ����Ʈ ���� XOR
		for (int i = 0; i < 3; ++i)
		{
			for (unsigned int j = 0; j < buff[i].length; ++j)
				*(buff[i].buff + j) = *(buff[i].buff + j) ^ _packetKey_1;
		}

		// Payload�� checkSum, randXorCode�� ���� XOR Code2�� ����Ʈ ���� XOR
		for (int i = 0; i < 3; ++i)
		{
			for (unsigned int j = 0; j < buff[i].length; ++j)
				*(buff[i].buff + j) = *(buff[i].buff + j) ^ _packetKey_2;
		}

		// ��� ����
		PACKET_HEADER header;
		header.byCode = CPacket::_packetCode;
		header.shLen = this->GetPayloadUseSize();
		header.RandKey = *(buff[2].buff);
		header.CheckSum = *(buff[1].buff);

		this->InputHeader((char *)&header, sizeof(header));
		return;
	}
}

bool CPacket::Decode(PACKET_HEADER *pHeader)
{
	PACKET_HEADER *header = nullptr;

	if (nullptr == pHeader)
		header = (PACKET_HEADER *)m_chpBuff;
	else
		header = pHeader;

	// 0 - payload, 1 - checksum, 2 - randxorcode
	ENCRYPT_BUFF buff[3];
	buff[0].buff = this->GetPayloadPtr();
	buff[0].length = this->GetPayloadUseSize();
	buff[1].buff = (char *)&header->CheckSum;
	buff[1].length = sizeof(BYTE);
	buff[2].buff = (char *)&header->RandKey;
	buff[2].length = sizeof(BYTE);

	// Payload�� checkSum, randXorCode�� ���� XOR Code2�� ����Ʈ ���� XOR
	for (int i = 0; i < 3; ++i)
	{
		for (unsigned int j = 0; j < buff[i].length; ++j)
			*(buff[i].buff + j) = *(buff[i].buff + j) ^ _packetKey_2;
	}

	// Payload�� checkSum, randXorCode�� ���� XOR Code1�� ����Ʈ ���� XOR
	for (int i = 0; i < 3; ++i)
	{
		for (unsigned int j = 0; j < buff[i].length; ++j)
			*(buff[i].buff + j) = *(buff[i].buff + j) ^ _packetKey_1;
	}

	BYTE randXorCode = header->RandKey;

	// Payload�� checkSum�� randXorCode�� ����Ʈ ���� XOR
	for (int i = 0; i < 2; ++i)
	{
		for (unsigned int j = 0; j < buff[i].length; ++j)
			*(buff[i].buff + j) = *(buff[i].buff + j) ^ randXorCode;
	}

	unsigned int checkSum = 0;
	for (unsigned int i = 0; i < buff[0].length; ++i)
		checkSum += *(buff[0].buff + i);

	// return false�� �ۿ��� �α׸� ������.
	if (header->CheckSum == (checkSum % 256))
		return true;
	else
		return false;
}


//////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////
// ������ �����ε� : BYTE
///////////////////////////////////////////////////
CPacket& CPacket::operator << (BYTE byValue)
{
	Enqueue(reinterpret_cast<char *>(&byValue), sizeof(BYTE));
	return *this;
}

CPacket& CPacket::operator >> (BYTE &byValue)
{
	Dequeue(reinterpret_cast<char *>(&byValue), sizeof(BYTE));
	return *this;
}


///////////////////////////////////////////////////
// ������ �����ε� : char
///////////////////////////////////////////////////
CPacket& CPacket::operator << (char *chValue)
{
	Enqueue(chValue, (unsigned int)strlen(chValue));
	return *this;
}

CPacket& CPacket::operator >> (char *chValue)
{
	Dequeue(chValue, (unsigned int)strlen(this->GetBuffPtr() + this->m_iWritePos));
	return *this;
}


///////////////////////////////////////////////////
// ������ �����ε� : WCHAR
///////////////////////////////////////////////////
CPacket& CPacket::operator << (WCHAR *wchValue)
{
	Enqueue(reinterpret_cast<char *>(wchValue), (unsigned int)wcslen(wchValue));
	return *this;
}

CPacket& CPacket::operator >> (WCHAR *wchValue)
{
	Dequeue(reinterpret_cast<char *>(wchValue), (unsigned int)wcslen((WCHAR *)this->GetBuffPtr() + this->m_iWritePos));
	return *this;
}


///////////////////////////////////////////////////
// i������ �����ε� : short
///////////////////////////////////////////////////
CPacket& CPacket::operator << (short shValue)
{
	Enqueue((char *)&shValue, sizeof(short));
	return *this;
}

CPacket& CPacket::operator >> (short &shValue)
{
	Dequeue((char *)&shValue, sizeof(short));
	return *this;
}


///////////////////////////////////////////////////
// i������ �����ε� : int
///////////////////////////////////////////////////
CPacket& CPacket::operator << (int iValue)
{
    Enqueue((char *)&iValue, sizeof(int));
    return *this;
}

CPacket& CPacket::operator >> (int &iValue)
{
    Dequeue((char *)&iValue, sizeof(int));
    return *this;
}


///////////////////////////////////////////////////
// i������ �����ε� : unsigned int
///////////////////////////////////////////////////
CPacket& CPacket::operator << (unsigned int iValue)
{
	Enqueue((char *)&iValue, sizeof(unsigned int));
	return *this;
}

CPacket& CPacket::operator >> (unsigned int &iValue)
{
	Dequeue((char *)&iValue, sizeof(unsigned int));
	return *this;
}


///////////////////////////////////////////////////
// ������ �����ε� : double
///////////////////////////////////////////////////
CPacket& CPacket::operator << (double dValue)
{
	Enqueue(reinterpret_cast<char *>(&dValue), sizeof(double));
	return *this;
}

CPacket& CPacket::operator >> (double &dValue)
{
	Dequeue(reinterpret_cast<char *>(&dValue), sizeof(double));
	return *this;
}


///////////////////////////////////////////////////
// ������ �����ε� : WORD
///////////////////////////////////////////////////
CPacket& CPacket::operator << (WORD wValue)
{
	Enqueue(reinterpret_cast<char *>(&wValue), sizeof(WORD));
	return *this;
}

CPacket& CPacket::operator >> (WORD &wValue)
{
	Dequeue(reinterpret_cast<char *>(&wValue), sizeof(WORD));
	return *this;
}


///////////////////////////////////////////////////
// ������ �����ε� : DWORD
///////////////////////////////////////////////////
CPacket& CPacket::operator << (DWORD dwValue)
{
	Enqueue(reinterpret_cast<char *>(&dwValue), sizeof(DWORD));
	return *this;
}

CPacket& CPacket::operator >> (DWORD &dwValue)
{
	Dequeue(reinterpret_cast<char *>(&dwValue), sizeof(DWORD));
	return *this;
}


///////////////////////////////////////////////////
// ������ �����ε� : UINT64
///////////////////////////////////////////////////
CPacket& CPacket::operator << (UINT64 u64Value)
{
    Enqueue(reinterpret_cast<char *>(&u64Value), sizeof(UINT64));
    return *this;
}

CPacket& CPacket::operator >> (UINT64 &u64Value)
{
    Dequeue(reinterpret_cast<char *>(&u64Value), sizeof(UINT64));
    return *this;
}


///////////////////////////////////////////////////
// ������ �����ε� : __int64
///////////////////////////////////////////////////
CPacket& CPacket::operator << (__int64 i64Value)
{
	Enqueue(reinterpret_cast<char *>(&i64Value), sizeof(__int64));
	return *this;
}

CPacket& CPacket::operator >> (__int64 &i64Value)
{
	Dequeue(reinterpret_cast<char *>(&i64Value), sizeof(__int64));
	return *this;
}
