#include "../../include/mdf/String.h"
#include <cstdio>
#include <string>

namespace mdf {

    FastMemoryPool String::ms_pool(sizeof(String::Data) + 4096, 500);

    String::String(void) :
            m_data(NULL) {
        m_data = new(String::ms_pool.Alloc()) Data();
        m_data->m_str = (char*) m_data;
        m_data->m_str += sizeof(String::Data);
    }

    String::String(const String& right) :
            m_data(NULL) {
        *this = right;
    }

    String::~String(void) {
        m_data->Release();
    }

    void String::Data::Delete() {
        String::ms_pool.Free(this);
    }

    String& String::operator=(const String& right) {
        if (m_data == right.m_data)
            return *this;
        right.m_data->AddRef();
        m_data->Release();
        m_data = right.m_data;

        return *this;
    }

    String::operator char*() {
        if (NULL == m_data)
            return NULL;
        return m_data->m_str;
    }

    String::operator unsigned char*() {
        if (NULL == m_data)
            return NULL;
        return (unsigned char*) m_data->m_str;
    }

    char* String::c_str() {
        if (NULL == m_data)
            return NULL;
        return m_data->m_str;
    }

}
