#include "../../include/mdf/FastMemoryPool.h"
#include "../../include/mdf/mapi.h"
#include "../../include/mdf/atom.h"

namespace mdf {

    FastMemoryPool::FastMemoryPool(int objectSize, int count) {
        if (0 < objectSize % 4)
            m_objectSize = objectSize + (4 - objectSize % 4);
        else
            m_objectSize = objectSize;
        m_objectSize = objectSize;
        m_expandCount = count;
        m_poolHeader = NULL;
        m_header = NULL;

        m_ownerThreadId = -1;
        m_threadCount = 0;
    }

    FastMemoryPool::FastMemoryPool(int objectSize, int count, uint64 ownerThreadId) {
        if (0 < objectSize % 4)
            m_objectSize = objectSize + (4 - objectSize % 4);
        else
            m_objectSize = objectSize;
        m_expandCount = count;
        m_poolHeader = NULL;
        m_header = NULL;

        m_ownerThreadId = CurThreadId();
        m_threadCount = 0;
    }

    FastMemoryPool::~FastMemoryPool() {
        POOL* pPool = NULL;
        while (NULL != m_poolHeader) {
            pPool = m_poolHeader;
            m_poolHeader = m_poolHeader->next;
            MDF_SAFE_DELETE_ARRAY(pPool->buffer);
            MDF_SAFE_DELETE(pPool);
        }

        for (int i = 0; i < m_threadCount; i++) {
            MDF_SAFE_DELETE(m_poolForThread[i]);
        }
    }

    void FastMemoryPool::Expand() {
        POOL* pPool = new POOL;
        unsigned char* buffer = new unsigned char[(sizeof(MEMORY) + m_objectSize) * m_expandCount];
        if (NULL == buffer || NULL == pPool) mdf_assert(false); //系统内存用完

        pPool->freeCount = m_expandCount;
        pPool->buffer = buffer;
        pPool->next = m_poolHeader;
        m_poolHeader = pPool;

        int pos = 0;
        MEMORY* pMemory = (MEMORY*) buffer;
        pMemory->pPool = pPool;
        pMemory->isAlloced = 0;
        pMemory->buffer = &buffer[sizeof(MEMORY) + pos];
        pMemory->pThis = this;
        pMemory->next = NULL;
        pos += sizeof(MEMORY) + m_objectSize;

        for (int i = 1; i < m_expandCount; i++) {
            pMemory->next = (MEMORY*) &buffer[pos];
            pMemory = pMemory->next;
            pMemory->pPool = pPool;
            pMemory->isAlloced = 0;
            pMemory->buffer = &buffer[sizeof(MEMORY) + pos];
            pMemory->pThis = this;
            pMemory->next = NULL;
            pos += sizeof(MEMORY) + m_objectSize;
        }

        //////////////////////////////////////////////////////////////////////////
        //
        m_header = (MEMORY*) buffer;
    }

    void FastMemoryPool::UniteFree() {
        POOL* pPool = m_poolHeader;
        MEMORY* pMemory = NULL;
        int i = 0;
        int pos = 0;
        for (; NULL != pPool; pPool = pPool->next) {
            if (pPool->freeCount == 0) continue;
            pos = 0;
            for (i = 0; i < m_expandCount; i++) {
                pMemory = (MEMORY*) &pPool->buffer[pos];
                if (0 == pMemory->isAlloced) {
                    pMemory->next = m_header;
                    m_header = pMemory;
                }
                pos += sizeof(MEMORY) + m_objectSize;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        //
        if (NULL == m_header) Expand();
        mdf_assert(NULL != m_header);
    }

    void* FastMemoryPool::Alloc() {
        uint64 tid = CurThreadId();
        int index = 0;
        FastMemoryPool* pPool = NULL;
        for (index = 0; index < m_threadCount; index++) {
            if (tid == m_poolIndex[index]) {
                pPool = m_poolForThread[index];
                break;
            }
        }
        if (NULL == pPool) {
            pPool = new FastMemoryPool(m_objectSize, m_expandCount, tid);
            if (NULL == pPool) mdf_assert(false); //系统内存用完

            index = AtomAdd(&m_threadCount, 1);
            m_poolIndex[index] = tid;
            m_poolForThread[index] = pPool;
        }
        mdf_assert(tid == pPool->m_ownerThreadId); //一出现并发立刻崩溃，确保其它地方的如果出错不用考虑并发可能
        return pPool->AllocMethod();
    }

    void* FastMemoryPool::AllocMethod() {
        if (NULL == m_header) UniteFree();
        MEMORY* pMemory = m_header;
        m_header = m_header->next;

        mdf_assert(0 == pMemory->isAlloced);
        mdf_assert(NULL != pMemory->buffer);
        pMemory->next = NULL;

        AtomAdd(&pMemory->pPool->freeCount, -1);
        pMemory->isAlloced = 1;

        memset(pMemory->buffer, 0, m_objectSize);
        return pMemory->buffer;
    }

    void FastMemoryPool::Free(void* pObject) {
        if (NULL == pObject) return;
        MEMORY* pMemory = (MEMORY*) ((char*) pObject - sizeof(MEMORY));
        if (0 != ((char*) pMemory - (char*) pMemory->pPool->buffer) % (sizeof(MEMORY) + pMemory->pThis->m_objectSize)) mdf_assert(false);//捕获用户对不属于内存池的内存调用Free
        mdf_assert(NULL == pMemory->next);
        if (1 != AtomAdd(&pMemory->isAlloced, -1)) mdf_assert(false); //捕获用户未配对的Alloc()/Free()调用
        AtomAdd(&pMemory->pPool->freeCount, 1);
    }

}
