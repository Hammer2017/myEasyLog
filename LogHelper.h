//
// Created by robot on 24-11-19.
//

#ifndef LOGHELPER_H
#define LOGHELPER_H

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include "easylogging++.h"

#define LOGGER_NAME "tasknode"
#define MY_CUSTOM_LOGGER(LEVEL) CLOG(LEVEL, "default", LOGGER_NAME)
#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE
#define ELPP_EXPERIMENTAL_ASYNC



class LogHelper {
public:
    explicit LogHelper();

    ~LogHelper();

    void startRotationThread();

    static std::chrono::system_clock::time_point getNextMidnight();

    static void init_param();
    static std::string resolveFilename(const std::string& filename);

    static void rolloutHandler(const char* filename, std::size_t size);
    void rotationLoop();


private:
    std::unique_ptr<std::thread> rotationThread;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> stopFlag;
    static std::string previousFilenameTemp;
};


#endif //LOGHELPER_H
