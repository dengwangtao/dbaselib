#include <benchmark/benchmark.h>

#include "dbase/log/log.h"
#include "dbase/log/sink.h"

#include <memory>
#include <string_view>

namespace
{
class NullSink final : public dbase::log::Sink
{
    public:
        void write(
                const dbase::log::LogEvent&,
                std::string_view) override
        {
            // Intentionally discard log output.
        }

        void flush() override
        {
        }
};

std::shared_ptr<dbase::log::Logger> makeLogger(dbase::log::PatternStyle style)
{
    auto logger = std::make_shared<dbase::log::Logger>(style);
    logger->clearSinks();
    logger->addSink(std::make_shared<NullSink>());
    logger->setLevel(dbase::log::Level::Trace);
    logger->setFlushOn(dbase::log::Level::Fatal);
    return logger;
}
}  // namespace

static void BM_Log_Compact(benchmark::State& state)
{
    auto logger = makeLogger(dbase::log::PatternStyle::Compact);

    for (auto _ : state)
    {
        logger->log(
                dbase::log::Level::Info,
                "hello benchmark",
                std::source_location::current());
    }
}

BENCHMARK(BM_Log_Compact);

static void BM_Log_Source(benchmark::State& state)
{
    auto logger = makeLogger(dbase::log::PatternStyle::Source);

    for (auto _ : state)
    {
        logger->log(
                dbase::log::Level::Info,
                "hello benchmark",
                std::source_location::current());
    }
}

BENCHMARK(BM_Log_Source);

static void BM_Log_SourceFunc(benchmark::State& state)
{
    auto logger = makeLogger(dbase::log::PatternStyle::SourceFunc);

    for (auto _ : state)
    {
        logger->log(
                dbase::log::Level::Info,
                "hello benchmark",
                std::source_location::current());
    }
}

BENCHMARK(BM_Log_SourceFunc);

static void BM_Log_Threaded(benchmark::State& state)
{
    auto logger = makeLogger(dbase::log::PatternStyle::Threaded);

    for (auto _ : state)
    {
        logger->log(
                dbase::log::Level::Info,
                "hello benchmark",
                std::source_location::current());
    }
}

BENCHMARK(BM_Log_Threaded);

static void BM_Log_Disabled(benchmark::State& state)
{
    auto logger = makeLogger(dbase::log::PatternStyle::Source);
    logger->setLevel(dbase::log::Level::Error);

    for (auto _ : state)
    {
        logger->log(
                dbase::log::Level::Info,
                "hello benchmark",
                std::source_location::current());
    }
}

BENCHMARK(BM_Log_Disabled);

BENCHMARK_MAIN();