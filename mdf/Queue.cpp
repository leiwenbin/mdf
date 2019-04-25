// Queue.cpp: implementation of the Queue class.
//
//////////////////////////////////////////////////////////////////////

#include "../../include/mdf/Queue.h"
#include "../../include/mdf/atom.h"
#include <stdio.h>
#include <vector>

#ifndef NULL
#define NULL 0
#endif
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
namespace mdf {

    Queue::Queue(int nSize) {
        m_nSize = nSize;
        m_nWriteAbleCount = m_nSize;
        m_nReadAbleCount = 0;
        m_queue = new QUEUE_NODE[m_nSize];
        m_push = 0;
        m_pop = 0;
        Clear();
    }

    Queue::~Queue() {
        if (NULL == m_queue)
            return;
        delete[] m_queue;
    }

    bool Queue::Push(void* pObject) {
        if (NULL == pObject)
            return false;
        if (0 >= m_nWriteAbleCount)
            return false; //列队已满
        if (0 >= AtomDec(&m_nWriteAbleCount, 1)) //已经有足够多的push操作向列队不同位置写入数据
        {
            AtomAdd(&m_nWriteAbleCount, 1);
            return false;
        }
        uint32 pushPos = AtomAdd(&m_push, 1);
        pushPos = pushPos % m_nSize;
        /*
         只有在NPop并发情况下，因Pop无序完成，第一个位置的Pop未完成，
         后面的Pop就先完成提示有空位，导致这里检查第一个位置时，发现数据未被取走
         因为该类只允许1对N，所以必然是单线程Push( void *pObject )，所以条件内m_push不需要并发控制
         */
        if (!m_queue[pushPos].IsEmpty) {
            m_push--;
            AtomAdd(&m_nWriteAbleCount, 1);
            return false;
        }
        m_queue[pushPos].pObject = pObject;
        m_queue[pushPos].IsEmpty = false;
        AtomAdd(&m_nReadAbleCount, 1);

        return true;
    }

    void* Queue::Pop() {
        if (0 >= m_nReadAbleCount)
            return NULL; //空列队
        if (0 >= AtomDec(&m_nReadAbleCount, 1)) //已经有足够多的pop操作读取列队不同位置的数据
        {
            AtomAdd(&m_nReadAbleCount, 1);
            return NULL;
        }
        uint32 popPos = AtomAdd(&m_pop, 1);
        popPos = popPos % m_nSize;
        /*
         只有在NPush并发情况下，因Push无序完成，第一个位置的Push未完成，
         后面的Push就先完成提示有数据，导致这里检查第一个位置时，发现没有数据可读
         因为该类只允许1对N，必然是单线程Pop()，所以条件内m_pop不需要并发控制
         */
        if (m_queue[popPos].IsEmpty) {
            m_pop--;
            AtomAdd(&m_nReadAbleCount, 1);
            return NULL;
        }
        void* pObject = m_queue[popPos].pObject;
        m_queue[popPos].pObject = NULL;
        m_queue[popPos].IsEmpty = true;
        AtomAdd(&m_nWriteAbleCount, 1);

        return pObject;
    }

    void Queue::Clear() {
        if (NULL == m_queue)
            return;
        uint32 i = 0;
        m_nWriteAbleCount = m_nSize;
        m_nReadAbleCount = 0;
        for (i = 0; i < m_nSize; i++) {
            m_queue[i].pObject = NULL;
            m_queue[i].IsEmpty = true;
        }
    }

} //mdf_QUEUE_H
