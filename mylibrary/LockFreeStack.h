#pragma once

template <typename T>
class CLockFreeStack
{
public :
	typedef struct st_NODE
	{
		T Data;
		st_NODE *pNext;
	}NODE;

	typedef struct st_TOP_NODE
	{
		NODE *pNode;
		__int64 iUniqueNum;
	}TOP_NODE;

public :
	CLockFreeStack()
	{
		_pTop = (TOP_NODE *)_aligned_malloc(sizeof(TOP_NODE), 16);
		_pTop->pNode = NULL;
		_pTop->iUniqueNum = 0;

		_uniqueNum = 0;
	}

	~CLockFreeStack()
	{
		_aligned_free(_pTop);
	}

	bool Push(T _data)
	{
		__int64 uniqueNum = InterlockedIncrement64((LONG64 *)&_uniqueNum);
		TOP_NODE pOldTop;
		NODE *pNewNode = nodePool.Alloc();
		pNewNode->Data = _data;

		do
		{
			pOldTop.pNode = _pTop->pNode;
			pOldTop.iUniqueNum = _pTop->iUniqueNum;

			pNewNode->pNext = pOldTop.pNode;
		} while (!InterlockedCompareExchange128((LONG64 *)_pTop, uniqueNum, (LONG64)pNewNode, (LONG64 *)&pOldTop));

		InterlockedIncrement((long *)&iSize);
		return TRUE;
	}

	bool Pop(T *_pData)
	{
		if (0 > InterlockedDecrement((long *)&iSize))
		{
			InterlockedIncrement((long *)&iSize);
			return FALSE;
		}

		__int64 uniqueNum = InterlockedIncrement64((LONG64 *)&_uniqueNum);
		NODE *pReturnNode = NULL;
		TOP_NODE pOldTop;  
	
		do
		{
			pOldTop.pNode = _pTop->pNode;
			pOldTop.iUniqueNum = _pTop->iUniqueNum;
			
			pReturnNode = pOldTop.pNode;
		} while (!InterlockedCompareExchange128((LONG64 *)_pTop, uniqueNum, (LONG64)pOldTop.pNode->pNext, (LONG64 *)&pOldTop));

		*_pData = pReturnNode->Data;
		nodePool.Free(pReturnNode);
		return TRUE;
	}

	int GetUseSize(void) { return iSize; }
private :
	TOP_NODE *_pTop;
	__int64 _uniqueNum;

	CMemoryPool<NODE> nodePool;

	int iSize;
};