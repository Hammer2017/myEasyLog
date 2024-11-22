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

//#define LOGGER_NAME "tasknode"
//#define LOGGER(LEVEL) CLOG(LEVEL, LOGGER_NAME)

#define LOGS_TRACE(module) CLOG(TRACE, module)
#define LOGS_INFO(module)  CLOG(INFO, module)
#define LOGS_WARN(module)  CLOG(WARN, module)
#define LOGS_DEBUG(module) CLOG(DEBUG, module)
#define LOGS_ERROR(module) CLOG(ERROR, module)
#define LOGS_FATAL(module) CLOG(FATAL, module)

#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE
#define ELPP_EXPERIMENTAL_ASYNC



class LogHelper {
public:
    LogHelper();
    ~LogHelper();
    static void setLoggerName(const std::string& loggerName);
    void startRotationThread();

    static std::chrono::system_clock::time_point getNextMidnight();

    static void init_param();
    static std::string resolveFilename(const std::string& filename);

    static void rolloutHandler(const char* filename, std::size_t size);
    void rotationLoop();
    static void myCrashHandler(int sig);

private:
    std::unique_ptr<std::thread> rotationThread;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> stopFlag;
    static std::string previousFilenameTemp;
    static std::string m_loggerName;
};


#endif //LOGHELPER_H
