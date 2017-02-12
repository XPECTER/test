#pragma once

template <typename T>
class CLockFreeQueue
{
private :
	typedef struct stNODE
	{
		T Data;
		stNODE *pNext;
	}NODE;

	typedef struct stEND_NODE
	{
		NODE *pNode;
		__int64 uniqueNum;
	}END_NODE;

// Func
public :
	CLockFreeQueue()
	{
		NODE *_pDummy = new NODE;
		_pDummy->pNext = NULL;

		_pHead = (END_NODE *)_aligned_malloc(sizeof(END_NODE), 16);
		_pHead->pNode = _pDummy;
		_pHead->uniqueNum = 0;

		_pTail = (END_NODE *)_aligned_malloc(sizeof(END_NODE), 16);
		_pTail->pNode = _pDummy;
		_pTail->uniqueNum = 0;

		_iUniqueNum = 0;
		_iSize = 0;
	}

	~CLockFreeQueue()
	{
		_aligned_free(_pHead);
		_aligned_free(_pTail);
	}

	bool Enqueue(T data)
	{
		NODE *pNewNode = _nodePool.Alloc();
		pNewNode->Data = data;
		pNewNode->pNext = nullptr;

		__int64 newUniqueNum = InterlockedIncrement64(&_iUniqueNum);
		END_NODE oldTail;
		NODE *next = nullptr;

		while (true)
		{
			oldTail.pNode = _pTail->pNode;
			oldTail.uniqueNum = _pTail->uniqueNum;
			next = oldTail.pNode->pNext;
			
			if (nullptr == next)
			{
				if (NULL == InterlockedCompareExchangePointer((PVOID *)&_pTail->pNode->pNext, pNewNode, NULL))
				{
					InterlockedCompareExchange128((LONG64 *)_pTail, _iUniqueNum, (LONG64)pNewNode, (LONG64 *)&oldTail);
					break;
				}
			}
			else
			{
				InterlockedCompareExchange128((LONG64 *)_pTail, _iUniqueNum, (LONG64)next, (LONG64 *)&oldTail);
			}
		}

		InterlockedIncrement(&_iSize);
		return true;
	}

	bool Dequeue(T *pData)
	{
		if (0 > InterlockedDecrement(&_iSize))
		{
			InterlockedIncrement(&_iSize);
			return false;
		}

		__int64 newUniqueNum = InterlockedIncrement64(&_iUniqueNum);
		END_NODE oldHead;
		END_NODE oldTail;
		NODE *next = nullptr;

		while (true)
		{
			oldHead.pNode = _pHead->pNode;
			oldHead.uniqueNum = _pHead->uniqueNum;
			oldTail.pNode = _pTail->pNode;
			oldTail.uniqueNum = _pTail->uniqueNum;
			next = oldHead.pNode->pNext;

			if (oldHead.pNode == _pHead->pNode)
			{
				if (oldHead.pNode == oldTail.pNode)
				{
					if (NULL == next)
						return false;
					else
						InterlockedCompareExchange128((LONG64 *)_pTail, _iUniqueNum, (LONG64)oldTail.pNode->pNext, (LONG64 *)&oldTail);
				}
				else
				{
					*pData = next->Data;
					if (InterlockedCompareExchange128((LONG64 *)_pHead, _iUniqueNum, (LONG64)next, (LONG64 *)&oldHead))
					{
						_nodePool.Free(oldHead.pNode);
						break;
					}
				}
			}
		}

		return true;
	}

	bool Peek(T *pData, int iSequence)
	{
		NODE *pNode = _pHead->pNode;
		int iCnt = 0;

		while (true)
		{
			pNode = pNode->pNext;

			if (NULL == pNode)
				return false;
			else
			{
				if (iSequence == iCnt)
				{
					*pData = pNode->Data;
					break;
				}
				iCnt++;
			}
		}

		return true;
	}

	int GetUseSize(void) { return _iSize; }
private :

// Param
public :

private :
	END_NODE *_pHead;
	END_NODE *_pTail;

	CMemoryPool<NODE> _nodePool;
	__int64 _iUniqueNum;
	long _iSize;
};