#include "../../include/mdf/MultiChanThreadPool.h"
#include "../../include/mdf/MemoryPool.h"
#include "../../include/mdf/mapi.h"

using namespace std;

namespace mdf {

    MultiChanThreadPool::MultiChanThreadPool() :
            m_nMinThreadNum(0), m_nThreadNum(0) {
        m_pTaskPool = new MemoryPool(sizeof(Task), 200);
        m_pContextPool = NULL;
    }

    MultiChanThreadPool::~MultiChanThreadPool() {
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

    void MultiChanThreadPool::SetOnStart(MethodPointer method, void* pObj, void* pParam) {
        m_taskOnStart.Accept(method, pObj, pParam);
    }

    void MultiChanThreadPool::SetOnStart(FuntionPointer fun, void* pParam) {
        m_taskOnStart.Accept(fun, pParam);
    }

    bool MultiChanThreadPool::Start(int nMinThreadNum) {
        if (nMinThreadNum < 0) return false;
        m_nMinThreadNum = nMinThreadNum;
        std::list<Task*> chanTasks;
        m_tasks.resize(nMinThreadNum, chanTasks);
        Signal taskSignal;
        m_sigNewTask.resize(nMinThreadNum, taskSignal);
        return CreateThread(nMinThreadNum);
    }

    bool MultiChanThreadPool::CreateThread(unsigned short nNum) {
        if (m_tasks.size() < nNum) return false;
        m_pContextPool = new MemoryPool(sizeof(MULTI_CHAN_THREAD_CONTEXT), nNum * 2);
        AutoLock lock(&m_threadsMutex);

        MULTI_CHAN_THREAD_CONTEXT* pContext;
        for (int i = 0; i < nNum; i++) {
            pContext = CreateContext();
            pContext->bIdle = true;
            pContext->bRun = true;
            pContext->tasks = &(m_tasks.at(i));
            pContext->pSignal = &(m_sigNewTask.at(i));
            pContext->thread.Run(Executor::Bind(&MultiChanThreadPool::ThreadFunc), this, pContext);
            m_threads.insert(multiChanThreadMaps::value_type(pContext->thread.GetID(), pContext));
        }
        m_nThreadNum += nNum;
        return true;
    }

    MULTI_CHAN_THREAD_CONTEXT* MultiChanThreadPool::CreateContext() {
        //AutoLock lock(&m_contextPoolMutex);
        MULTI_CHAN_THREAD_CONTEXT* pContext = new(m_pContextPool->Alloc()) MULTI_CHAN_THREAD_CONTEXT;
        pContext->tasks = NULL;
        pContext->pSignal = NULL;

        return pContext;
    }

    void MultiChanThreadPool::ReleaseContext(MULTI_CHAN_THREAD_CONTEXT* pContext) {
        //AutoLock lock(&m_contextPoolMutex);
        pContext->thread.~Thread();
        m_pContextPool->Free(pContext);
    }

    void MultiChanThreadPool::Stop() {
        AutoLock lockTask(&m_tasksMutex);
        for (unsigned int i = 0; i < m_tasks.size(); ++i) {
            m_tasks.at(i).clear();
        }
        lockTask.Unlock();

        AutoLock lock(&m_threadsMutex);
        multiChanThreadMaps::iterator it = m_threads.begin();
        int i = 0;
        //全部设为停止
        for (i = 0, it = m_threads.begin(); it != m_threads.end(); it++, ++i) {
            it->second->bRun = false;
            m_sigNewTask.at(i).Notify();
        }
        //通知所有线程，停止
        for (it = m_threads.begin(); it != m_threads.end(); it++) {
            it->second->thread.Stop(3000);
            ReleaseContext(it->second);
        }
        m_threads.clear();
        m_nThreadNum = 0;
    }

    void MultiChanThreadPool::Accept(std::string& strIndex, MethodPointer method, void* pObj, void* pParam) {
        Task* pTask = CreateTask();
        pTask->Accept(method, pObj, pParam);
        unsigned int uiIndex = mdf::APHash(strIndex.c_str());
        PushTask(pTask, uiIndex);
    }

    void MultiChanThreadPool::Accept(std::string& strIndex, FuntionPointer fun, void* pParam) {
        Task* pTask = CreateTask();
        pTask->Accept(fun, pParam);
        unsigned int uiIndex = mdf::APHash(strIndex.c_str());
        PushTask(pTask, uiIndex);
    }

    Task* MultiChanThreadPool::CreateTask() {
        //AutoLock lock(&m_taskPoolMutex);
        Task* pTask = new(m_pTaskPool->Alloc()) Task;
        return pTask;
    }

    void MultiChanThreadPool::ReleaseTask(Task* pTask) {
        //AutoLock lock(&m_taskPoolMutex);
        pTask->~Task();
        m_pTaskPool->Free(pTask);
    }

    void MultiChanThreadPool::PushTask(Task* pTask, unsigned int uiIndex) {
        unsigned short id = uiIndex % m_nThreadNum;
        AutoLock lock(&m_tasksMutex);  //如果有线程正在对线程列队进行操作必须互斥
        m_tasks.at(id).push_back(pTask);
        m_sigNewTask.at(id).Notify();
    }

    Task* MultiChanThreadPool::PullTask(std::list<Task*>* pTasks) {
        Task* pTask = NULL;
        AutoLock lock(&m_tasksMutex);
        if (pTasks->empty())
            return pTask;

        pTask = pTasks->front();
        pTasks->pop_front();

        return pTask;
    }

    void* MultiChanThreadPool::ThreadFunc(void* pParam) {
        m_taskOnStart.Execute();
        MULTI_CHAN_THREAD_CONTEXT* pContext = (MULTI_CHAN_THREAD_CONTEXT*) pParam;
        Task* pTask = NULL;
        while (pContext->bRun) {
            if (!pContext->bRun)
                break;    //外部停止
            pContext->bIdle = false;
            while (pContext->bRun) {
                pTask = PullTask(pContext->tasks);    //取得任务
                if (NULL == pTask)
                    break;
                pTask->Execute();    //执行任务
                ReleaseTask(pTask);
            }
            pContext->bIdle = true;
            if (!pContext->bRun)
                break;    //避免进入wait
            if (!pContext->pSignal->Wait())
                continue;    //等待任务
        }
        pContext->pSignal->Notify();    //通知其它线程停止
        return (void*) 0;
    }

    void MultiChanThreadPool::StopIdle() {
        int nThreadNum = m_nMinThreadNum * 2;
        AutoLock lock(&m_threadsMutex);
        multiChanThreadMaps::iterator it = m_threads.begin();
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

    unsigned int MultiChanThreadPool::GetTaskCount(unsigned short usIndex) {
        AutoLock lock(&m_tasksMutex);
        if (usIndex >= m_tasks.size()) return 0;
        return m_tasks.at(usIndex).size();
    }

}    //namespace mdf
