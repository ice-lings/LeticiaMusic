#ifndef LOGGER_V2_H
#define LOGGER_V2_H

#include <fstream>

#include <string>
#include <chrono>
#include <thread>
#include <memory>
#include <sstream>
#include <utility>
#include <ctime>
#include <iostream>

#include <QObject>
#include <QString>
#include <QtGlobal>
#include <QDateTime>

#include "singleton_holder.h"

class Logger : public QObject, public SingletonHolder<Logger>
{
    Q_OBJECT
    friend class SingletonHolder<Logger>;
    
public:
    // 测试支持：设置测试实例（允许在单元测试中替换Logger实例）
    static void set_test_instance(Logger* instance) { s_testInstance = instance; }
    
    // 测试支持：清除测试实例，恢复默认单例
    static void clear_test_instance() { s_testInstance = nullptr; }
    
    // 重写get_instance以支持测试实例替换
    static Logger& get_instance() {
        if (s_testInstance) {
            return *s_testInstance;
        }
        return SingletonHolder<Logger>::get_instance();
    }

public:
    enum class Level {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Critical = 4
    };
    Q_ENUM(Level)

    // 初始化配置
    void init(const std::string& logDir = "log",
              Level minLevel = Level::Info,
              bool consoleOutput = true);

    // Windows Debug: 附加 stdout 到父进程控制台（VSCode 终端可见日志）
    static void attachDebugConsole();
    
    // 核心API - 格式化日志（支持可变参数）
    template<typename... Args>
    void debug(const std::string& format, Args&&... args) {
        logFormat(Level::Debug, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void info(const std::string& format, Args&&... args) {
        logFormat(Level::Info, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void warning(const std::string& format, Args&&... args) {
        logFormat(Level::Warning, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void error(const std::string& format, Args&&... args) {
        logFormat(Level::Error, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void critical(const std::string& format, Args&&... args) {
        logFormat(Level::Critical, format, std::forward<Args>(args)...);
    }
    
    // Qt字符串重载
    void debug(const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);
    void critical(const QString& message);
    
    // 带位置信息的日志（用于替换LOG_*宏）
    void debug(const QString& message, const char* file, int line);
    void info(const QString& message, const char* file, int line);
    void warning(const QString& message, const char* file, int line);
    void error(const QString& message, const char* file, int line);
    
    // 设置日志级别
    void setMinLevel(Level level) { m_minLevel = level; }
    Level minLevel() const { return m_minLevel; }
    
    // 获取当前日志文件路径
    std::string currentLogPath() const;
    
    // Qt消息处理
    static void qtMessageHandler(QtMsgType type, 
                                 const QMessageLogContext& context, 
                                 const QString& msg);
    static void installQtMessageHandler();
    
    // 刷新日志缓冲区
    void flush();

protected:
    // 构造函数和析构函数设为protected，允许子类继承（用于测试）
    explicit Logger(QObject* parent = nullptr);
    ~Logger();

private:
    // 格式化日志实现
    template<typename... Args>
    void logFormat(Level level, const std::string& format, Args&&... args);
    
    // 内部日志实现（virtual，允许子类重写用于测试）
    virtual void logImpl(Level level, const std::string& message,
                         const char* file = nullptr, int line = 0);
    
    // 获取格式化时间戳
    std::string getTimestamp() const;
    
    // 获取线程ID字符串
    std::string getThreadId() const;
    
    // 级别转字符串
    std::string levelToString(Level level) const;
    
    // 确保日志文件打开（按日期切换）
    void ensureLogFileOpen();
    
    // 根据当前日期生成日志文件路径
    std::string generateLogPath() const;
    
    // 辅助函数：格式化参数
    template<typename T>
    void formatArg(std::ostringstream& ss, T&& arg);
    
private:
    // 测试支持：测试实例指针（nullptr表示使用默认单例）
    static Logger* s_testInstance;
    
    // 成员变量
    std::string m_logDir;
    std::ofstream m_logFile;

    Level m_minLevel = Level::Info;
    bool m_consoleOutput = true;
    int m_currentYear = 0;
    int m_currentMonth = 0;
    int m_currentDay = 0;
};

// 全局访问宏
#define Log Logger::get_instance()

// 模板函数实现（必须在头文件中）
template<typename... Args>
void Logger::logFormat(Level level, const std::string& format, Args&&... args)
{
    if (level < m_minLevel) return;

    std::ostringstream ss;
    formatArg(ss, format);

    ((formatArg(ss, std::forward<Args>(args))), ...);

    logImpl(level, ss.str());
}

template<typename T>
void Logger::formatArg(std::ostringstream& ss, T&& arg)
{
    if constexpr (std::is_same_v<std::decay_t<T>, QString>) {
        ss << arg.toStdString();
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        ss << arg;
    } else {
        ss << arg;
    }
}

#endif // LOGGER_V2_H