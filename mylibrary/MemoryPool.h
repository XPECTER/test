#pragma once

template<class T>
class CMemoryPool
{
private:
    typedef struct st_NODE
    {
		T Data;
		st_NODE* pNext;

		st_NODE()
		{
			pNext = NULL;
		}
    }NODE;

	typedef struct st_TOP_NODE
	{
		NODE *pNode;
		__int64 uniqueNum;
	}TOP_NODE;

public:
    CMemoryPool(int _amount = 0)
	{
		m_iUseCount = 0;
		m_iAllocCount = 0;
		m_iUniqueNum = 0;
		
		// Top Node
		m_pFreeTop = (TOP_NODE *)_aligned_malloc(sizeof(TOP_NODE), 16);
		m_pFreeTop->pNode = NULL;
		m_pFreeTop->uniqueNum = 0;

		NODE* tempNode = nullptr;

		if (0 == _amount)
		{
			bDynamicMode = true;
		}
		else
		{
			bDynamicMode = false;
			m_iAllocCount = _amount;

			for (int i = 0; i < m_iAllocCount; ++i)
			{
				// 연결 시켜줘야 한다.
				tempNode = CreateNode();
			}
		}

		return;
	}

    virtual ~CMemoryPool() 
	{
		NODE *deleteNode = NULL;
		NODE *deleteNextNode = NULL;

		while (TRUE)
		{
			deleteNode = m_pFreeTop->pNode;

			if (NULL == deleteNode)
				break;

			deleteNextNode = m_pFreeTop->pNode->pNext;
			delete deleteNode;
			deleteNode = deleteNextNode;
		}

		return;
	}

	//////////////////////////////////////////////////////////////
	// Dynamic Mode : m_pFreeNode가 NULL이면 새로 new 해서 돌려줌 
	// Static Mode	: m_pFreeNode가 NULL이면 nullptr 돌려줌
	//////////////////////////////////////////////////////////////
    T* Alloc()
    {
		NODE *AllocNode = NULL;
		int _iAllocCount = m_iAllocCount;

		if (_iAllocCount < InterlockedIncrement((long *)&m_iUseCount))
		{
			AllocNode = new NODE;
			InterlockedIncrement((long *)&m_iAllocCount);
			return (T *)AllocNode;
		}
		else
		{
			__int64 uniqueNum = InterlockedIncrement64((LONG64 *)&m_iUniqueNum);
			TOP_NODE pOldTop;

			do
			{
				pOldTop.pNode = m_pFreeTop->pNode;
				pOldTop.uniqueNum = m_pFreeTop->uniqueNum;
			} while (!InterlockedCompareExchange128((LONG64 *)m_pFreeTop, uniqueNum, (LONG64)pOldTop.pNode->pNext, (LONG64 *)&pOldTop));

			AllocNode = pOldTop.pNode;
			return (T *)AllocNode;
		}
    }

    // 메모리 반환은 하지만 삭제하진 않음. 리스트로 관리.
    bool Free(T *pFree)
    {
		TOP_NODE pOldTop;
		TOP_NODE pNewTop;
			
		pNewTop.pNode = (NODE *)pFree;
		pNewTop.uniqueNum = InterlockedIncrement64((LONG64 *)&m_iUniqueNum);

		do
		{
			pOldTop.pNode = m_pFreeTop->pNode;
			pOldTop.uniqueNum = m_pFreeTop->uniqueNum;
				
			pNewTop.pNode->pNext = pOldTop.pNode;
		} while (!InterlockedCompareExchange128((LONG64 *)m_pFreeTop, pNewTop.uniqueNum, (LONG64)pNewTop.pNode, (LONG64 *)&pOldTop));

		if (0 > InterlockedDecrement((long *)&m_iUseCount))
			return false;
		else
			return true;
    }

	int GetUseCount(void) { return m_iUseCount; }
	int GetAllocCount(void) { return m_iAllocCount; }

private :
	// Static Mode 일 때 사용
	NODE* CreateNode(void)
	{
		NODE *newNode = NULL;
		newNode = new NODE;
		 
		m_iAllocCount++;
		return newNode;
	}

private :
	// Block 개수
	int m_iAllocCount;
	int m_iUseCount;

	__int64 m_iUniqueNum;
	TOP_NODE *m_pFreeTop;

	bool bDynamicMode;
};