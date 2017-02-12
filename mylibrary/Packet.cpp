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

// 입력받은 크기만큼의 버퍼를 할당한다. 입력받은 크기가 없다면 기본 크기로 할당한다.
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
	// 주의 : 위치를 초기화 시키기 때문에 패킷을 사용하기 전에 호출해야함.
	// 물론 패킷을 사용하고 바꾸면 에러가 나겠지만..
	this->m_iHeaderSize = headerSize;
	this->m_iReadPos = headerSize;
	this->m_iWritePos = headerSize;

	return;
}

///////////////////////////////////////////////////
// 현재 객체의 버퍼 크기를 반환한다.
///////////////////////////////////////////////////
int CPacket::GetBuffSize(void)
{
	return m_iBuffSize;
}


///////////////////////////////////////////////////
// 객체 안의 버퍼가 사용하고 있는 크기를 반환한다.
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
// 객체 안의 버퍼가 사용할 수 있는 크기를 반환한다.
///////////////////////////////////////////////////
int CPacket::GetFreeSize(void)
{
	return m_iBuffSize - m_iWritePos;
}


///////////////////////////////////////////////////
// 버퍼의 시작위치 포인터를 반환한다.
///////////////////////////////////////////////////
char* CPacket::GetBuffPtr(void)
{
	return m_chpBuff;
}


///////////////////////////////////////////////////
// 버퍼의 Payload부분의 포인터를 반환한다.
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
// 객체의 버퍼가 가지고 있는 데이터를 비운다.
///////////////////////////////////////////////////
void CPacket::Clear(void)
{
	m_iReadPos = m_iHeaderSize;
	m_iWritePos = m_iHeaderSize;
	m_bEncode = false;
}


///////////////////////////////////////////////////
// 버퍼에 데이터를 넣는다. 반환하는 값은 복사한 사이즈
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
// 버퍼에서 데이터를 뺀다. 반환하는 값은 복사한 사이즈
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

// Write의 위치를 옮김
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

		// payload만 가지고 헤더까지 세팅. payload가 있다는 전제하에 Encode호출
		BYTE randXorCode = rand() % 256; // 0x00 ~ 0xff

		// 페이로드 포인터
		buff[0].buff = m_chpBuff + CPacket::dfHEADER_SIZE;
		buff[0].length = this->GetPayloadUseSize();

		// Payload의 1바이트씩 더해서 checkSum 구함
		int checkSum = 0;
		for (int i = m_iReadPos; i < m_iWritePos; ++i)
			checkSum += *(m_chpBuff + i);

		BYTE resultCheckSum = (BYTE)(checkSum % 256);

		// 체크섬 포인터
		buff[1].buff = (char *)&resultCheckSum;
		buff[1].length = sizeof(resultCheckSum);

		// RandKey 포인터
		buff[2].buff = (char *)&randXorCode;
		buff[2].length = sizeof(randXorCode);

		// Payload와 checkSum을 randXorCode로 바이트 단위 XOR
		for (int i = 0; i < 2; ++i)
		{
			for (unsigned int j = 0; j < buff[i].length; ++j)
				*(buff[i].buff + j) = *(buff[i].buff + j) ^ randXorCode;
		}

		// Payload와 checkSum, randXorCode를 고정 XOR Code1로 바이트 단위 XOR
		for (int i = 0; i < 3; ++i)
		{
			for (unsigned int j = 0; j < buff[i].length; ++j)
				*(buff[i].buff + j) = *(buff[i].buff + j) ^ _packetKey_1;
		}

		// Payload와 checkSum, randXorCode를 고정 XOR Code2로 바이트 단위 XOR
		for (int i = 0; i < 3; ++i)
		{
			for (unsigned int j = 0; j < buff[i].length; ++j)
				*(buff[i].buff + j) = *(buff[i].buff + j) ^ _packetKey_2;
		}

		// 헤더 세팅
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

	// Payload와 checkSum, randXorCode를 고정 XOR Code2로 바이트 단위 XOR
	for (int i = 0; i < 3; ++i)
	{
		for (unsigned int j = 0; j < buff[i].length; ++j)
			*(buff[i].buff + j) = *(buff[i].buff + j) ^ _packetKey_2;
	}

	// Payload와 checkSum, randXorCode를 고정 XOR Code1로 바이트 단위 XOR
	for (int i = 0; i < 3; ++i)
	{
		for (unsigned int j = 0; j < buff[i].length; ++j)
			*(buff[i].buff + j) = *(buff[i].buff + j) ^ _packetKey_1;
	}

	BYTE randXorCode = header->RandKey;

	// Payload와 checkSum을 randXorCode로 바이트 단위 XOR
	for (int i = 0; i < 2; ++i)
	{
		for (unsigned int j = 0; j < buff[i].length; ++j)
			*(buff[i].buff + j) = *(buff[i].buff + j) ^ randXorCode;
	}

	unsigned int checkSum = 0;
	for (unsigned int i = 0; i < buff[0].length; ++i)
		checkSum += *(buff[0].buff + i);

	// return false면 밖에서 로그를 찍어줘라.
	if (header->CheckSum == (checkSum % 256))
		return true;
	else
		return false;
}


//////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////
// 연산자 오버로딩 : BYTE
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
// 연산자 오버로딩 : char
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
// 연산자 오버로딩 : WCHAR
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
// i연산자 오버로딩 : short
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
// i연산자 오버로딩 : int
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
// i연산자 오버로딩 : unsigned int
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
// 연산자 오버로딩 : double
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
// 연산자 오버로딩 : WORD
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
// 연산자 오버로딩 : DWORD
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
// 연산자 오버로딩 : UINT64
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
// 연산자 오버로딩 : __int64
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
