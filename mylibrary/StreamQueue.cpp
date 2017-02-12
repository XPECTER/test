#include <Windows.h>
#include "StreamQueue.h"

// 에러 처리를 사용코드부에서 하나 여기서 하나 같지 않을까
// 두 번하던가

CStreamQueue::CStreamQueue(void)
{
	this->m_chpBuffer = NULL;
	this->m_iBufferSize = eBUFFER_SIZE_DEFAULT;
	this->m_iReadPos = 0;
	this->m_iWritePos = 0;

	Initial(m_iBufferSize);
}

CStreamQueue::CStreamQueue(int _iSize)
{
	this->m_chpBuffer = NULL;
	this->m_iBufferSize = _iSize;
	this->m_iReadPos = 0;
	this->m_iWritePos = 0;

	Initial(m_iBufferSize);
}

CStreamQueue::~CStreamQueue()
{
	free(this->m_chpBuffer);
	DeleteCriticalSection(&m_csQueue);
}

void CStreamQueue::Initial(int _iBufferSize)
{
	// 버퍼 생성 및 큐 초기화
	this->m_chpBuffer = (char *)malloc(_iBufferSize);
	memset(this->m_chpBuffer, 0, _iBufferSize);
	this->m_iBufferSize = _iBufferSize;
	InitializeCriticalSection(&m_csQueue);
}

int CStreamQueue::GetBufferSize(void)
{
	// 버퍼의 전체 용량을 얻음.
	return this->m_iBufferSize;
}

unsigned int CStreamQueue::GetUseSize(void)
{
	// 버퍼의 현재 사용량 얻음. 2가지 경우가 있음. 실제 사용된 공간
	if (this->m_iWritePos >= this->m_iReadPos)			// 끊어진 곳 포함 안됨.
		return this->m_iWritePos - this->m_iReadPos;
	else
		return (this->m_iBufferSize - this->m_iReadPos) + this->m_iWritePos;	// 끊어진 곳 포함.
}

int CStreamQueue::GetFreeSize(void)
{
	// 버퍼의 현재 가용량 얻기. 마지막 공간도 계산에 넣음.
	return  this->m_iBufferSize - this->GetUseSize() - eBUFFER_BLANK;
}

int CStreamQueue::GetNotBrokenGetSize(void)
{
	// 버퍼 포인터로 외부에서 한번에 읽을 수 있는 길이
	if (m_iReadPos > m_iWritePos)
		return m_iBufferSize - m_iReadPos;
	else
		return m_iWritePos - m_iReadPos;
}

int CStreamQueue::GetNotBrokenPutsize(void)
{
	// 버퍼 포인터로 외부에서 한번에 쓸 수 있는 길이
	if (m_iReadPos > m_iWritePos)
		return m_iReadPos - m_iWritePos - eBUFFER_BLANK;
	else
	{
		if (0 == m_iReadPos)
			return m_iBufferSize - m_iWritePos - eBUFFER_BLANK;
		else
			return m_iBufferSize - m_iWritePos;
	}
}

// 여기의 Data는 진짜 문자 Data네.. 버퍼 포인터인줄
int CStreamQueue::Enqueue(char *_chpData, int _iSize)
{
	if (0 > GetFreeSize() - _iSize)
		return -1;

	// GetFreeSize 만큼 recv할 것이다. 그래서 큐가 넘칠 일이 희박할 것.
	// 끊어진 곳이 포함되어 있으면 두 번에 걸쳐서 큐에 넣고
	if (m_iWritePos + _iSize > m_iBufferSize)
	{
		// 끊어지기 전까지 데이터 먼저 넣음
		memcpy_s(m_chpBuffer + m_iWritePos, m_iBufferSize, _chpData, m_iBufferSize - m_iWritePos);

		// 끊어진 후 데이터 넣음
		memcpy_s(m_chpBuffer, m_iBufferSize, (_chpData + (m_iBufferSize - m_iWritePos)), (_iSize - (m_iBufferSize - m_iWritePos)));
	}
	else // 아니면 한 번에 memcpy하고 포인터 이동
	{
		memcpy_s(m_chpBuffer + m_iWritePos, m_iBufferSize, _chpData, _iSize);
		//m_iWritePos += _iSize;
	}

	MoveWrite(_iSize);
	return _iSize;
}

// 이건 버퍼 포인터인가?
int CStreamQueue::Dequeue(char *_chpDest, int _iSize)
{
	if (0 > GetUseSize() - _iSize)
		return -1;

	// _chpDest는 외부의 임시 버퍼
	// 읽을 사이즈가 끊어진 곳이 포함되어 있으면 두 번에 걸쳐서 읽는다
	if (m_iReadPos + _iSize > m_iBufferSize)
	{
		// 끊어지기 전까지 데이터 카피하고
		memcpy_s(_chpDest, _iSize, (m_chpBuffer + m_iReadPos), (m_iBufferSize - m_iReadPos));

		// 끊어진 후 남은 곳 데이터 카피
		memcpy_s((_chpDest + (m_iBufferSize - m_iReadPos)), _iSize, m_chpBuffer, (_iSize - (m_iBufferSize - m_iReadPos)));

		// 포인터 이동
		//m_iReadPos = (_iSize - (m_iBufferSize - m_iReadPos));
	}
	else // 아니면 한 번에 memcpy
	{
		memcpy_s(_chpDest, _iSize, m_chpBuffer + m_iReadPos, _iSize);
		//m_iReadPos += _iSize;
	}

	// 포인터 이동시킬 것.
	RemoveData(_iSize);
	return _iSize;
}

