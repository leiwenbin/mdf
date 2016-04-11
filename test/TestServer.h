//
//  TestServer.h
//  BaseSocketIOS
//
//  Created by admin on 15/8/26.
//  Copyright (c) 2015年 leiwenbin. All rights reserved.
//

#ifndef BaseSocketIOS_TestServer_h
#define BaseSocketIOS_TestServer_h
#pragma once

#include "../../include/frame/netserver/NetServer.h"

using namespace mdf;

class TestServer : public mdf::NetServer {
public:
    TestServer();

    virtual ~TestServer();

protected:
    void* Main(void* pParam);

    void OnConnect(mdf::NetHost& host);

    void OnCloseConnect(mdf::NetHost& host);

    void OnMsg(mdf::NetHost& host);

    void OnConnectFailed(char* ip, int port, int reConnectTime);
};

#endif
