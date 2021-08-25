//
// Created by cjf12 on 2019-10-10.
//

#include "include/DroidBlaster.h"
#include "include/EventLoop.h"
#include "include/Log.h"

void android_main(android_app* pApplication){
    DroidBlaster(pApplication).run();
}

