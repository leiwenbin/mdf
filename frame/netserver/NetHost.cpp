#include "../../../include/frame/netserver/NetHost.h"
#include "../../../include/frame/netserver/NetConnect.h"
#include "../../../include/frame/netserver/HostData.h"
#include "../../../include/mdf/atom.h"

using namespace std;

namespace mdf {

    NetHost::NetHost() :
            m_pConnect(NULL) {
    }

    NetHost::NetHost(const NetHost& obj) :
            m_pConnect(NULL) {
        *this = obj;
    //	AtomAdd(&obj.m_pConnect->m_useCount, 1);
    //	m_pConnect = obj.m_pConnect;
    }

    NetHost& NetHost::operator=(const NetHost& obj) {
        if (m_pConnect == obj.m_pConnect) return *this;
        if (NULL != m_pConnect) m_pConnect->Release();
        if (NULL != obj.m_pConnect) AtomAdd(&obj.m_pConnect->m_useCount, 1);
        m_pConnect = obj.m_pConnect;
        return *this;
    }

    NetHost::~NetHost() {
        if (NULL != m_pConnect) m_pConnect->Release();
    }

    bool NetHost::Send(const unsigned char* pMsg, unsigned int uLength) {
        return m_pConnect->SendData(pMsg, uLength);
    }

    bool NetHost::Recv(unsigned char* pMsg, unsigned int uLength, bool bClearCache) {
        return m_pConnect->ReadData(pMsg, uLength, bClearCache);
    }

    void NetHost::Close() {
        m_pConnect->Close();
    }

    bool NetHost::IsServer() {
        return m_pConnect->IsServer();
    }

    int64 NetHost::ID() {
        if (NULL == m_pConnect) return -1;
        return m_pConnect->GetID();
    }

    //放入某分组
    void NetHost::InGroup(std::string& groupID) {
        m_pConnect->InGroup(groupID);
    }

    //从某分组删除
    void NetHost::OutGroup(std::string& groupID) {
        m_pConnect->OutGroup(groupID);
    }

    void NetHost::GetServerAddress(string& ip, int& port) {
        m_pConnect->GetServerAddress(ip, port);
        return;
    }

    void NetHost::GetAddress(string& ip, int& port) {
        m_pConnect->GetAddress(ip, port);
        return;
    }

    void NetHost::SetData(HostData* pData, bool autoFree) {
        m_pConnect->SetData(pData, autoFree);
        return;
    }

    HostData* NetHost::GetData() {
        return m_pConnect->GetData();
    }

    void* NetHost::GetSvrInfo() {
        return m_pConnect->GetSvrInfo();
    }

    //获取最近心跳时间
    time_t NetHost::GetLastHeartTime() {
        return m_pConnect->GetLastHeart();
    }

    //获取连接创建时间
    time_t NetHost::GetConnectCreateTime() {
        return m_pConnect->GetCreateTime();
    }

    //获取空连接状态
    bool NetHost::GetIdleState() {
        return m_pConnect->GetIdleState();
    }

    //获取连接业务行为状态
    bool NetHost::GetNoBehaviorState() {
        return m_pConnect->GetNoBehaviorState();
    }

    //设置连接业务行为
    void NetHost::SetBehavior() {
        m_pConnect->SetBehavior();
    }

    //获取连接是否是正常断开的
    bool NetHost::GetNormalDisconnect() {
        return m_pConnect->GetNormalDisconnect();
    }

    //获取发送缓冲区的数据长度
    uint32 NetHost::GetSendBufferDataLength() {
        return m_pConnect->GetSendBuffUsedLength();
    }

    //获取接收缓冲区的数据长度
    uint32 NetHost::GetRecvBufferDataLength() {
        return m_pConnect->GetLength();
    }

} // namespace mdf
