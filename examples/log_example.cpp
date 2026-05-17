#include "dbase/log/log.h"
#include "dbase/platform/process.h"
#include <thread>

using namespace dbase::log;


void thread_func()
{
    DBASE_LOG_INFO("This is an info message");
    DBASE_LOG_INFO("tid={}", dbase::platform::tid());
    DBASE_LOG_INFO("pid={}", dbase::platform::pid());
    DBASE_LOG_INFO("ppid={}", dbase::platform::ppid());
    DBASE_LOG_INFO("exe path={}", dbase::platform::executablePath().value().string());
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