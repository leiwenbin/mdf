//
//  TestServer.cpp
//  BaseSocketIOS
//
//  Created by admin on 15/8/26.
//  Copyright (c) 2015年 leiwenbin. All rights reserved.
//

#include "string.h"
#include "stdio.h"
#include "unistd.h"
#include "../../include/mdf/mapi.h"
#include "TestServer.h"

TestServer::TestServer() {
}

TestServer::~TestServer() {
}

void* TestServer::Main(void* pParam) {
    while (IsOk()) {
#ifndef WIN32
        m_sleep(1000);
#else
        Sleep(1000);
#endif
    }
    return 0;
}

void TestServer::OnConnect(mdf::NetHost& host) {
    char* b = new char[100];
    for (int i = 0; i < 100; ++i) {
        this->SendMsg(host.ID(), b, 99);
    }

    sleep(3);

    for (int i = 0; i < 100; ++i) {
        this->SendMsg(host.ID(), b, 99);
    }

    printf("new client(%i) connect in\n", host.ID());
}

void TestServer::OnCloseConnect(mdf::NetHost& host) {
    printf("client(%i) close connect\n", host.ID());
}

void TestServer::OnMsg(mdf::NetHost& host) {
    unsigned char c[256] = {0};
    if (!host.Recv(c, 4, false))
        return;
    unsigned short len = 0;
    memcpy(&len, c, 4);
    if (len > 256) {
        printf("close client:wrong format msg\n");
        host.Close();
        return;
    }
    if (!host.Recv(c, len))
        return;
}

void TestServer::OnConnectFailed(char* ip, int port, int reConnectTime) {
    printf("connect %s:%d,%d failed!\n", ip, port, reConnectTime);
}
