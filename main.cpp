#include <iostream>
#include <thread>
#include <climits>
#include <cstdio>
#include <ctime>

#include "easylogging++.h"

INITIALIZE_EASYLOGGINGPP
#define ELPP_THREAD_SAFE
#define ELPP_NO_DEFAULT_LOG_FILE
#define ELPP_EXPERIMENTAL_ASYNC



void causeCrash() {
    LOG(ERROR) << "这是一条崩溃消息";
    int* ptr = nullptr;
    *ptr = 42;  // 导致崩溃
}


// 获取下一个0点的时间点

std::chrono::system_clock::time_point getNextMidnight() {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
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




#if 1
std::string resolveFilename(const std::string& filename) {
    std::string resultingFilename = filename;
    std::size_t dateIndex = std::string::npos;
    std::string dateTimeFormatSpecifierStr = std::string("%datetime");
    while ((dateIndex = resultingFilename.find(dateTimeFormatSpecifierStr.c_str())) != std::string::npos) {
        while (dateIndex > 0 && resultingFilename[dateIndex - 1] == el::base::consts::kFormatSpecifierChar) {
            dateIndex = resultingFilename.find(dateTimeFormatSpecifierStr.c_str(), dateIndex + 1);
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

#endif

static unsigned int idx;
static  bool is_need_switch_file = false;
static std::string previousFilenameTemp;

#if 1
void rolloutHandler(const char* filename, std::size_t size) {
    // SHOULD NOT LOG ANYTHING HERE BECAUSE LOG FILE IS CLOSED!
    std::cout << "************** Rolling out [" << filename << "] because it reached [" << size << " bytes]" << std::endl;


    auto L = el::Loggers::getLogger("default");
    if(L == nullptr)
    {
        std::cout << "Oops, it is not called default!" << std::endl;
    }else
    {
        // 获取 logger 的配置
        // el::Configurations* conf = L->configurations();
        // // 从配置中获取日志文件名
        // std::string logFilePath = "logs/tasknode/%datetime{%Y-%M-%d}/tasknode_%datetime{%Y%M%d_%H%m%s_%g}.log";
        // std::string resolvedFilename = resolveFilename(logFilePath);
        // std::cout << "transfer log file path: " << resolvedFilename << std::endl;

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
#endif



int main()
{
    el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);
    el::Configurations conf("logging.conf");
    el::Loggers::reconfigureLogger("default", conf);
    el::Helpers::installPreRollOutCallback(rolloutHandler);
    // 启用严格日志文件大小检查
    el::Loggers::addFlag(el::LoggingFlag::StrictLogFileSizeCheck);

    LOG(INFO) << "begin TEST!";
    LOG(ERROR) << "这是一条测试消息";

    // 日志轮转线程
    std::thread logRotatorThread([]() {
        while (true) {
            {
                auto nextWakeUp = getNextMidnight();
                std::this_thread::sleep_until(nextWakeUp);
                LOG(INFO) << "About to rotate log file!";
                auto L = el::Loggers::getLogger("default");
                if (L == nullptr)
                {
                    LOG(ERROR) << "Oops, it is not called default!";
                }
                else
                {
                    L->reconfigure();
                    previousFilenameTemp.clear();
                }
            }
        }
    });

    logRotatorThread.detach();


    // 获取 logger 的配置
    const el::Configuration* config = conf.get(el::Level::Global, el::ConfigurationType::LogFlushThreshold);
    const std::string flushThreshold = config ? config->value() : "";
    LOG(INFO) << "flushThreshold" << flushThreshold;

    for (int i = 0; i < 1000; ++i)
    {
        LOG(INFO) << "Test " << i;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    causeCrash();

    el::Helpers::uninstallPreRollOutCallback();

    LOG(INFO) << "end TEST!";
    return 0;
}
