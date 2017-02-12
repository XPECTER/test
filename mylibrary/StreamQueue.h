// Circler Queue
#pragma once


#define df_TEMP_BUFFER_SIZE 2000
class CStreamQueue
{
public :
	enum eSTREAM_QUEUE
	{
		eBUFFER_SIZE_DEFAULT	= 4000,
		eBUFFER_BLANK			= 1			// 8, 4�� �ص� �ǰ� �� ��� ����
	};

public :
	///////////////////////////////////////
	// ������, �Ҹ���. 
	///////////////////////////////////////
	CStreamQueue(void);						// default ������. Queue_size 20480
	CStreamQueue(int _iSize);				// Queue_size�� �Է¹��� ������.

	virtual ~CStreamQueue();				// default �Ҹ���

	///////////////////////////////////////
	// Buffer���� �� ť �ʱ�ȭ
	///////////////////////////////////////
	void Initial(int _iBufferSize = eBUFFER_SIZE_DEFAULT);

	///////////////////////////////////////
	// ���� ��ü�� �뷮�� ����.
	///////////////////////////////////////
	int GetBufferSize(void);

	///////////////////////////////////////
	// ������ ���� ��뷮 ���.
	///////////////////////////////////////
	unsigned int GetUseSize(void);

	///////////////////////////////////////
	// ������ ���� ���뷮 ���.
	///////////////////////////////////////
	int GetFreeSize(void);

	///////////////////////////////////////
	// ���� �����ͷ� �ܺο��� �ѹ��� �а� �� �� �ִ� ����
	///////////////////////////////////////
	int GetNotBrokenGetSize(void);
	int GetNotBrokenPutsize(void);

	///////////////////////////////////////
	// Write ��ġ�� ������ ����.
	///////////////////////////////////////
	int Enqueue(char *_chpData, int _iSize);

	///////////////////////////////////////
	// Read ��ġ�� �ִ� ������ ������. Read������ �̵�
	///////////////////////////////////////
	int Dequeue(char *_chpDest, int _iSize);

	///////////////////////////////////////
	// Read ��ġ�� �ִ� ������ ������. Read������ ����
	///////////////////////////////////////
	int Peek(char *_chpDest, int _iSize);
	int Peek(char *_chpDest, int _DestSize, int _iStartPos, int _iEndPos);

	///////////////////////////////////////
	// ���ϴ� ���̸�ŭ Read��ġ���� ����.
	///////////////////////////////////////
	bool RemoveData(int _iSize);

	///////////////////////////////////////
	// ���ϴ� ���̸�ŭ Write��ġ �̵�.
	///////////////////////////////////////
	bool MoveWrite(int _iSize);

	///////////////////////////////////////
	// Buffer�� ��� ������ ����
	///////////////////////////////////////
	void ClearBuffer(void);

	///////////////////////////////////////
	// Read�� ��ġ�� Write�� ��ġ ���� ������.
	///////////////////////////////////////
	char *GetBufferPtr(void);
	char *GetReadBufferPtr(void);
	char *GetWriteBufferPtr(void);

	///////////////////////////////////////
	int GetWritePos(void);
	int GetReadPos(void);

	
	///////////////////////////////////////
	// Enter Critical Section
	void Lock(void);

	///////////////////////////////////////
	// Leave Critical Section
	void Unlock(void);


protected :
	char *m_chpBuffer;					// Buffer ������
	int m_iBufferSize;					// Buffer ������
	int m_iReadPos;						// Read ��ġ
	int m_iWritePos;					// Write ��ġ
	CRITICAL_SECTION m_csQueue;		// ������ ȯ���� ���� ���.
};