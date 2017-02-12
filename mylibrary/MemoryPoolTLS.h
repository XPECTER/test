#pragma once

#define dfMEMORYPOOL_CHECK_CODE 0x5850456754455253

template <typename T>
class CMemoryPoolTLS
{
private:
	class CCHUNK;

	typedef struct st_DATA_BLOCK
	{
		CCHUNK *pReturnChunk;
		UINT64 checkCode;
		T Data;
		st_DATA_BLOCK *pNextBlock;
	}DATA_BLOCK;

	class CCHUNK
	{
	public:
		CCHUNK()
		{
			m_pReturnPool = NULL;
			m_iFreeCount = 0;
			m_iMaxAllocCount = 0;
			m_pTopBlock = NULL;
			m_pCurrBlock = NULL;
		}

		T* AllocFromChunk()
		{
			/*if (NULL == this->m_pCurrBlock)
				return NULL;*/

			T *pReturnData = &m_pCurrBlock->Data;
			m_pCurrBlock = m_pCurrBlock->pNextBlock;

			return pReturnData;
		}

		void FreeToChunk()
		{
			if (0 == InterlockedDecrement((long *)&m_iFreeCount))
				m_pReturnPool->Free(this);

			return;
		}

		void ChunkInit(int _iDataAmount, CMemoryPoolTLS<T> *pTlsPool)
		{
			this->m_iFreeCount = _iDataAmount;
			this->m_iMaxAllocCount = _iDataAmount;
			this->m_pReturnPool = &(pTlsPool->m_ChunkPool);

			DATA_BLOCK *pDataBlock = new DATA_BLOCK[_iDataAmount];

			for (int i = 0; i < _iDataAmount - 1; ++i)
			{
				pDataBlock[i].pNextBlock = &pDataBlock[i + 1];
				pDataBlock[i].pReturnChunk = this;
				pDataBlock[i].checkCode = dfMEMORYPOOL_CHECK_CODE;
			}

			pDataBlock[_iDataAmount - 1].pNextBlock = NULL;
			pDataBlock[_iDataAmount - 1].pReturnChunk = this;
			pDataBlock[_iDataAmount - 1].checkCode = dfMEMORYPOOL_CHECK_CODE;

			this->m_pTopBlock = pDataBlock;
			this->m_pCurrBlock = pDataBlock;

			return;
		}

		void ChunkClear()
		{
			this->m_iFreeCount = this->m_iMaxAllocCount;
			this->m_pCurrBlock = this->m_pTopBlock;

			return;
		}

		CMemoryPool<CCHUNK>* GetReturnPoolPtr()
		{
			return this->m_pReturnPool;
		}

		CMemoryPool<CCHUNK> *m_pReturnPool = nullptr;
	private:
		int m_iFreeCount;
		int m_iMaxAllocCount;
		DATA_BLOCK *m_pTopBlock;
		DATA_BLOCK *m_pCurrBlock;
	};

public:
	CMemoryPoolTLS(int _iChunkDataAmount, bool _bDynamicMode) : m_ChunkPool(0)
	{
		m_iChunkDataAmount = _iChunkDataAmount;
		m_bDynamicMode = _bDynamicMode;

		m_iAllocChunkCount = 0;
		m_iUseBlockCount = 0;

		m_dwTlsIndex = TlsAlloc();
		if (TLS_OUT_OF_INDEXES == m_dwTlsIndex)
			throw TLS_OUT_OF_INDEXES;
	}

	~CMemoryPoolTLS()
	{
		// 모든 chunk free??
	}

	T *Alloc(void)
	{
		CCHUNK *pChunk = (CCHUNK *)TlsGetValue(this->m_dwTlsIndex);
		T *pReturnData = NULL;

		if (NULL == pChunk)
			pChunk = ChunkSettoTLS();

		// 여기서 다른 스레드가 
		// chunk를 반환했다가 다시 alloc받았으면 두 스레드가 같은
		// chunk를 가리키고 있을 수 있음.
		pReturnData = pChunk->AllocFromChunk();
		DATA_BLOCK *pBlock = (DATA_BLOCK *)((char *)pReturnData - 16);
		if (NULL == pBlock->pNextBlock)
		{
			TlsSetValue(this->m_dwTlsIndex, NULL); //?
			ChunkSettoTLS();
		}

		InterlockedIncrement((long *)&m_iUseBlockCount);
		return pReturnData;
	}


	bool Free(T *pFreeData)
	{
		DATA_BLOCK *pBlock = (DATA_BLOCK *)((char *)pFreeData - 16);
		if (dfMEMORYPOOL_CHECK_CODE != pBlock->checkCode)
			return false;

		pBlock->pReturnChunk->FreeToChunk();
		InterlockedDecrement((long *)&m_iUseBlockCount);
		return true;
	}

	// 사용 중인 데이터 블록 갯수
	long GetUseCount(void) { return m_iUseBlockCount; }

	// 할당 받은 chunk 갯수
	long GetAllocCount(void) { return m_iAllocChunkCount; }

private:
	// 메모리풀이 락프리라서 같은 CHUNK가 튀어나올리는 없다.
	CCHUNK* ChunkSettoTLS()
	{
		CCHUNK *pAllocChunk = m_ChunkPool.Alloc();
		
		if (NULL == pAllocChunk->m_pReturnPool)
		{
			pAllocChunk->ChunkInit(this->m_iChunkDataAmount, this);
			InterlockedIncrement((long *)&(this->m_iAllocChunkCount));
		}
		else
			pAllocChunk->ChunkClear();

		if (!TlsSetValue(m_dwTlsIndex, (void *)pAllocChunk))
			throw 0;
		
		return pAllocChunk;
	}

private:
	// tls index
	DWORD m_dwTlsIndex;
	
	// 모니터링 변수
	int m_iAllocChunkCount;
	int m_iUseBlockCount;
	
	// TLS POOL의 모드 true면 동적할당. false면 미리 확보 후 모자르면 그 때 동적할당.
	bool m_bDynamicMode;
	
	// Chunk를 관리하는 메모리 풀
	CMemoryPool<CCHUNK> m_ChunkPool;

	// 한 Chunk에 몇 개의 데이터가 들어가는가
	int m_iChunkDataAmount;
};