#include "../../include/mdf/ShareObject.h"
#include "../../include/mdf/atom.h"
#include <cstdio>

namespace mdf {

    ShareObject::ShareObject() {
        m_refCount = 1;
    }

    ShareObject::~ShareObject() {

    }

    void ShareObject::AddRef() {
        if (NULL == this)
            return;
        mdf::AtomAdd(&m_refCount, 1);
    }

    void ShareObject::Release() {
        if (NULL == this)
            return;
        if (1 != mdf::AtomDec(&m_refCount, 1))
            return;
        Delete();
    }

    void ShareObject::Delete() {
        delete this;
    }

}
