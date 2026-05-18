#include "dbase/log/log.h"
#include "dbase/log/registry.h"
#include "dbase/platform/process.h"
#include "dbase/log/sink.h"
#include "dbase/fs/fs.h"
#include <format>
#include <memory>
#include <thread>
#include <iostream>

using namespace dbase::log;

void thread_func()
{
    registry().clear();
    registry().add("thread_console_logger", std::make_shared<Logger>(PatternStyle::Timestamp | PatternStyle::Level | PatternStyle::Message | PatternStyle::ThreadId));

#define MY_LOG_INFO(LOGGER_NAME, ...)                                                                          \
    if (hasLogger(LOGGER_NAME))                                                                                \
    {                                                                                                          \
        getLogger(LOGGER_NAME)->logf(::dbase::log::Level::Info, std::source_location::current(), __VA_ARGS__); \
    }

    MY_LOG_INFO("thread_console_logger", "This is an info message");
    MY_LOG_INFO("thread_console_logger", "tid={}", dbase::platform::tid());
    MY_LOG_INFO("thread_console_logger", "pid={}", dbase::platform::pid());
    MY_LOG_INFO("thread_console_logger", "ppid={}", dbase::platform::ppid());
    MY_LOG_INFO("thread_console_logger", "exe path={}", dbase::platform::executablePath().value().string());
#undef MY_LOG_INFO
}

int main()
{
    setDefaultPatternStyle(PatternStyle::Threaded);
    addDefaultSink(std::make_shared<FileSink>("log.log", true));

    DBASE_LOG_INFO("This is an info message");
    DBASE_LOG_INFO("tid={}", dbase::platform::tid());
    DBASE_LOG_INFO("pid={}", dbase::platform::pid());
    DBASE_LOG_INFO("ppid={}", dbase::platform::ppid());
    DBASE_LOG_INFO("exe path={}", dbase::platform::executablePath().value().string());

    std::thread t(thread_func);
    t.join();

    clearDefaultSinks();
    addDefaultSink(std::make_shared<RotatingFileSink>("log_rotate.log", 1024, 3, true));
    for (int i = 0; i < 100; ++i)
    {
        DBASE_LOG_INFO("Logging message {}", i);
    }
    resetDefaultSinks();

    if (true)
    {
        if (dbase::fs::exists("log.log"))
        {
            (void)dbase::fs::removeFile("log.log");
        }
        if (dbase::fs::exists("log_rotate.log"))
        {
            (void)dbase::fs::removeFile("log_rotate.log");
        }

        for (int i = 1; i < 100; ++i)
        {
            std::string filename = std::format("log_rotate.{}.log", i);
            if (dbase::fs::exists(filename))
            {
                (void)dbase::fs::removeFile(filename);
            }
            else
            {
                break;
            }
        }
    }

    resetDefaultSinks();
    addDefaultMode(LogMode::Async);

    DBASE_LOG_INFO("[Async] This is an info message");

    return 0;
}