#include <Windows.h>
#include "StreamQueue.h"

// ���� ó���� ����ڵ�ο��� �ϳ� ���⼭ �ϳ� ���� ������
// �� ���ϴ���

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
	// ���� ���� �� ť �ʱ�ȭ
	this->m_chpBuffer = (char *)malloc(_iBufferSize);
	memset(this->m_chpBuffer, 0, _iBufferSize);
	this->m_iBufferSize = _iBufferSize;
	InitializeCriticalSection(&m_csQueue);
}

int CStreamQueue::GetBufferSize(void)
{
	// ������ ��ü �뷮�� ����.
	return this->m_iBufferSize;
}

unsigned int CStreamQueue::GetUseSize(void)
{
	// ������ ���� ��뷮 ����. 2���� ��찡 ����. ���� ���� ����
	if (this->m_iWritePos >= this->m_iReadPos)			// ������ �� ���� �ȵ�.
		return this->m_iWritePos - this->m_iReadPos;
	else
		return (this->m_iBufferSize - this->m_iReadPos) + this->m_iWritePos;	// ������ �� ����.
}

int CStreamQueue::GetFreeSize(void)
{
	// ������ ���� ���뷮 ���. ������ ������ ��꿡 ����.
	return  this->m_iBufferSize - this->GetUseSize() - eBUFFER_BLANK;
}

int CStreamQueue::GetNotBrokenGetSize(void)
{
	// ���� �����ͷ� �ܺο��� �ѹ��� ���� �� �ִ� ����
	if (m_iReadPos > m_iWritePos)
		return m_iBufferSize - m_iReadPos;
	else
		return m_iWritePos - m_iReadPos;
}

int CStreamQueue::GetNotBrokenPutsize(void)
{
	// ���� �����ͷ� �ܺο��� �ѹ��� �� �� �ִ� ����
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

// ������ Data�� ��¥ ���� Data��.. ���� ����������
int CStreamQueue::Enqueue(char *_chpData, int _iSize)
{
	if (0 > GetFreeSize() - _iSize)
		return -1;

	// GetFreeSize ��ŭ recv�� ���̴�. �׷��� ť�� ��ĥ ���� ����� ��.
	// ������ ���� ���ԵǾ� ������ �� ���� ���ļ� ť�� �ְ�
	if (m_iWritePos + _iSize > m_iBufferSize)
	{
		// �������� ������ ������ ���� ����
		memcpy_s(m_chpBuffer + m_iWritePos, m_iBufferSize, _chpData, m_iBufferSize - m_iWritePos);

		// ������ �� ������ ����
		memcpy_s(m_chpBuffer, m_iBufferSize, (_chpData + (m_iBufferSize - m_iWritePos)), (_iSize - (m_iBufferSize - m_iWritePos)));
	}
	else // �ƴϸ� �� ���� memcpy�ϰ� ������ �̵�
	{
		memcpy_s(m_chpBuffer + m_iWritePos, m_iBufferSize, _chpData, _iSize);
		//m_iWritePos += _iSize;
	}

	MoveWrite(_iSize);
	return _iSize;
}

// �̰� ���� �������ΰ�?
int CStreamQueue::Dequeue(char *_chpDest, int _iSize)
{
	if (0 > GetUseSize() - _iSize)
		return -1;

	// _chpDest�� �ܺ��� �ӽ� ����
	// ���� ����� ������ ���� ���ԵǾ� ������ �� ���� ���ļ� �д´�
	if (m_iReadPos + _iSize > m_iBufferSize)
	{
		// �������� ������ ������ ī���ϰ�
		memcpy_s(_chpDest, _iSize, (m_chpBuffer + m_iReadPos), (m_iBufferSize - m_iReadPos));

		// ������ �� ���� �� ������ ī��
		memcpy_s((_chpDest + (m_iBufferSize - m_iReadPos)), _iSize, m_chpBuffer, (_iSize - (m_iBufferSize - m_iReadPos)));

		// ������ �̵�
		//m_iReadPos = (_iSize - (m_iBufferSize - m_iReadPos));
	}
	else // �ƴϸ� �� ���� memcpy
	{
		memcpy_s(_chpDest, _iSize, m_chpBuffer + m_iReadPos, _iSize);
		//m_iReadPos += _iSize;
	}

	// ������ �̵���ų ��.
	RemoveData(_iSize);
	return _iSize;
}

// ���� ����. ������ �����ʹ� �̵���Ű�� ����.
int CStreamQueue::Peek(char *_chpDest, int _iSize)
{
	// _chpDest�� �ܺ��� �ӽ� ����
	// ���� ����� ������ ���� ���ԵǾ� ������ �� ���� ���ļ� �д´�
	if (m_iReadPos + _iSize > m_iBufferSize)
	{
		// �������� ������ ������ ī���ϰ�
		memcpy_s(_chpDest, df_TEMP_BUFFER_SIZE, (m_chpBuffer + m_iReadPos), (m_iBufferSize - m_iReadPos));

		// ������ �� ���� �� ������ ī��
		memcpy_s((_chpDest + (m_iBufferSize - m_iReadPos)), df_TEMP_BUFFER_SIZE, m_chpBuffer, (_iSize - (m_iBufferSize - m_iReadPos)));
	}
	else // �ƴϸ� �� ���� memcpy
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



// Read�� ��ġ�� �ű�
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

// Write�� ��ġ�� �ű�
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

// ť�� ��� ���� ����.. ���� ��ġ�� �ٲٴ°�
void CStreamQueue::ClearBuffer(void)
{
	m_iReadPos = 0;
	m_iWritePos = 0;
}

// ť�� ó�� ������ return
char * CStreamQueue::GetBufferPtr(void)
{
	return m_chpBuffer;
}


// ť�� ���� Read��ġ return
char* CStreamQueue::GetReadBufferPtr(void)
{
	return m_chpBuffer + m_iReadPos;
}

// ť�� ���� Write��ġ return
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

// Critical Section�� ����
void CStreamQueue::Lock(void)
{
	EnterCriticalSection(&m_csQueue);
}

// Critical Section���� ����
void CStreamQueue::Unlock(void)
{
	LeaveCriticalSection(&m_csQueue);
}