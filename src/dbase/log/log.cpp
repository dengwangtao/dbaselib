#include "dbase/log/log.h"
#include "dbase/log/sink.h"
#include "dbase/platform/process.h"
#include "dbase/time/time.h"

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
            output += std::format("p={}", event.pid);
            needSeparator = true;
        }

        if (hasPatternField(m_style, PatternStyle::ThreadId))
        {
            if (needSeparator)
            {
                output += '|';
            }

            output += std::format("t={}", event.tid);
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

    for (const auto& sink : sinksCopy)
    {
        sink->flush();
    }
}

void Logger::log(Level level, std::string_view message, const std::source_location& location)
{
    if (!shouldLog(level))
    {
        return;
    }

    std::vector<std::shared_ptr<Sink>> sinksCopy;
    Formatter formatterCopy;
    const Level flushOnLevel = m_flushOn.load(std::memory_order_acquire);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        sinksCopy = m_sinks;
        formatterCopy = m_formatter;
    }

    const auto style = formatterCopy.style();

    const auto event = detail::makeLogEvent(level, message, location, style);

    const auto formatted = formatterCopy.format(event);

    for (const auto& sink : sinksCopy)
    {
        sink->write(event, formatted);
    }

    if (static_cast<int>(level) >= static_cast<int>(flushOnLevel))
    {
        for (const auto& sink : sinksCopy)
        {
            sink->flush();
        }
    }
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
}  // namespace dbase::log