#include "../../include/mdf/atom.h"

#ifdef WIN32
#include <windows.h>
#endif

namespace mdf {

//自增,返回新值
    unsigned int AtomSelfAdd(void* var) {
#ifdef WIN32
        return InterlockedIncrement((long *)(var)); // NOLINT
#else
        return __sync_add_and_fetch((uint32*) (var), 1); // NOLINT
#endif
    }

//自减,返回新值
    unsigned int AtomSelfDec(void* var) {
#ifdef WIN32
        return InterlockedDecrement((long *)(var)); // NOLINT
#else
        return __sync_add_and_fetch((uint32*) (var), -1); // NOLINT
#endif
    }

//加一个值,返回旧值
    unsigned int AtomAdd(void* var, const unsigned int value) {
#ifdef WIN32
        return InterlockedExchangeAdd((long *)(var), value); // NOLINT
#else
        return __sync_fetch_and_add((uint32*) (var), value); // NOLINT
#endif
    }

//减一个值,返回旧值
    unsigned int AtomDec(void* var, int value) {
        value = value * -1;
#ifdef WIN32
        return InterlockedExchangeAdd((long *)(var), value); // NOLINT
#else
        return __sync_fetch_and_add((uint32*) (var), value); // NOLINT
#endif
    }

//赋值,windows下返回新值，linux下返回旧值
    unsigned int AtomSet(void* var, const unsigned int value) {
#ifdef WIN32
        ::InterlockedExchange((long *)(var), (long)(value)); // NOLINT
#else
        __sync_lock_test_and_set((uint32*) (var), value); // NOLINT
#endif
        return value;
    }

//取值
    unsigned int AtomGet(void* var) {
#ifdef WIN32
        return InterlockedExchangeAdd((long *)(var), 0); // NOLINT
#else
        return __sync_fetch_and_add((uint32*) (var), 0); // NOLINT
#endif
    }

} //namespace mdf
