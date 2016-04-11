// SRWLock.cpp: implementation of the SRWLock class.
//
//////////////////////////////////////////////////////////////////////

#include "../../include/mdf/SRWLock.h"
#include "../../include/mdf/atom.h"
#include "../../include/mdf/mapi.h"
#include <cstdio>
#include <cstring>

SRWLock::SRWLock() {
    int i = 0;
    for (; i < 2000; i++)
        memset(&m_threadData[i], 0, sizeof(THREAD_HIS));
    m_readCount = 0;
    m_writeCount = 0;
    m_ownerThreadId = 0;
    m_bWaitRead = false;
    m_waitWriteCount = 0;
}

SRWLock::~SRWLock() {
}

SRWLock::THREAD_HIS* SRWLock::GetThreadData(bool lock) {
    mdf::uint64 curTID = mdf::CurThreadId();
    int i = 0;
    for (; i < MAX_THREAD_COUNT; i++) {
        if (0 == m_threadData[i].threadID) {
            if (!lock)
                return NULL;
            if (0 != mdf::AtomAdd(&m_threadData[i].threadID, 1))
                continue;
            m_threadData[i].threadID = curTID;
            return &m_threadData[i];
        }

        if (curTID != m_threadData[i].threadID)
            continue;
        return &m_threadData[i];
    }

    return NULL;
}

void SRWLock::Lock() {
    THREAD_HIS* threadOp = GetThreadData(true);
    char filename[256];
    sprintf(filename, "./log/%ld.txt", threadOp->threadID);
    FILE* fp = fopen(filename, "a");
    if (NULL == fp) {
        fp = fopen(filename, "w");
    }
    fclose(fp);
    fp = fopen(filename, "a");
    fprintf(fp, "Lock()：%d读 %d写\n", threadOp->readCount, threadOp->writeCount);
    fclose(fp);
    if (0 < threadOp->writeCount) {
        mdf::AtomAdd(&m_writeCount, 1);
        threadOp->writeCount++;
        fp = fopen(filename, "a");
        fprintf(fp, "写嵌套Lock()，直接return\n");
        fclose(fp);
        return;
    }

    m_checkLock.Lock();
    mdf::AtomAdd(&m_writeCount, 1);
    if (m_ownerThreadId == threadOp->threadID) {
        if (m_readCount == threadOp->readCount) {
            threadOp->writeCount++;
            m_checkLock.Unlock();
            fp = fopen(filename, "a");
            fprintf(fp, "读嵌套Lock()，直接return\n");
            fclose(fp);
            return;
        }

        mdf::AtomDec(&m_readCount, threadOp->readCount);
        m_bWaitRead = true;
        m_checkLock.Unlock();
        fp = fopen(filename, "a");
        fprintf(fp, "Lock():wait读完成\n");
        fclose(fp);
        m_waitRead.Wait(-1);
        m_checkLock.Lock();
    } else {
        if (threadOp->readCount == mdf::AtomDec(&m_readCount, threadOp->readCount)) {
            if (0 < threadOp->readCount) {
                mdf::AtomAdd(&m_readCount, threadOp->readCount);
                threadOp->writeCount++;
                m_checkLock.Unlock();
                return;
            }
        }
    }
// 	else
// 	{
// 		if ( threadOp->readCount == mdf::AtomDec(&m_readCount, threadOp->readCount) )
// 		{
// 			if ( m_bWaitRead )
// 			{
// 				m_bWaitRead = false;
// 				m_waitRead.Notify();
// 				fp = fopen( filename, "a" );
// 				fprintf( fp, "Lock().唤醒写\n" );
// 				fclose(fp);
// 			}
// 			else if ( 0 < threadOp->readCount )
// 			{
// 				mdf::AtomAdd(&m_readCount, threadOp->readCount);
// 				threadOp->writeCount++;
// 				m_checkLock.Unlock();
// 				return;
// 			}
// 		}
// 	}
    m_checkLock.Unlock();

    fp = fopen(filename, "a");
    fprintf(fp, "Lock():wait锁\n");
    fclose(fp);
    m_lock.Lock();
    fp = fopen(filename, "a");
    fprintf(fp, "Lock():入锁\n");
    fclose(fp);
    mdf::AtomAdd(&m_readCount, threadOp->readCount);
    m_ownerThreadId = threadOp->threadID;
    threadOp->writeCount++;
    return;
}

