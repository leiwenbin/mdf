﻿#include "../../../include/mdf/mapi.h"
#include "../../../include/frame/netserver/NetServer.h"
#include "../../../include/frame/netserver/NetEngine.h"
#include "../../../include/frame/netserver/IOCPFrame.h"
#include "../../../include/frame/netserver/EpollFrame.h"

namespace mdf {

    NetServer::NetServer() {
#ifdef WIN32
        m_pNetCard = new mdf::IOCPFrame;
#else
        m_pNetCard = new mdf::EpollFrame;
#endif
        m_pNetCard->m_pNetServer = this;
        m_bStop = true;
    }

    NetServer::~NetServer() {
        MDF_SAFE_DELETE(m_pNetCard);
    }

    void* NetServer::TMain(void* pParam) {
        Main(pParam);
        return 0;
    }

    void NetServer::SetOnWorkStart(MethodPointer method, void* pObj, void* pParam) {
        m_pNetCard->SetOnWorkStart(method, pObj, pParam);
    }

    void NetServer::SetOnWorkStart(FuntionPointer fun, void* pParam) {
        m_pNetCard->SetOnWorkStart(fun, pParam);
    }

    /**
     * 客户接口，对所有可见
     * 运行服务器
     * 默认逻辑按一下顺序启动模块
     * 硬盘、显卡、CPU、网卡
     *
     */
    const char* NetServer::Start() {
        if (NULL == this->m_pNetCard) return "no class NetEngine object";
        if (!m_pNetCard->Start()) return m_pNetCard->GetInitError();
        m_bStop = false;
        m_mainThread.Run(Executor::Bind(&NetServer::TMain), this, NULL);
        return NULL;
    }

    void NetServer::WaitStop() {
        if (NULL == this->m_pNetCard) return;
        m_pNetCard->WaitStop();
        m_mainThread.WaitStop();
    }

    /**
     * 客户接口，对所有可见
     * 关闭服务器
     */
    void NetServer::Stop() {
        m_bStop = true;
        m_mainThread.Stop(3000);
        if (NULL != this->m_pNetCard) m_pNetCard->Stop();
    }

    //服务器正常返回true，否则返回false
    bool NetServer::IsOk() {
        return !m_bStop;
    }

    //设置平均连接数
    void NetServer::SetAverageConnectCount(int count) {
        m_pNetCard->SetAverageConnectCount(count);
    }

    //设置最大连接数
    void NetServer::SetMaxConnectCount(int count) {
        m_pNetCard->SetMaxConnectCount(count);
    }

    //设置心跳时间
    void NetServer::SetHeartTime(int nSecond) {
        m_pNetCard->SetHeartTime(nSecond);
    }

    //设置防空连接时间
    void NetServer::SetIdleTime(int nSecond) {
        m_pNetCard->SetIdleTime(nSecond);
    }

    //设置网络IO线程数量
    void NetServer::SetIOThreadCount(int nCount) {
        m_pNetCard->SetIOThreadCount(nCount);
    }

    //设置工作线程数
    void NetServer::SetWorkThreadCount(int nCount) {
        m_pNetCard->SetWorkThreadCount(nCount);
    }

    //监听端口
    bool NetServer::Listen(int port) {
        return m_pNetCard->Listen(port);
    }

    //连接其它IP
    bool NetServer::Connect(const char* ip, int port, void* pSvrInfo, int reConnectTime) {
        return m_pNetCard->Connect(ip, port, pSvrInfo, reConnectTime);
    }

    //向某组连接广播消息
    void NetServer::BroadcastMsg(std::vector<std::string>* recvGroupIDs, char* msg, unsigned int msgsize, std::vector<std::string>* filterGroupIDs) {
        m_pNetCard->BroadcastMsg(recvGroupIDs, msg, msgsize, filterGroupIDs);
    }

    //返回发送失败列表的向某组连接广播消息
    void NetServer::BroadcastMsg(std::vector<std::string>* recvGroupIDs, char* msg, unsigned int msgsize, std::vector<std::string>* filterGroupIDs, std::vector<int64>* failedList) {
        m_pNetCard->BroadcastMsg(recvGroupIDs, msg, msgsize, filterGroupIDs, failedList);
    }

    //向某主机发送消息
    bool NetServer::SendMsg(int64 hostID, char* msg, unsigned int msgsize) {
        return m_pNetCard->SendMsg(hostID, msg, msgsize);
    }

    /*
     关闭与主机的连接
     */
    void NetServer::CloseConnect(int64 hostID) {
        m_pNetCard->CloseConnect(hostID);
    }

    //打开TCP_NODELAY
    void NetServer::OpenNoDelay() {
        m_pNetCard->OpenNoDelay();
    }

    //获取当前连接数
    uint32 NetServer::GetConnectionCount() {
        return m_pNetCard->GetConnectionCount();
    }

    //获取网络IO线程池任务数
    uint32 NetServer::GetNetIOTaskCount() {
        return m_pNetCard->GetNetIOTaskCount();
    }

    //获取工作线程池任务数
    uint32 NetServer::GetWorkTaskCount() {
        return m_pNetCard->GetWorkTaskCount();
    }

    const char* NetServer::GetInitError() {
        return m_pNetCard->GetInitError();
    }

} // namespace mdf
