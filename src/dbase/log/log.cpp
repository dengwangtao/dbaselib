#include "dbase/log/log.h"
#include "dbase/log/sink.h"
#include "dbase/platform/process.h"
#include "dbase/time/time.h"
#include "dbase/thread/thread.h"

#include <format>
#include <string>
#include <utility>

namespace dbase::log
{
namespace
{
std::string baseFileName(std::string_view file)
{
    const auto pos = file.find_last_of("/\\");
    if (pos == std::string_view::npos)
    {
        return std::string(file);
    }

    return std::string(file.substr(pos + 1));
}

void replaceAllInPlace(std::string& text, std::string_view from, std::string_view to)
{
    if (from.empty())
    {
        return;
    }

    std::size_t pos = 0;
    while ((pos = text.find(from.data(), pos, from.size())) != std::string::npos)
    {
        text.replace(pos, from.size(), to.data(), to.size());
        pos += to.size();
    }
}

std::string normalizeFunctionName(std::string_view func)
{
    std::string text(func);

    replaceAllInPlace(text, "__cdecl ", "");
    replaceAllInPlace(text, "__thiscall ", "");
    replaceAllInPlace(text, "__vectorcall ", "");
    replaceAllInPlace(text, "__stdcall ", "");
    replaceAllInPlace(text, "__fastcall ", "");
    replaceAllInPlace(text, "(void)", "()");

    const auto parenPos = text.find('(');
    if (parenPos != std::string::npos)
    {
        const auto spacePos = text.rfind(' ', parenPos);
        if (spacePos != std::string::npos)
        {
            text.erase(0, spacePos + 1);
        }
    }

    return text;
}

std::string formatTimestampUs(std::int64_t timestampUs)
{
    const auto ms = static_cast<int>((timestampUs / 1000) % 1000);

    return std::format("{}.{:03}", dbase::time::formatTimestampMs(timestampUs / 1000, "%Y-%m-%d %H:%M:%S"), ms);
}

void appendBracketed(std::string& output, std::string_view text)
{
    output += '[';
    output += text;
    output += "] ";
}
}  // namespace

namespace detail
{
LogEvent makeLogEvent(Level level, std::string_view message, const std::source_location& location, PatternStyle style)
{
    LogEvent event;
    event.level = level;
    event.message = std::string(message);
    event.timestampUs = dbase::time::nowUs();
    event.sourceLocation = location;

    if (hasPatternField(style, PatternStyle::ProcessId))
    {
        event.pid = dbase::platform::pid();
    }

    if (hasPatternField(style, PatternStyle::ThreadId))
    {
        event.tid = dbase::platform::tid();
    }

    return event;
}
}  // namespace detail

Formatter::Formatter(PatternStyle style)
    : m_style(style)
{
}

void Formatter::setStyle(PatternStyle style) noexcept
{
    m_style = style;
}

PatternStyle Formatter::style() const noexcept
{
    return m_style;
}

std::string Formatter::format(const LogEvent& event) const
{
    std::string output;
    output.reserve(event.message.size() + 128);

    if (hasPatternField(m_style, PatternStyle::Timestamp))
    {
        appendBracketed(output, formatTimestampUs(event.timestampUs));
    }

    if (hasPatternField(m_style, PatternStyle::Level))
    {
        appendBracketed(output, toSpdlogString(event.level));
    }

    if (hasPatternField(m_style, PatternStyle::ProcessId) || hasPatternField(m_style, PatternStyle::ThreadId))
    {
        output += '[';

        bool needSeparator = false;

        if (hasPatternField(m_style, PatternStyle::ProcessId))
        {
            output += std::format("pid={}", event.pid);
            needSeparator = true;
        }

        if (hasPatternField(m_style, PatternStyle::ThreadId))
        {
            if (needSeparator)
            {
                output += ' ';
            }

            output += std::format("tid={}", event.tid);
        }

        output += "] ";
    }

    if (hasPatternField(m_style, PatternStyle::File))
    {
        output += '[';
        output += baseFileName(event.sourceLocation.file_name());

        if (hasPatternField(m_style, PatternStyle::Line))
        {
            output += std::format(":{}", event.sourceLocation.line());
        }

        output += "] ";
    }
    else if (hasPatternField(m_style, PatternStyle::Line))
    {
        output += std::format("[line={}] ", event.sourceLocation.line());
    }

    if (hasPatternField(m_style, PatternStyle::Function))
    {
        appendBracketed(
                output,
                normalizeFunctionName(event.sourceLocation.function_name()));
    }

    if (hasPatternField(m_style, PatternStyle::Message))
    {
        output += event.message;
    }

    if (!output.empty() && output.back() == ' ')
    {
        output.pop_back();
    }

    return output;
}

Logger::Logger()
    : Logger(PatternStyle::Source)
{
}

Logger::Logger(PatternStyle style)
    : m_formatter(style)
{
    m_sinks.emplace_back(std::make_shared<ConsoleSink>());
}

Logger::Logger(PatternStyle style, LogMode mode)
    : m_formatter(style)
{
    m_sinks.emplace_back(std::make_shared<ConsoleSink>());
    if (mode == LogMode::Async)
    {
        m_mode.store(LogMode::Async, std::memory_order_release);
        startWorker();
    }
}

Logger::~Logger()
{
    stopWorker();
}

void Logger::setMode(LogMode mode)
{
    const auto oldMode = m_mode.load(std::memory_order_acquire);

    if (oldMode == mode)
    {
        return;
    }

    if (mode == LogMode::Async)
    {
        startWorker();
        m_mode.store(LogMode::Async, std::memory_order_release);
    }
    else
    {
        m_mode.store(LogMode::Sync, std::memory_order_release);
        stopWorker();
    }
}

LogMode Logger::mode() const noexcept
{
    return m_mode.load(std::memory_order_acquire);
}

void Logger::setLevel(Level level) noexcept
{
    m_level.store(level, std::memory_order_release);
}

Level Logger::level() const noexcept
{
    return m_level.load(std::memory_order_acquire);
}

bool Logger::shouldLog(Level level) const noexcept
{
    return static_cast<int>(level) >= static_cast<int>(m_level.load(std::memory_order_acquire));
}

void Logger::setPatternStyle(PatternStyle style) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_formatter.setStyle(style);
}

