//
// Created by robot on 24-11-19.
//

#include <climits>
#include "LogHelper.h"





std::string LogHelper::previousFilenameTemp;
std::string LogHelper::m_loggerName;

/**
 * brief: 构造函数
 */

LogHelper::LogHelper():stopFlag(false)
{

}

/**
 * brief: 析构函数
 */

LogHelper::~LogHelper()
{
    // 停止线程并等待退出
    {
        std::lock_guard<std::mutex> lock(mtx);
        stopFlag = true;
        cv.notify_all();
    }
    if (rotationThread && rotationThread->joinable()) {
        rotationThread->join();
    }
}

void LogHelper::setLoggerName(const std::string& loggerName)
{
    m_loggerName = loggerName;
}

void LogHelper::startRotationThread()
{
    rotationThread = std::make_unique<std::thread>(&LogHelper::rotationLoop, this);
}

std::chrono::system_clock::time_point LogHelper::getNextMidnight()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm = *std::localtime(&tt);
    local_tm.tm_hour = 0;
    local_tm.tm_min = 0;
    local_tm.tm_sec = 0;
    auto midnight = std::chrono::system_clock::from_time_t(std::mktime(&local_tm));
    if(midnight < now) {
        midnight += std::chrono::hours(24); // 如果已经过了今天的0点，则设置为明天的0点
    }
    return midnight;
}

/**
 * brief: 日志按大小轮转回调函数
 * param: filename 日志文件名
 * param: size 日志文件大小
 */
void LogHelper::rolloutHandler(const char* filename, std::size_t size) {
    // SHOULD NOT LOG ANYTHING HERE BECAUSE LOG FILE IS CLOSED!
    std::cout << "************** Rolling out [" << filename << "] because it reached [" << size << " bytes]" << std::endl;
    auto L = el::Loggers::getLogger(m_loggerName);
    if(L == nullptr)
    {
        std::cout << "Oops, it is not called default!" << std::endl;
    }else
    {
        // 用于保存上一次的文件名时间戳
        static std::string filePathPrefix;
        std::string filenameTemp;
        if (previousFilenameTemp.empty()) {
            // 第一次调用时，直接使用传入的 filename
            filenameTemp = filename;
            size_t pos = filenameTemp.rfind('.');
            filenameTemp = filenameTemp.substr(0, pos);

            size_t prefixPos = filenameTemp.find_first_of('_');
            filePathPrefix = filenameTemp.substr(0, prefixPos + 1);

            std::cout << "First time: using original filename" << std::endl;
            std::cout << "filePathPrefix: " << filePathPrefix << std::endl;
        } else {
            // 后续调用时，使用上一次生成的时间戳
            filenameTemp = previousFilenameTemp;
            std::cout << "Subsequent time: using previous filenameTemp" << std::endl;
        }

        // 获取当前时间
        time_t currentTime = time(nullptr);
        struct tm currentTimeStruct{};
        localtime_r(&currentTime, &currentTimeStruct);

        char currentTimeStr[PATH_MAX] = {0};
        strftime(currentTimeStr, sizeof(currentTimeStr), "%Y%m%d_%H%M%S", &currentTimeStruct);

        // 格式化备份文件名
        char backupFile[PATH_MAX] = {0};
        snprintf(backupFile, sizeof(backupFile), "%s_%s.log", filenameTemp.c_str(), currentTimeStr);

        std::cout << "Backup file: " << backupFile << std::endl;

        // 更新静态变量保存当前的 filenameTemp
        previousFilenameTemp = filePathPrefix + currentTimeStr;

        // 执行自定义日志备份
        std::stringstream ss;
        ss << "mv " << filename << " " << backupFile;
        system(ss.str().c_str());
    }
}

/**
 * brief: 日志轮转线程(按天，每日0点轮转)
 * param: void
 */
