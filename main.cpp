#include <iostream>
#include "LogHelper.h"


INITIALIZE_EASYLOGGINGPP


void causeCrash() {
    LOGGER(INFO) << "这是一条崩溃消息";
    int* ptr = nullptr;
    *ptr = 42;  // 导致崩溃
}



int main()
{
    // 启动日志
    LogHelper logHelper;
    LogHelper::init_param();
    logHelper.startRotationThread();


    for (int i = 0; i < 600; ++i)
    {
        LOGGER(INFO) << "Test " << i;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    causeCrash();

    LOGGER(INFO) << "end TEST!";
    return 0;
}
