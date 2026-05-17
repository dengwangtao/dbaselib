#include "dbase/log/log.h"


using namespace dbase::log;


int main()
{
    setDefaultPatternStyle(PatternStyle::Threaded);

    DBASE_LOG_INFO("This is an info message");

    return 0;
}