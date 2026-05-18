#include "dbase/log/log.h"
#include "dbase/log/registry.h"
#include "dbase/platform/process.h"
#include <thread>

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

    DBASE_LOG_INFO("This is an info message");
    DBASE_LOG_INFO("tid={}", dbase::platform::tid());
    DBASE_LOG_INFO("pid={}", dbase::platform::pid());
    DBASE_LOG_INFO("ppid={}", dbase::platform::ppid());
    DBASE_LOG_INFO("exe path={}", dbase::platform::executablePath().value().string());

    std::thread t(thread_func);
    t.join();

    return 0;
}