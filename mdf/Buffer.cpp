#include "../../include/mdf/Buffer.h"
#include <cstdio>
#include <cstring>

namespace mdf {

    FastMemoryPool Buffer::ms_pool(sizeof(mdf::Buffer::LIST_NODE), 100);

    Buffer::Buffer() {
        m_usePool = true;
        m_pHeader = m_pTail = NULL;
        m_sumSize = 0;
    }

    Buffer::Buffer(bool usePool) {
        m_usePool = usePool;
        m_pHeader = m_pTail = NULL;
        m_sumSize = 0;
    }

    Buffer::~Buffer() {
    }

    void Buffer::Clear() {
        LIST_NODE* pDelete = NULL;
        while (NULL != m_pHeader) {
            pDelete = m_pHeader = m_pHeader->pNext;
            if (m_usePool)
                Buffer::ms_pool.Free(pDelete);
            else
                delete pDelete;
        }
    }

    int Buffer::Size() {
        return m_sumSize;
    }

    void Buffer::AddData(unsigned char* pData, int size) {
        mdf::AutoLock lock(&m_lock);
        if (NULL == m_pHeader) {
            //增加结点
            if (m_usePool)
                m_pHeader = (LIST_NODE*) Buffer::ms_pool.Alloc();
            else
                m_pHeader = new LIST_NODE;
            if (NULL == m_pHeader) {
                return;
            }
            m_pTail = m_pHeader;
            m_pHeader->pNext = NULL;
            m_pHeader->pos = 0;
            m_pHeader->size = 0;
        }

        int saveSize = 0;
        int pos = 0;
        while (size > 0) {
            //将剩余数据存入链表尾
            if (m_pTail->size + size <= EXPAND_SIZE) //不用扩大链表
            {
                memcpy(&m_pTail->pData[m_pTail->size], &pData[pos], size);
                m_pTail->size += size;
                pos += size;
                m_sumSize += size;
                break;
            }
            //先将末尾结点填满，记录剩余数据长度
            saveSize = EXPAND_SIZE - m_pTail->size;
            memcpy(&m_pTail->pData[m_pTail->size], &pData[pos], saveSize);
            m_pTail->size += saveSize;
            size -= saveSize;
            pos += saveSize;
            m_sumSize += saveSize;

            //增加结点
            if (m_usePool)
                m_pTail->pNext = (LIST_NODE*) Buffer::ms_pool.Alloc();
            else
                m_pTail->pNext = new LIST_NODE;
            if (NULL == m_pTail->pNext) {
                return;
            }
            m_pTail = m_pTail->pNext;
            m_pTail->pNext = NULL;
            m_pTail->pos = 0;
            m_pTail->size = 0;
        }
    }

    bool Buffer::GetData(unsigned char* pData, int size, bool checkData) {
        mdf::AutoLock lock(&m_lock);
        if (m_sumSize < size) return false;
        if (!checkData) m_sumSize -= size;

        int getSize = 0;
        int pos = 0;
        LIST_NODE* pHeader = m_pHeader;
        while (true) {
            //将剩余数据取出
            if (pHeader->size - pHeader->pos >= size) //不用从下一个结点取数据
            {
                memcpy(&pData[pos], &pHeader->pData[pHeader->pos], size);
                if (!checkData) pHeader->pos += size;
                break;
            }
            //先将表头剩余数据长度全部读出
            getSize = pHeader->size - pHeader->pos;
            memcpy(&pData[pos], &pHeader->pData[pHeader->pos], getSize);
            if (!checkData) pHeader->pos += getSize;
            pos += getSize;
            size -= getSize;

            pHeader = pHeader->pNext;
            if (!checkData) {
                if (m_usePool)
                    Buffer::ms_pool.Free(m_pHeader);
                else
                    delete m_pHeader;
                m_pHeader = pHeader;
            }
        }

        return true;
    }

    bool Buffer::Seek(int size) {
        mdf::AutoLock lock(&m_lock);
        if (m_sumSize < size) return false;
        m_sumSize -= size; //跳过数据

        int seekSize = 0; //跳过长度
        LIST_NODE* pHeader = m_pHeader;
        while (true) {
            if (pHeader->size - pHeader->pos >= size) //跳过长度<当前节点剩余数据，在节点内跳过
            {
                pHeader->pos += size;
                break;
            }
            //跳过长度>当前节点剩余数据，跳过整个节点剩余数据
            seekSize = pHeader->size - pHeader->pos; //剩余数据
            pHeader->pos += seekSize; //跳过
            size -= seekSize;

            pHeader = pHeader->pNext;
            if (m_usePool)
                Buffer::ms_pool.Free(m_pHeader);
            else
                delete m_pHeader;
            m_pHeader = pHeader;
        }

        return true;
    }

}