// 위랑 같음. 하지만 포인터는 이동시키지 않음.
int CStreamQueue::Peek(char *_chpDest, int _iSize)
{
	// _chpDest는 외부의 임시 버퍼
	// 읽을 사이즈가 끊어진 곳이 포함되어 있으면 두 번에 걸쳐서 읽는다
	if (m_iReadPos + _iSize > m_iBufferSize)
	{
		// 끊어지기 전까지 데이터 카피하고
		memcpy_s(_chpDest, df_TEMP_BUFFER_SIZE, (m_chpBuffer + m_iReadPos), (m_iBufferSize - m_iReadPos));

		// 끊어진 후 남은 곳 데이터 카피
		memcpy_s((_chpDest + (m_iBufferSize - m_iReadPos)), df_TEMP_BUFFER_SIZE, m_chpBuffer, (_iSize - (m_iBufferSize - m_iReadPos)));
	}
	else // 아니면 한 번에 memcpy
	{
		memcpy_s(_chpDest, df_TEMP_BUFFER_SIZE, m_chpBuffer + m_iReadPos, _iSize);
	}

	return _iSize;
}

int CStreamQueue::Peek(char *_chpDest, int _DestSize,int _iStartPos, int _iEndPos)
{
	int tempStartRead = m_iReadPos + _iStartPos;
	int tempEndRead = m_iReadPos + _iEndPos;
	int brokenLen;
	int length = _iEndPos - _iStartPos;

	if (0 > length)
		return -1;

	// r 2192 w 0 t 2200
	if (m_iReadPos > m_iWritePos)
	{
		if (m_iBufferSize > tempStartRead && m_iBufferSize > tempEndRead)
			memcpy_s(_chpDest, _DestSize, m_chpBuffer + tempStartRead, length);
		else if (m_iBufferSize > tempStartRead && m_iBufferSize <= tempEndRead)
		{
			if (m_iWritePos < tempEndRead - m_iBufferSize)
				return -1;

			brokenLen = m_iBufferSize - tempStartRead;
			memcpy_s(_chpDest, _DestSize, m_chpBuffer + tempStartRead, brokenLen);
			memcpy_s(_chpDest + brokenLen, _DestSize - brokenLen, m_chpBuffer, length - brokenLen);
		}
		else
		{
			tempStartRead -= m_iBufferSize;
			tempEndRead -= m_iBufferSize;

			if (m_iWritePos < tempEndRead || m_iWritePos < tempStartRead)
				return -1;

			memcpy_s(_chpDest, _DestSize, m_chpBuffer + tempStartRead, length);
		}
	}
	else
	{
		if (m_iWritePos < tempEndRead || m_iWritePos < tempStartRead)
			return -1;

		memcpy_s(_chpDest, _DestSize, m_chpBuffer + tempStartRead, length);
	}

	return length;
}



// Read의 위치를 옮김
bool CStreamQueue::RemoveData(int _iSize)
{
	int iTempReadPos = m_iReadPos;
	iTempReadPos += _iSize;

	if (m_iReadPos > m_iWritePos)
	{
		if (iTempReadPos > m_iBufferSize)
		{
			iTempReadPos -= m_iBufferSize;

			if (iTempReadPos > m_iWritePos)
				return false;
		}
	}
	else // read < write
	{
		if (iTempReadPos > m_iWritePos)
			return false;
	}

	m_iReadPos = iTempReadPos;

	if (m_iBufferSize == m_iReadPos)
		m_iReadPos = 0;

	return true;
}

// Write의 위치를 옮김
bool CStreamQueue::MoveWrite(int _iSize)
{
	int iTempWritePos = this->m_iWritePos;
	iTempWritePos += _iSize;

	if (m_iReadPos > m_iWritePos)
	{
		if (iTempWritePos > m_iReadPos - eBUFFER_BLANK)
			return false;
	}
	else // read < write
	{
		if (iTempWritePos > m_iBufferSize)
		{
			iTempWritePos -= m_iBufferSize;

			if (iTempWritePos > m_iReadPos - eBUFFER_BLANK)
				return false;
		}
	}

	m_iWritePos = iTempWritePos;

	if (m_iBufferSize == m_iWritePos)
		m_iWritePos = 0;

	return true;
}

// 큐의 모든 내용 삭제.. 지만 위치만 바꾸는거
void CStreamQueue::ClearBuffer(void)
{
	m_iReadPos = 0;
	m_iWritePos = 0;
}

// 큐의 처음 포인터 return
char * CStreamQueue::GetBufferPtr(void)
{
	return m_chpBuffer;
}


// 큐의 현재 Read위치 return
char* CStreamQueue::GetReadBufferPtr(void)
{
	return m_chpBuffer + m_iReadPos;
}

// 큐의 현재 Write위치 return
char* CStreamQueue::GetWriteBufferPtr(void)
{
	return m_chpBuffer + m_iWritePos;
}

int CStreamQueue::GetWritePos(void)
{
	return m_iWritePos;
}

int CStreamQueue::GetReadPos(void)
{
	return m_iReadPos;
}

// Critical Section에 진입
void CStreamQueue::Lock(void)
{
	EnterCriticalSection(&m_csQueue);
}

// Critical Section에서 나옴
void CStreamQueue::Unlock(void)
{
	LeaveCriticalSection(&m_csQueue);
}