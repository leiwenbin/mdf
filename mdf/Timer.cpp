// Timer.cpp: implementation of the Timer class.
//
//////////////////////////////////////////////////////////////////////

#include "../../include/mdf/Timer.h"
#include <ctime>
#include <vector>
#include "../../include/mdf/atom.h"
#include <cstring>
#include <cstdio>
#include "../../include/mdf/ShareObject.h"

#ifdef WIN32
#include "windows.h"
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
namespace mdf {
    Timer::Timer() :
            m_threadId(0) {
        if (m_worker.Run(Executor::Bind(&Timer::Work), this, NULL)) {
            m_sigRun.Wait();
        }
    }

    Timer::~Timer() {

    }

    void Timer::SetTimer(int eventId, uint64 second, MethodPointer method, void* pObj, ShareObject* pSafeObj, void* pData, int dataSize, int repeat) {
        KillTimer(eventId);
        TASK* pTask = new TASK;
        pTask->ref = 1;
        pTask->eventId = eventId;
        pTask->lastTime = (uint64) time(NULL);
        pTask->millSecond = second * 1000;
        pTask->repeat = repeat;
        pTask->state = 0;
        if (NULL == pSafeObj)
            pTask->pObj = (ShareObject*) pObj;
        else
            pTask->pObj = pSafeObj;
        pTask->pObj->AddRef();
        pTask->dataSize = dataSize;
        pTask->pData = NULL;
        if (pTask->dataSize > 0) {
            pTask->pData = new char[pTask->dataSize];
            memcpy(pTask->pData, pData, pTask->dataSize);
        }
        pTask->worker.Accept(method, pObj, pTask->pData);

        AutoLock lock(&m_lock);
        m_waitTasks.insert(std::map<int, TASK*>::value_type(eventId, pTask));

        return;
    }

    void Timer::SetTimer(int eventId, uint64 second, FuntionPointer fun, void* pData, int dataSize, int repeat) {
        KillTimer(eventId);
        TASK* pTask = new TASK;
        pTask->ref = 1;
        pTask->eventId = eventId;
        pTask->lastTime = (uint64) time(NULL);
        pTask->millSecond = second * 1000;
        pTask->repeat = repeat;
        pTask->state = 0;
        pTask->pObj = NULL;
        pTask->dataSize = dataSize;
        pTask->pData = NULL;
        if (pTask->dataSize > 0) {
            pTask->pData = new char[pTask->dataSize];
            memcpy(pTask->pData, pData, pTask->dataSize);
        }
        pTask->worker.Accept(fun, pTask->pData);

        AutoLock lock(&m_lock);
        m_waitTasks.insert(std::map<int, TASK*>::value_type(eventId, pTask));

        return;
    }

    void Timer::KillTimer(int eventId) {
        uint64 threadId = CurThreadId();
        TASK* pTask = NULL;
        {
            AutoLock lock(&m_lock);
            std::map<int, TASK*>::iterator it = m_waitTasks.find(eventId);
            if (m_waitTasks.end() == it)
                return;
            pTask = it->second;
            m_waitTasks.erase(it);
        }
        if (threadId != m_threadId) {
            AutoLock lock(&pTask->lock);
            pTask->state = 2;
        }
        pTask->Release();
        return;
    }

    void Timer::TASK::Release() {
        if (1 != AtomDec(&ref, 1)) {
            return;
        }
        pObj->Release();
        if (NULL != pData) {
            char* pDel = (char*) pData;
            MDF_SAFE_DELETE_ARRAY(pDel)
            pData = NULL;
        }

        delete this;
    }

    void Timer::KillAllTimer() {
        std::vector<int> allIds;
        { //所有id全部复制到allIds中
            AutoLock lock(&m_lock);
            std::map<int, TASK*>::iterator it = m_waitTasks.begin();
            while (it != m_waitTasks.end()) {
                allIds.push_back(it->first);
                it++;
            }
        }

        //逐个调用KillTimer
        unsigned int i = 0;
        for (; i < allIds.size(); i++) {
            KillTimer(allIds[i]);
        }

        return;
    }

#ifdef WIN32
#else

    static uint64 GetTickCount() {
        struct timespec ts;

        clock_gettime(CLOCK_MONOTONIC, &ts);

        return (uint64) (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    }

#endif

    void* Timer::Work(void* param) {
        m_threadId = CurThreadId();
        m_sigRun.Notify();
        uint64 curtime;
        std::vector<TASK*> waitTasks;
        while (true) {
            waitTasks.clear();
            curtime = GetTickCount();
            { //到时间的任务全部保存到waitTasks里，解锁
                AutoLock lock(&m_lock);
                std::map<int, TASK*>::iterator it = m_waitTasks.begin();
                while (it != m_waitTasks.end()) {
                    if (curtime - it->second->lastTime < it->second->millSecond) {
                        it++;
                        continue;
                    }
                    it->second->lastTime = curtime;
                    AtomAdd(&it->second->ref, 1);
                    waitTasks.push_back(it->second);
                    if (0 < it->second->repeat) {
                        if (1 == it->second->repeat) {
                            it->second->Release();
                            m_waitTasks.erase(it++);
                        } else
                            it->second->repeat--;
                    } else
                        it++;
                }
            }
            { //逐个任务执行
                for (uint32 i = 0; i < waitTasks.size(); i++) {
                    {
                        AutoLock lock(&waitTasks[i]->lock); //不阻塞SetTimer线程，只阻塞部分KillTimer线程
                        if (2 != waitTasks[i]->state) {
                            waitTasks[i]->worker.Execute();
                        }
                    }
                    waitTasks[i]->Release();
                }
            }
            m_sleep(1000);
        }

        return NULL;
    }

}