PatternStyle Logger::patternStyle() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_formatter.style();
}

void Logger::setFlushOn(Level level) noexcept
{
    m_flushOn.store(level, std::memory_order_release);
}

Level Logger::flushOn() const noexcept
{
    return m_flushOn.load(std::memory_order_acquire);
}

void Logger::addSink(std::shared_ptr<Sink> sink)
{
    if (!sink)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_sinks.emplace_back(std::move(sink));
}

void Logger::clearSinks()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sinks.clear();
}

void Logger::flush()
{
    std::vector<std::shared_ptr<Sink>> sinksCopy;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        sinksCopy = m_sinks;
    }

    if (m_mode.load(std::memory_order_acquire) == LogMode::Sync)
    {
        for (const auto& sink : sinksCopy)
        {
            sink->flush();
        }
        return;
    }

    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();

    LogTask task;
    task.type = LogTaskType::Flush;
    task.sinks = std::move(sinksCopy);
    task.flushPromise = std::move(promise);

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_queue.emplace_back(std::move(task));
    }

    m_queueCv.notify_one();
    future.wait();
}

void Logger::log(Level level, std::string_view message, const std::source_location& location)
{
    if (!shouldLog(level))
    {
        return;
    }

    LogTask task;
    task.flushOn = m_flushOn.load(std::memory_order_acquire);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        task.sinks = m_sinks;
        task.formatter = m_formatter;
    }
    task.event = detail::makeLogEvent(level, message, location, task.formatter.style());

    if (m_mode.load(std::memory_order_acquire) == LogMode::Async)
    {
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_queue.emplace_back(std::move(task));
        }

        m_queueCv.notify_one();
        return;
    }

    writeTask(task);
}

const char* toString(Level level) noexcept
{
    switch (level)
    {
        case Level::Trace:
            return "TRACE";
        case Level::Debug:
            return "DEBUG";
        case Level::Info:
            return "INFO";
        case Level::Warn:
            return "WARN";
        case Level::Error:
            return "ERROR";
        case Level::Fatal:
            return "FATAL";
        default:
            return "UNKNOWN";
    }
}

const char* toSpdlogString(Level level) noexcept
{
    switch (level)
    {
        case Level::Trace:
            return "trace";
        case Level::Debug:
            return "debug";
        case Level::Info:
            return "info";
        case Level::Warn:
            return "warn";
        case Level::Error:
            return "error";
        case Level::Fatal:
            return "critical";
        default:
            return "unknown";
    }
}

Logger& defaultLogger()
{
    static Logger logger;
    return logger;
}

void setDefaultLevel(Level level) noexcept
{
    defaultLogger().setLevel(level);
}

void setDefaultPatternStyle(PatternStyle style) noexcept
{
    defaultLogger().setPatternStyle(style);
}

void setDefaultFlushOn(Level level) noexcept
{
    defaultLogger().setFlushOn(level);
}

void addDefaultSink(std::shared_ptr<Sink> sink)
{
    defaultLogger().addSink(std::move(sink));
}

void addDefaultMode(LogMode mode)
{
    defaultLogger().setMode(mode);
}

void resetDefaultSinks()
{
    auto& logger = defaultLogger();
    logger.clearSinks();
    logger.addSink(std::make_shared<ConsoleSink>());
}

void flushDefaultLogger()
{
    defaultLogger().flush();
}

void clearDefaultSinks()
{
    defaultLogger().clearSinks();
}

void log(Level level, std::string_view message, const std::source_location& location)
{
    defaultLogger().log(level, message, location);
}

void Logger::startWorker()
{
    std::lock_guard<std::mutex> lock(m_queueMutex);

    if (m_worker && m_worker->joinable())
    {
        return;
    }

    m_stopping = false;

    m_worker = std::make_unique<dbase::thread::Thread>([this]
                                                       { workerLoop(); }, "logger_worker");
    m_worker->start();
}

void Logger::stopWorker()
{
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_stopping = true;
    }

    m_queueCv.notify_all();

    if (m_worker && m_worker->joinable())
    {
        m_worker->join();
    }
}

void Logger::workerLoop()
{
    for (;;)
    {
        LogTask task;

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);

            m_queueCv.wait(lock, [this]
                           { return m_stopping || !m_queue.empty(); });

            if (m_stopping && m_queue.empty())
            {
                break;
            }

            task = std::move(m_queue.front());
            m_queue.pop_front();
        }

        writeTask(task);
    }
}

void Logger::writeTask(const LogTask& task)
{
    if (task.type == LogTaskType::Flush)
    {
        for (const auto& sink : task.sinks)
        {
            sink->flush();
        }

        if (task.flushPromise)
        {
            task.flushPromise->set_value();
        }

        return;
    }

    const auto formatted = task.formatter.format(task.event);

    for (const auto& sink : task.sinks)
    {
        sink->write(task.event, formatted);
    }

    if (static_cast<int>(task.event.level) >= static_cast<int>(task.flushOn))
    {
        for (const auto& sink : task.sinks)
        {
            sink->flush();
        }
    }
}

}  // namespace dbase::log