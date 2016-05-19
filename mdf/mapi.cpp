// mapi.cpp: implementation of the mapi class.
//
//////////////////////////////////////////////////////////////////////

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "../../include/mdf/mapi.h"

#ifdef WIN32
#include <windows.h>
#include <sys/stat.h> 
#else

#include <unistd.h>
#include <sys/stat.h> 

#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

using namespace std;

namespace mdf {

    void mdf_assert(bool isTrue) {
        if (isTrue) return;
        char* p = NULL;
        *p = 1;
        exit(0);
    }

    void m_sleep(long lMillSecond) {
#ifndef WIN32
        usleep(lMillSecond * 1000);
#else
        Sleep( lMillSecond );
#endif
    }

    bool addrToI64(uint64& addr64, const char* ip, int port) {
        unsigned char addr[8];
        int nIP[4];
        sscanf(ip, "%d.%d.%d.%d", &nIP[0], &nIP[1], &nIP[2], &nIP[3]);
        addr[0] = nIP[0];
        addr[1] = nIP[1];
        addr[2] = nIP[2];
        addr[3] = nIP[3];
        char checkIP[25];
        sprintf(checkIP, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
        if (0 != strcmp(checkIP, ip)) return false;
        memcpy(&addr[4], &port, 4);
        memcpy(&addr64, addr, 8);

        return true;
    }

    void i64ToAddr(char* ip, int& port, uint64 addr64) {
        unsigned char addr[8];
        memcpy(addr, &addr64, 8);
        sprintf(ip, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
        memcpy(&port, &addr[4], 4);
    }

    void TrimString(string& str, string del) {
        int nPos = 0;
        unsigned int i = 0;
        for (; i < del.size(); i++) {
            while (true) {
                nPos = str.find(del.c_str()[i], 0);
                if (-1 == nPos) break;
                str.replace(str.begin() + nPos, str.begin() + nPos + 1, "");
            }
        }
    }

    void TrimStringLeft(string& str, string del) {
        unsigned int i = 0;
        bool bTrim = false;
        for (; i < str.size(); i++) {
            if (string::npos == del.find(str.c_str()[i])) break;
            bTrim = true;
        }
        if (!bTrim) return;
        str.replace(str.begin(), str.begin() + i, "");
    }

    void TrimStringRight(string& str, string del) {
        int i = str.size() - 1;
        bool bTrim = false;
        for (; i >= 0; i--) {
            if (string::npos == del.find(str.c_str()[i])) break;
            bTrim = true;
        }
        if (!bTrim) return;
        str.replace(str.begin() + i + 1, str.end(), "");
    }

//压缩空白字符
    char* Trim(char* str) {
        if (NULL == str || '\0' == str[0]) return str;
        int i = 0;
        char* src = str;
        char strTemp[256];
        for (i = 0; '\0' != *src; src++) {
            if (' ' == *src || '\t' == *src) continue;
            strTemp[i++] = *src;
        }
        strTemp[i++] = '\0';
        strcpy(str, strTemp);
        return str;
    }

//压缩空白字符
    char* TrimRight(char* str) {
        if (NULL == str || '\0' == str[0]) return str;
        int i = 0;
        char* src = str;
        for (i = 0; '\0' != *src; src++) {
            if (' ' == *src || '\t' == *src) {
                i++;
                continue;
            }
            i = 0;
        }
        str[strlen(str) - i] = 0;
        return str;
    }

//压缩空白字符
    char* TrimLeft(char* str) {
        if (NULL == str || '\0' == str[0]) return str;
        char* src = str;
        for (; '\0' != *src; src++) {
            if (' ' != *src && '\t' != *src) break;
        }
        char strTemp[256];
        strcpy(strTemp, src);
        strcpy(str, strTemp);
        return str;
    }

    int reversal(int i) {
        int out = 0;
        out = i << 24;
        out += i >> 24;
        out += ((i >> 8) << 24) >> 8;
        out += ((i >> 16) << 24) >> 16;
        return out;
    }

    unsigned long GetFileSize(const char* filename) {
        unsigned long filesize = 0;
        struct stat statbuff;
        if (stat(filename, &statbuff) < 0)
            return filesize;
        else
            filesize = statbuff.st_size;
        return filesize;
    }

    unsigned int GetCUPNumber(unsigned int maxCpu, int defaultCpuNumber) {
#ifdef WIN32
        SYSTEM_INFO sysInfo;
        ::GetSystemInfo(&sysInfo);
        int dwNumCpu = sysInfo.dwNumberOfProcessors;
        if ( dwNumCpu > maxCpu ) return defaultCpuNumber;
        return dwNumCpu;
#else
        unsigned int dwNumCpu = (unsigned int) sysconf(_SC_NPROCESSORS_ONLN);
        if (dwNumCpu > maxCpu) return (unsigned int) defaultCpuNumber;
        return dwNumCpu;
#endif
    }

#ifndef WIN32

#include <linux/unistd.h>
#include <sys/syscall.h>
/*
 syscall(__NR_gettid)或者syscall(SYS_gettid)
 SYS_gettid与__NR_gettid是相等的常量
 在64bit=186，在32bit=224

 命令行查看程序线程id方法
 首先使用[pgrep 进程名]指令显示出主线程id
 然后使用[ls /proc/主线程id/task/]显示所有子线程id
 */
#define gettid() syscall(__NR_gettid)
#endif

    uint64 CurThreadId() {
#ifdef WIN32
        return GetCurrentThreadId();
#else
        return gettid();
#endif
    }

    //返回0时0分0秒的当前日期
    time_t mdf_Date() {
        time_t curTime = time(NULL);
        tm* pTm = localtime(&curTime);
        char hour[32];
        char mintue[32];
        char second[32];
        strftime(hour, 30, "%H", pTm);
        strftime(mintue, 30, "%M", pTm);
        strftime(second, 30, "%S", pTm);
        int sumSecond = atoi(hour) * 3600 + atoi(mintue) * 60 + atoi(second);
        curTime -= sumSecond;
        return curTime;
    }

    bool GetExeDir(char* exeDir, int size) {
#ifdef WIN32
        TCHAR *pExepath = (TCHAR*)exeDir;
        GetModuleFileName(NULL, pExepath, size);
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char fname[_MAX_FNAME];
        char ext[_MAX_EXT];

        _splitpath( pExepath, drive, dir, fname, ext);
        _makepath( pExepath, drive, dir, NULL, NULL);
        return true;
#else
        int rval;
        char* last_slash;

        //读符号链接 /proc/self/exe 的目标
        rval = readlink("/proc/self/exe", exeDir, size);
        if (rval == -1) //readlink调用失败
        {
            strcpy(exeDir, "./");
            return false;
        }
        exeDir[rval] = '\0';
        last_slash = strrchr(exeDir, '/');
        if (NULL == last_slash || exeDir == last_slash) //一些异常正在发生
        {
            strcpy(exeDir, "./");
            return false;
        }
        size = last_slash - exeDir;
        exeDir[size] = '\0';
#endif
        return true;
    }

    mdf::uint64 MillTime() {
#ifdef WIN32
        SYSTEMTIME st;
        GetLocalTime(&st);

        FILETIME ft;
        SystemTimeToFileTime( &st, &ft );
        //LONGLONG nLL;
        ULARGE_INTEGER ui;
        ui.LowPart = ft.dwLowDateTime;
        ui.HighPart = ft.dwHighDateTime;
        //nLL = (ft.dwHighDateTime << 32) + ft.dwLowDateTime;
        mdf::int64 mt = ((LONGLONG)(ui.QuadPart - 116444736000000000) / 10000);
        mt -= 3600 * 8 * 1000;//日历时间(time()返回的时间)
        return mt;
#else
        struct timespec ts;

        clock_gettime(CLOCK_REALTIME, &ts);

        return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
    }

    //HASH算法
    unsigned int APHash(const char* str) {
        unsigned int hash = 0;
        int i;
        for (i = 0; *str; i++) {
            if ((i & 1) == 0) {
                hash ^= ((hash << 7) ^ (*str++) ^ (hash >> 3));
            } else {
                hash ^= (~((hash << 11) ^ (*str++) ^ (hash >> 5)));
            }
        }
        return (hash & 0x7FFFFFFF);
    }

    //获取内存使用
    double get_mem_use() {
#ifndef WIN32
        FILE* fd = NULL;
        char buff[1024] = {0};
        char pattern[1024] = {0};
        MEM_OCCUPY mem;
        MEM_OCCUPY* p_mem = &mem;
        memset(p_mem, 0, sizeof(MEM_OCCUPY));

        fd = fopen("/proc/meminfo", "r");
        if (NULL == fd) return 0;

        while (NULL != fgets(buff, sizeof(buff), fd)) {
            sscanf(buff, "%s", pattern);
            if (strcmp(pattern, "MemTotal:") == 0)
                sscanf(buff, "%s %lu", p_mem->name, &p_mem->total);
            else if (strcmp(pattern, "MemFree:") == 0)
                sscanf(buff, "%s %lu", p_mem->name2, &p_mem->free);
            else if (strcmp(pattern, "Buffers:") == 0)
                sscanf(buff, "%s %lu", p_mem->name3, &p_mem->buffer);
            else if (strcmp(pattern, "Cached:") == 0)
                sscanf(buff, "%s %lu", p_mem->name4, &p_mem->cache);

            memset(pattern, 0, 1024);

        }

        fclose(fd); //关闭文件fd

        return (double) (p_mem->total - (p_mem->free + p_mem->buffer + p_mem->cache)) / (double) p_mem->total * (double) 100;
#else
        return 0;
#endif
    }

    //获取CPU使用(每秒)
    double get_cpu_use() {
#ifndef WIN32
        FILE* fd = NULL;
        char buff[1024] = {0};
        CPU_OCCUPY cpu_occupy, cpu_occupy_now;
        CPU_OCCUPY* p_cpu_occupy = &cpu_occupy;
        memset(p_cpu_occupy, 0, sizeof(CPU_OCCUPY));
        CPU_OCCUPY* p_cpu_occupy_now = &cpu_occupy_now;
        memset(p_cpu_occupy_now, 0, sizeof(CPU_OCCUPY));

        fd = fopen("/proc/stat", "r");
        if (NULL == fd) return 0;
        fgets(buff, sizeof(buff), fd);
        sscanf(buff, "%s %u %u %u %u", p_cpu_occupy->name, &p_cpu_occupy->user, &p_cpu_occupy->nice, &p_cpu_occupy->system, &p_cpu_occupy->idle);

        m_sleep(1000);
        memset(buff, 0, 1024);
        fseek(fd, 0, SEEK_SET);
        fgets(buff, sizeof(buff), fd);
        sscanf(buff, "%s %u %u %u %u", p_cpu_occupy_now->name, &p_cpu_occupy_now->user, &p_cpu_occupy_now->nice, &p_cpu_occupy_now->system, &p_cpu_occupy_now->idle);

        fclose(fd);

        double use = (double) ((p_cpu_occupy_now->user + p_cpu_occupy_now->nice + p_cpu_occupy_now->system) - (p_cpu_occupy->user + p_cpu_occupy->nice + p_cpu_occupy->system));
        double all = (double) ((p_cpu_occupy_now->user + p_cpu_occupy_now->nice + p_cpu_occupy_now->system + p_cpu_occupy_now->idle) - (p_cpu_occupy->user + p_cpu_occupy->nice + p_cpu_occupy->system + p_cpu_occupy->idle));

        return use / all * (double) 100;
#else
        return 0;
#endif
    }

    //分割字符串
    std::vector<std::string> split(std::string str, std::string pattern) {
        std::vector<std::string> result;

        if (pattern == "") {
            result.push_back(str);
            return result;
        }

        std::string::size_type pos;
        str += pattern;//扩展字符串以方便操作
        size_t size = str.length();

        for (size_t i = 0; i < size; i++) {
            pos = str.find(pattern, i);
            if (pos < size) {
                std::string s = str.substr(i, pos - i);
                result.push_back(s);
                i = pos + pattern.length() - 1;
            }
        }
        return result;
    }

}
