#include "../../include/mdf/FinishedTime.h"
#include "../../include/mdf/mapi.h"

namespace mdf {

    FinishedTime::FinishedTime(MethodPointer method, void* pObj) {
        m_finished = false;
        m_task.Accept(method, pObj, this);
        m_start = MillTime();
        m_useTime = 0;
        m_end = 0;
    }

    FinishedTime::FinishedTime(FuntionPointer fun) {
        m_finished = false;
        m_task.Accept(fun, this);
        m_start = MillTime();
        m_useTime = 0;
        m_end = 0;
    }

    FinishedTime::~FinishedTime() {
        Finished();
    }

    mdf::uint32 FinishedTime::UseTime() {
        return m_useTime;
    }

    void FinishedTime::Finished() {
        if (m_finished) return;
        m_finished = true;
        m_end = MillTime();
        m_useTime = (mdf::uint32) (m_end - m_start);
        m_task.Execute();
    }

}
