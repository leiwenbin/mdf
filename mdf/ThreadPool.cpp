#include "../../include/mdf/ThreadPool.h"
#include "../../include/mdf/MemoryPool.h"

using namespace std;

namespace mdf {

    ThreadPool::ThreadPool() :
            m_nMinThreadNum(0), m_nThreadNum(0) {
        m_pTaskPool = new MemoryPool(sizeof(Task), 200);
        m_pContextPool = NULL;
    }

    ThreadPool::~ThreadPool() {
        Stop();
        if (NULL != m_pTaskPool) {
            delete m_pTaskPool;
            m_pTaskPool = NULL;
        }
        if (NULL != m_pContextPool) {
            delete m_pContextPool;
            m_pContextPool = NULL;
        }
    }

    void ThreadPool::SetOnStart(MethodPointer method, void* pObj, void* pParam) {
        m_taskOnStart.Accept(method, pObj, pParam);
    }

    void ThreadPool::SetOnStart(FuntionPointer fun, void* pParam) {
        m_taskOnStart.Accept(fun, pParam);
    }

    bool ThreadPool::Start(int nMinThreadNum) {
        if (nMinThreadNum < 0) return false;
        m_nMinThreadNum = nMinThreadNum;
        return CreateThread(m_nMinThreadNum);
    }

    bool ThreadPool::CreateThread(unsigned short nNum) {
        m_pContextPool = new MemoryPool(sizeof(THREAD_CONTEXT), nNum * 2);
        AutoLock lock(&m_threadsMutex);

        THREAD_CONTEXT* pContext;
        for (int i = 0; i < nNum; i++) {
            pContext = CreateContext();
            pContext->bIdle = true;
            pContext->bRun = true;
            pContext->thread.Run(Executor::Bind(&ThreadPool::ThreadFunc), this, pContext);
            m_threads.insert(threadMaps::value_type(pContext->thread.GetID(), pContext));
        }
        m_nThreadNum += nNum;
        return true;
    }

    THREAD_CONTEXT* ThreadPool::CreateContext() {
        //AutoLock lock(&m_contextPoolMutex);
        THREAD_CONTEXT* pContext = new(m_pContextPool->Alloc()) THREAD_CONTEXT;

        return pContext;
    }

    void ThreadPool::ReleaseContext(THREAD_CONTEXT* pContext) {
        //AutoLock lock(&m_contextPoolMutex);
        pContext->thread.~Thread();
        m_pContextPool->Free(pContext);
    }

    void ThreadPool::Stop() {
        AutoLock lockTask(&m_tasksMutex);
        m_tasks.clear(); //清空任务
        lockTask.Unlock();

        AutoLock lock(&m_threadsMutex);
        threadMaps::iterator it = m_threads.begin();
        //全部设为停止
        for (it = m_threads.begin(); it != m_threads.end(); it++)
            it->second->bRun = false;
        m_sigNewTask.Notify(); //通知1个线程停止，线程停止会通知其它线程停止
        //通知所有线程，停止
        for (it = m_threads.begin(); it != m_threads.end(); it++) {
            it->second->thread.Stop(3000);
            ReleaseContext(it->second);
        }
        m_threads.clear();
        m_nThreadNum = 0;
    }

    void ThreadPool::Accept(MethodPointer method, void* pObj, void* pParam) {
        Task* pTask = CreateTask();
        pTask->Accept(method, pObj, pParam);
        PushTask(pTask);
    }

    void ThreadPool::Accept(FuntionPointer fun, void* pParam) {
        Task* pTask = CreateTask();
        pTask->Accept(fun, pParam);
        PushTask(pTask);
    }

    Task* ThreadPool::CreateTask() {
        //AutoLock lock(&m_taskPoolMutex);
        Task* pTask = new(m_pTaskPool->Alloc()) Task;

        return pTask;
    }

    void ThreadPool::ReleaseTask(Task* pTask) {
        //AutoLock lock(&m_taskPoolMutex);
        pTask->~Task();
        m_pTaskPool->Free(pTask);
    }

    void ThreadPool::PushTask(Task* pTask) {
        //AutoLock lockThread(&m_threadsMutex);
        AutoLock lock(&m_tasksMutex);  //如果有线程正在对线程列队进行操作必须互斥
        m_tasks.push_back(pTask);
        m_sigNewTask.Notify();
    }

    Task* ThreadPool::PullTask() {
        Task* pTask = NULL;
        AutoLock lock(&m_tasksMutex);
        if (m_tasks.empty())
            return pTask;

        pTask = m_tasks.front();
        m_tasks.pop_front();

        return pTask;
    }

    void* ThreadPool::ThreadFunc(void* pParam) {
        m_taskOnStart.Execute();
        THREAD_CONTEXT* pContext = (THREAD_CONTEXT*) pParam;
        Task* pTask = NULL;
        while (pContext->bRun) {
            if (!pContext->bRun)
                break;    //外部停止
            pContext->bIdle = false;
            while (pContext->bRun) {
                pTask = PullTask();    //取得任务
                if (NULL == pTask)
                    break;
                pTask->Execute();    //执行任务
                ReleaseTask(pTask);
            }
            pContext->bIdle = true;
            if (!pContext->bRun)
                break;    //避免进入wait
            if (!m_sigNewTask.Wait())
                continue;    //等待任务
        }
        m_sigNewTask.Notify();    //通知其它线程停止
        return (void*) 0;
    }

    void ThreadPool::StopIdle() {
        int nThreadNum = m_nMinThreadNum * 2;
        AutoLock lock(&m_threadsMutex);
        threadMaps::iterator it = m_threads.begin();
        for (; m_nThreadNum > nThreadNum && it != m_threads.end(); it++) {
            if (!it->second->bIdle)
                continue;
            it->second->bRun = false;
            it->second->thread.Stop(3000);
            ReleaseContext(it->second);
            m_threads.erase(it);
            it = m_threads.begin();
            m_nThreadNum--;
        }

        return;
    }

    unsigned int ThreadPool::GetTaskCount() {
        AutoLock lock(&m_tasksMutex);
        return m_tasks.size();
    }

}    //namespace mdf
