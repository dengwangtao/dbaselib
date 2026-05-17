#include "dbase/platform/process.h"

#include <string>
#include <thread>

#if defined(_WIN32)
#include <Windows.h>
#include <tlhelp32.h>
#else
#include <unistd.h>
#endif

#if defined(__linux__)
#include <sys/syscall.h>
#endif

namespace dbase::platform
{
std::uint32_t pid() noexcept
{
    static const std::uint32_t cached_pid = []() noexcept
    {
#if defined(_WIN32)
        return static_cast<std::uint32_t>(::GetCurrentProcessId());
#else
        return static_cast<std::uint32_t>(::getpid());
#endif
    }();

    return cached_pid;
}

std::uint32_t ppid() noexcept
{
#if defined(_WIN32)
    const DWORD currentPid = ::GetCurrentProcessId();
    const HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    PROCESSENTRY32 entry{};
    entry.dwSize = sizeof(entry);

    if (!::Process32First(snapshot, &entry))
    {
        ::CloseHandle(snapshot);
        return 0;
    }

    do
    {
        if (entry.th32ProcessID == currentPid)
        {
            ::CloseHandle(snapshot);
            return static_cast<std::uint32_t>(entry.th32ParentProcessID);
        }
    } while (::Process32Next(snapshot, &entry));

    ::CloseHandle(snapshot);
    return 0;
#else
    return static_cast<std::uint32_t>(::getppid());
#endif
}

std::uint64_t tid() noexcept
{
    thread_local const std::uint64_t cached_tid = []() noexcept
    {
#if defined(_WIN32)
        return static_cast<std::uint64_t>(::GetCurrentThreadId());
#elif defined(__linux__)
        return static_cast<std::uint64_t>(::syscall(SYS_gettid));
#else
        return static_cast<std::uint64_t>(
                std::hash<std::thread::id>{}(std::this_thread::get_id()));
#endif
    }();

    return cached_tid;
}

dbase::Result<std::filesystem::path> executablePath()
{
    static const auto cached_path = []() -> dbase::Result<std::filesystem::path>
    {
#if defined(_WIN32)
        std::wstring buffer(1024, L'\0');

        for (;;)
        {
            const DWORD len = ::GetModuleFileNameW(
                    nullptr,
                    buffer.data(),
                    static_cast<DWORD>(buffer.size()));

            if (len == 0)
            {
                return dbase::Result<std::filesystem::path>(
                        dbase::Error(
                                dbase::ErrorCode::SystemError,
                                "GetModuleFileNameW failed"));
            }

            if (len < buffer.size())
            {
                buffer.resize(len);
                return dbase::Result<std::filesystem::path>(
                        std::filesystem::path(buffer));
            }

            buffer.resize(buffer.size() * 2);
        }

#elif defined(__linux__)
        std::string buffer(1024, '\0');

        for (;;)
        {
            const auto len = ::readlink(
                    "/proc/self/exe",
                    buffer.data(),
                    buffer.size());

            if (len < 0)
            {
                return dbase::Result<std::filesystem::path>(
                        dbase::Error(
                                dbase::ErrorCode::SystemError,
                                "readlink /proc/self/exe failed"));
            }

            if (static_cast<std::size_t>(len) < buffer.size())
            {
                buffer.resize(static_cast<std::size_t>(len));
                return dbase::Result<std::filesystem::path>(
                        std::filesystem::path(buffer));
            }

            buffer.resize(buffer.size() * 2);
        }

#else
        return dbase::Result<std::filesystem::path>(
                dbase::Error(
                        dbase::ErrorCode::NotSupported,
                        "executablePath not supported on this platform"));
#endif
    }();

    return cached_path;
}

}  // namespace dbase::platform