#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <type_traits>

#pragma push_macro("DEBUG")
#pragma push_macro("ERROR")
#pragma push_macro("WARNING")
#pragma push_macro("INFO")
#undef DEBUG
#undef ERROR
#undef WARNING
#undef INFO

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static Logger& GetInstance() {
        static Logger instance;
        return instance;
    }

    void SetLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        level_ = level;
    }

    // ==== 新增：更底层的LogEx，包含 file / line / func ====
    void LogEx(LogLevel level, const char* file, int line, const char* func, const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);

        if ((int)level < (int)level_) return;

        std::ostringstream line_stream;
        std::time_t t = std::time(nullptr);
        std::tm tm{};

#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif

        line_stream << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] ";

        switch (level) {
            case LogLevel::Debug:   line_stream << "DEBUG";   break;
            case LogLevel::Info:    line_stream << "INFO";    break;
            case LogLevel::Warning: line_stream << "WARNING"; break;
            case LogLevel::Error:   line_stream << "ERROR";   break;
        }

        // 打印 (file:line func)
        line_stream << " (" << file << ":" << line << " " << func << ") ";

        line_stream << msg;

        std::cout << line_stream.str() << std::endl;

        if (file_.is_open()) {
            file_ << line_stream.str() << std::endl;
        }
    }

    // ===== 上层封装，支持 {} 格式化 =====
    template<typename... Args>
    void Info(const char* file, int line, const char* func,
              std::string_view fmt, Args&&... args) {
        LogEx(LogLevel::Info, file, line, func, Format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void Warn(const char* file, int line, const char* func,
              std::string_view fmt, Args&&... args) {
        LogEx(LogLevel::Warning, file, line, func, Format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void Error(const char* file, int line, const char* func,
               std::string_view fmt, Args&&... args) {
        LogEx(LogLevel::Error, file, line, func, Format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void Debug(const char* file, int line, const char* func,
               std::string_view fmt, Args&&... args) {
        LogEx(LogLevel::Debug, file, line, func, Format(fmt, std::forward<Args>(args)...));
    }

private:
    std::ofstream file_;
    LogLevel level_ = LogLevel::Info;
    std::mutex mutex_;

    Logger() {
        std::ostringstream filename;
        std::time_t t = std::time(nullptr);
        std::tm tm{};

#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        filename << "log/MSBot_" << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") << ".log";

#ifdef _WIN32
        system("if not exist log mkdir log");
#else
        system("mkdir -p log");
#endif

        file_.open(filename.str(), std::ios::out | std::ios::trunc);
        if (!file_.is_open()) {
            std::cerr << "Failed to open log file!" << std::endl;
        }
    }

    ~Logger() {
        if (file_.is_open()) file_.close();
    }

    // ------------ 格式化函数 ------------
    std::string Format(std::string_view format) {
        return std::string(format);
    }

    template<typename T, typename... Args>
    std::string Format(std::string_view format, T&& value, Args&&... args) {
        std::string fmt(format);
        size_t pos = fmt.find("{}");

        if (pos == std::string::npos) {
            return fmt;
        }

        std::string result =
            fmt.substr(0, pos)
            + ToString(std::forward<T>(value))
            + Format(fmt.substr(pos + 2), std::forward<Args>(args)...);

        return result;
    }

    template<typename T>
    std::string ToString(T&& value) {
        if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
            return value;
        } else if constexpr (std::is_same_v<std::decay_t<T>, const char*> ||
                             std::is_same_v<std::decay_t<T>, char*>) {
            return std::string(value);
        } else if constexpr (std::is_same_v<std::decay_t<T>, std::string_view>) {
            return std::string(value);
        } else if constexpr (std::is_arithmetic_v<std::decay_t<T>>) {
            return std::to_string(value);
        } else {
            std::ostringstream oss;
            oss << value;
            return oss.str();
        }
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};


// ====== 自动传入 file / line / func 的宏 ======
#define LOG_INFO(fmt, ...)    Logger::GetInstance().Info(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) Logger::GetInstance().Warn(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)   Logger::GetInstance().Error(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)   Logger::GetInstance().Debug(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#pragma pop_macro("INFO")
#pragma pop_macro("WARNING")
#pragma pop_macro("ERROR")
#pragma pop_macro("DEBUG")