void SRWLock::Unlock() {
    THREAD_HIS* threadOp = GetThreadData(true);
    mdf::mdf_assert(NULL != threadOp);
    threadOp->writeCount--;
    char filename[256];
    sprintf(filename, "./log/%ld.txt", threadOp->threadID);
    FILE* fp = fopen(filename, "a");
    if (NULL == fp) {
        fp = fopen(filename, "w");
    }
    fclose(fp);
    fp = fopen(filename, "a");
    fprintf(fp, "Unlock()：%d读 %d写\n", threadOp->readCount, threadOp->writeCount);
    fclose(fp);

    m_checkLock.Lock();
    mdf::AtomDec(&m_writeCount, 1);
    if (0 == threadOp->writeCount) {
        fp = fopen(filename, "a");
        fprintf(fp, "唤醒%d读\n", m_waitWriteCount);
        fclose(fp);
        for (; m_waitWriteCount > 0; m_waitWriteCount--)
            m_waitWrite.Notify();
    }
    if (0 < threadOp->writeCount || 0 < threadOp->readCount) {
        fp = fopen(filename, "a");
        fprintf(fp, "嵌套忽略\n");
        fclose(fp);
        m_checkLock.Unlock();
        return;
    }
    m_ownerThreadId = 0;
    m_checkLock.Unlock();
    m_lock.Unlock();
    fp = fopen(filename, "a");
    fprintf(fp, "Unlock()：出锁\n");
    fclose(fp);
    /*
     if ( m_bWaitRead )
     {
     m_bWaitRead = false;
     m_waitRead.Notify();
     fp = fopen( filename, "a" );
     fprintf( fp, "唤醒写\n" );
     fclose(fp);
     }
     */

    return;
}

void SRWLock::ShareLock() {
    THREAD_HIS* threadOp = GetThreadData(true);
    char filename[256];
    sprintf(filename, "./log/%ld.txt", threadOp->threadID);
    FILE* fp = fopen(filename, "a");
    if (NULL == fp) {
        fp = fopen(filename, "w");
    }
    fclose(fp);
    fp = fopen(filename, "a");
    fprintf(fp, "ShareLock()：%d读 %d写\n", threadOp->readCount, threadOp->writeCount);
    fclose(fp);

    if (0 < threadOp->writeCount || 0 < threadOp->readCount) {
        fp = fopen(filename, "a");
        fprintf(fp, "嵌套，直接return\n");
        fclose(fp);
        threadOp->readCount++;
        mdf::AtomAdd(&m_readCount, 1);
        return;
    }

    m_checkLock.Lock();
    while (0 < m_writeCount) {
        m_waitWriteCount++;
        m_checkLock.Unlock();
        fp = fopen(filename, "a");
        fprintf(fp, "ShareLock():Wait写完成\n");
        fclose(fp);
        m_waitWrite.Wait(-1);
        m_checkLock.Lock();
    }

    if (0 < mdf::AtomAdd(&m_readCount, 1)) {
        threadOp->readCount++;
        m_checkLock.Unlock();
        fp = fopen(filename, "a");
        fprintf(fp, "读嵌套,直接return\n");
        fclose(fp);
        return;
    }
    fp = fopen(filename, "a");
    fprintf(fp, "ShareLock():Wait锁\n");
    fclose(fp);
    m_lock.Lock();
    fp = fopen(filename, "a");
    fprintf(fp, "ShareLock():入锁\n");
    fclose(fp);
    m_ownerThreadId = threadOp->threadID;
    threadOp->readCount++;
    m_checkLock.Unlock();

    return;
}

void SRWLock::ShareUnlock() {
    THREAD_HIS* threadOp = GetThreadData(true);
    mdf::mdf_assert(NULL != threadOp);
    threadOp->readCount--;
    char filename[256];
    sprintf(filename, "./log/%ld.txt", threadOp->threadID);
    FILE* fp = fopen(filename, "a");
    if (NULL == fp) {
        fp = fopen(filename, "w");
    }
    fclose(fp);
    fp = fopen(filename, "a");
    fprintf(fp, "ShareUnlock()：%d读 %d写\n", threadOp->readCount, threadOp->writeCount);
    fclose(fp);

    m_checkLock.Lock();
    if (1 != mdf::AtomDec(&m_readCount, 1)) {
        m_checkLock.Unlock();
        fp = fopen(filename, "a");
        fprintf(fp, "读未完成，忽略ShareUnlock\n");
        fclose(fp);
        return;
    }

    if (0 < threadOp->writeCount) {
        m_checkLock.Unlock();
        fp = fopen(filename, "a");
        fprintf(fp, "写未完成，忽略ShareUnlock\n");
        fclose(fp);
        return;
    }
    if (m_bWaitRead) {
        m_bWaitRead = false;
        m_waitRead.Notify();
        fp = fopen(filename, "a");
        fprintf(fp, "唤醒写\n");
        fclose(fp);
    }
    fp = fopen(filename, "a");
    fprintf(fp, "ShareUnlock():出锁\n");
    fclose(fp);
    m_ownerThreadId = 0;
    m_lock.Unlock();
    m_checkLock.Unlock();
}
