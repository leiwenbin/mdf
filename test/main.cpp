//
//  main.cpp
//  BaseSocket
//
//  Created by admin on 15/8/26.
//  Copyright (c) 2015年 leiwenbin. All rights reserved.
//

#include "main.h"

int main() {
    TestServer ser;

    ser.Listen(9999);
    //ser.Connect("10.10.158.184", 9090, NULL, 1);
    ser.Start();

    //ser.SendMsg(1, NULL, 0);
    ser.WaitStop();
    return 0;
}