void LogHelper::rotationLoop()
{
    while (true)
    {
        {
            auto nextWakeUp = getNextMidnight();
            std::unique_lock<std::mutex> lock(mtx);
            if (cv.wait_until(lock, nextWakeUp, [this]() { return stopFlag.load(); }))
            {
                break; // 收到停止信号，退出循环
            }
            auto L = el::Loggers::getLogger(m_loggerName);
            if (L == nullptr)
            {
                LOGS_ERROR(m_loggerName.c_str()) << "Oops, it is not called default!";
            }
            else
            {
                L->reconfigure();
                previousFilenameTemp.clear();
            }
        }
    }
}

void LogHelper::myCrashHandler(int sig) {
    //LOGGER(ERROR) << "Woops! Crashed!";
    // FOLLOWING LINE IS OPTIONAL
    el::Helpers::logCrashReason(sig, true, el::Level::Fatal, LogHelper::m_loggerName.c_str());
    // FOLLOWING LINE IS ABSOLUTELY NEEDED AT THE END IN ORDER TO ABORT APPLICATION
    el::Helpers::crashAbort(sig);
}

/**
 * brief: 初始化日志参数
 * param: void
 */
void LogHelper::init_param()
{
    el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);
    //el::Loggers::addFlag(el::LoggingFlag::MultiLoggerSupport);
    const el::Configurations conf("logging.conf");
    //const el::Configurations conf_default("logging_default.conf");
    el::Logger* fileLogger = el::Loggers::getLogger(m_loggerName);
    //el::Loggers::reconfigureLogger("default", conf_default);
    el::Loggers::reconfigureLogger(fileLogger, conf);

    el::Helpers::installPreRollOutCallback(&LogHelper::rolloutHandler);
    // 启用严格日志文件大小检查
    el::Loggers::addFlag(el::LoggingFlag::StrictLogFileSizeCheck);

    el::Helpers::setCrashHandler(&LogHelper::myCrashHandler);

    LOGS_INFO(m_loggerName.c_str()) << "begin TEST!";
    //LOGGER(ERROR) << "这是一条测试消息";
    LOGS_INFO(m_loggerName.c_str()) << "*************This is how we do it.";
}

/**
 * brief: 解析日志文件名
 * param: filename 日志文件名
 * return: std::string
 */

std::string LogHelper::resolveFilename(const std::string& filename)
{
    std::string resultingFilename = filename;
    std::size_t dateIndex = std::string::npos;
    std::string dateTimeFormatSpecifierStr = std::string("%datetime");
    while ((dateIndex = resultingFilename.find(dateTimeFormatSpecifierStr)) != std::string::npos) {
        while (dateIndex > 0 && resultingFilename[dateIndex - 1] == el::base::consts::kFormatSpecifierChar) {
            dateIndex = resultingFilename.find(dateTimeFormatSpecifierStr, dateIndex + 1);
        }
        if (dateIndex != std::string::npos) {
            const char* ptr = resultingFilename.c_str() + dateIndex;
            // Goto end of specifier
            ptr += dateTimeFormatSpecifierStr.size();
            std::string fmt;
            if ((resultingFilename.size() > dateIndex) && (ptr[0] == '{')) {
                // User has provided format for date/time
                ++ptr;
                int count = 1;  // Start by 1 in order to remove starting brace
                std::stringstream ss;
                for (; *ptr; ++ptr, ++count) {
                    if (*ptr == '}') {
                        ++count;  // In order to remove ending brace
                        break;
                    }
                    ss << *ptr;
                }
                resultingFilename.erase(dateIndex + dateTimeFormatSpecifierStr.size(), count);
                fmt = ss.str();
            } else {
                fmt = std::string("%datetime");
            }
            el::base::SubsecondPrecision ssPrec(3);
            std::string now = el::base::utils::DateTime::getDateTime(fmt.c_str(), &ssPrec);
            el::base::utils::Str::replaceAll(now, '/', '-'); // Replace path element since we are dealing with filename
            resultingFilename.replace(dateIndex, dateTimeFormatSpecifierStr.size(), now);
        }
    }
    return resultingFilename;
}
