#include <iostream>
#include "LogHelper.h"


INITIALIZE_EASYLOGGINGPP


void causeCrash() {
    LOGS_INFO("tasknode") << "这是一条崩溃消息";
    int* ptr = nullptr;
    *ptr = 42;  // 导致崩溃
}



int main()
{
#define LOGGER_NAME "tasknode"
    // 启动日志
    LogHelper logHelper;
    LogHelper::setLoggerName(LOGGER_NAME);
    LogHelper::init_param();
    logHelper.startRotationThread();


    for (int i = 0; i < 600; ++i)
    {
        LOGS_INFO(LOGGER_NAME) << "Test " << i;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    causeCrash();

    LOGS_INFO(LOGGER_NAME) << "end TEST!";
    return 0;
}
