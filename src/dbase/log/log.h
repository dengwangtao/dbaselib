#pragma once
#include <atomic>
#include <cstdint>
#include <format>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

namespace dbase::log
{
class Sink;

enum class Level
{
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Fatal
};

enum class PatternStyle : std::uint32_t
{
    None = 0,

    Timestamp = 1u << 0,
    Level = 1u << 1,
    Message = 1u << 2,

    File = 1u << 3,
    Line = 1u << 4,
    Function = 1u << 5,

    ProcessId = 1u << 6,
    ThreadId = 1u << 7,

    Compact =
            static_cast<std::uint32_t>(Timestamp) | static_cast<std::uint32_t>(Level) | static_cast<std::uint32_t>(Message),

    Source =
            static_cast<std::uint32_t>(Timestamp) | static_cast<std::uint32_t>(Level) | static_cast<std::uint32_t>(File) | static_cast<std::uint32_t>(Line) | static_cast<std::uint32_t>(Message),

    SourceFunc =
            static_cast<std::uint32_t>(Timestamp) | static_cast<std::uint32_t>(Level) | static_cast<std::uint32_t>(File) | static_cast<std::uint32_t>(Line) | static_cast<std::uint32_t>(Function) | static_cast<std::uint32_t>(Message),

    Threaded =
            static_cast<std::uint32_t>(Timestamp) | static_cast<std::uint32_t>(Level) | static_cast<std::uint32_t>(ProcessId) | static_cast<std::uint32_t>(ThreadId) | static_cast<std::uint32_t>(File) | static_cast<std::uint32_t>(Line) | static_cast<std::uint32_t>(Function) | static_cast<std::uint32_t>(Message)
};

[[nodiscard]] constexpr PatternStyle operator|(PatternStyle lhs, PatternStyle rhs) noexcept
{
    return static_cast<PatternStyle>(
            static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

[[nodiscard]] constexpr PatternStyle operator&(PatternStyle lhs, PatternStyle rhs) noexcept
{
    return static_cast<PatternStyle>(static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs));
}

constexpr PatternStyle& operator|=(PatternStyle& lhs, PatternStyle rhs) noexcept
{
    lhs = lhs | rhs;
    return lhs;
}

[[nodiscard]] constexpr bool hasPatternField(PatternStyle style, PatternStyle field) noexcept
{
    return (style & field) != PatternStyle::None;
}

struct LogEvent
{
        Level level{Level::Info};
        std::string message;
        std::uint64_t pid{0};
        std::uint64_t tid{0};
        std::int64_t timestampUs{0};
        std::source_location sourceLocation{};
};

namespace detail
{
[[nodiscard]] LogEvent makeLogEvent(Level level, std::string_view message, const std::source_location& location, PatternStyle style);
}

class Formatter
{
    public:
        Formatter() = default;
        explicit Formatter(PatternStyle style);

        void setStyle(PatternStyle style) noexcept;
        [[nodiscard]] PatternStyle style() const noexcept;

        [[nodiscard]] std::string format(const LogEvent& event) const;

    private:
        PatternStyle m_style{PatternStyle::Source};
};

class Logger
{
    public:
        Logger();
        explicit Logger(PatternStyle style);

        void setLevel(Level level) noexcept;
        [[nodiscard]] Level level() const noexcept;
        [[nodiscard]] bool shouldLog(Level level) const noexcept;

        void setPatternStyle(PatternStyle style) noexcept;
        [[nodiscard]] PatternStyle patternStyle() const noexcept;

        void setFlushOn(Level level) noexcept;
        [[nodiscard]] Level flushOn() const noexcept;

        void addSink(std::shared_ptr<Sink> sink);
        void clearSinks();
        void flush();

        void log(Level level, std::string_view message, const std::source_location& location = std::source_location::current());

        template <typename... Args>
        void logf(Level level, const std::source_location& location, std::format_string<Args...> fmt, Args&&... args)
        {
            if (!shouldLog(level))
            {
                return;
            }

            log(level, std::format(fmt, std::forward<Args>(args)...), location);
        }

    private:
        mutable std::mutex m_mutex;
        std::atomic<Level> m_level{Level::Info};
        std::atomic<Level> m_flushOn{Level::Error};
        Formatter m_formatter;
        std::vector<std::shared_ptr<Sink>> m_sinks;
};

[[nodiscard]] const char* toString(Level level) noexcept;
[[nodiscard]] const char* toSpdlogString(Level level) noexcept;

Logger& defaultLogger();
void setDefaultLevel(Level level) noexcept;
void setDefaultPatternStyle(PatternStyle style) noexcept;
void setDefaultFlushOn(Level level) noexcept;
void addDefaultSink(std::shared_ptr<Sink> sink);
void resetDefaultSinks();
void flushDefaultLogger();
void clearDefaultSinks();

void log(Level level, std::string_view message, const std::source_location& location = std::source_location::current());

template <typename... Args>
void logf(Level level, const std::source_location& location, std::format_string<Args...> fmt, Args&&... args)
{
    if (!defaultLogger().shouldLog(level))
    {
        return;
    }

    defaultLogger().log(level, std::format(fmt, std::forward<Args>(args)...), location);
}
}  // namespace dbase::log

#define DBASE_LOG_TRACE(...) \
    ::dbase::log::logf(::dbase::log::Level::Trace, std::source_location::current(), __VA_ARGS__)
#define DBASE_LOG_DEBUG(...) \
    ::dbase::log::logf(::dbase::log::Level::Debug, std::source_location::current(), __VA_ARGS__)
#define DBASE_LOG_INFO(...) \
    ::dbase::log::logf(::dbase::log::Level::Info, std::source_location::current(), __VA_ARGS__)
#define DBASE_LOG_WARN(...) \
    ::dbase::log::logf(::dbase::log::Level::Warn, std::source_location::current(), __VA_ARGS__)
#define DBASE_LOG_ERROR(...) \
    ::dbase::log::logf(::dbase::log::Level::Error, std::source_location::current(), __VA_ARGS__)
#define DBASE_LOG_FATAL(...) \
    ::dbase::log::logf(::dbase::log::Level::Fatal, std::source_location::current(), __VA_ARGS__)