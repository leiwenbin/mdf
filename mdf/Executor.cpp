﻿// Executor.cpp: implementation of the Executor class.
//
//////////////////////////////////////////////////////////////////////

#include "../../include/mdf/Executor.h"

namespace mdf {

//__thiscall调用方式成员函数,做线程函数,指针定义
    typedef void* (__stdcall* ThisCallFun)(void*);

//__stdcall调用方式成员函数,做线程函数,指针定义
    typedef void* (__stdcall* StdCallFun)(void*, void*);

//执行声明为void* fun(void*)的成员函数
    void* WinCall(MethodPointer uMethod, void* pObj, void* pParam) {
#ifdef WIN32
        ThisCallFun method = (ThisCallFun) uMethod;
        unsigned long This = (unsigned long) pObj;
    __asm //准备this指针.
    {
        mov ecx, This;
    }
          return method(pParam);
#else//linux下汇编调用暂未实现
        return NULL;
#endif
    }

//执行声明为void* _stdcall fun(void*)的成员函数
    void* LinuxCall(MethodPointer uMethod, void* pObj, void* pParam) {
        StdCallFun method = (StdCallFun) uMethod;
        return method(pObj, pParam);
    }

    Executor::Executor() {
    }

    Executor::~Executor() {
    }

//执行声明为void* fun(void*)的成员函数
    void* Executor::CallMethod(MethodPointer method, void* pObj, void* pParam) {
#ifdef WIN32
        return LinuxCall( method, pObj, pParam );
#else
        return LinuxCall(method, pObj, pParam);
#endif
    }

} //namespace mdf
