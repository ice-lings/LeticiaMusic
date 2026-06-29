#include "logger.h"

// 静态成员变量定义
Logger* Logger::s_testInstance = nullptr;

#include <QDateTime>
#include <QThread>
#include <QFileInfo>

#include <QIODevice>
#include <QDir>
#include <QCoreApplication>

#include <iostream>
#include <iomanip>

#ifdef Q_OS_WIN
#ifdef QT_DEBUG
#include <windows.h>
#endif
#endif
#include <chrono>
#include <ctime>
#include <sstream>
#include <mutex>

#ifdef Q_OS_ANDROID
#include <android/log.h>
#endif

void Logger::attachDebugConsole()
{
#ifdef Q_OS_WIN
#ifdef QT_DEBUG
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        SetConsoleOutputCP(CP_UTF8);
        std::cout.clear();
        std::cerr.clear();
    }
#endif
#endif
}

Logger::Logger(QObject* parent) : QObject(parent)
{
    // 默认设置
    m_consoleOutput = true;
}

Logger::~Logger()
{
    flush();
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

void Logger::init(const std::string& logDir, Level minLevel, bool consoleOutput)
{
    
    m_logDir = logDir;
    m_minLevel = minLevel;
    m_consoleOutput = consoleOutput;
    
    // 创建日志目录
    QDir dir(QString::fromStdString(logDir));
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // 设置当前日期
    QDate currentDate = QDate::currentDate();
    m_currentYear = currentDate.year();
    m_currentMonth = currentDate.month();
    m_currentDay = currentDate.day();
    
    // 确保日志文件打开
    ensureLogFileOpen();
    
    // 输出初始化信息
    info(QString("Logger initialized. Log directory: %1, Min level: %2")
         .arg(QString::fromStdString(logDir))
         .arg(static_cast<int>(minLevel)));
}

void Logger::logImpl(Level level, const std::string& message, const char* file, int line)
{
    static std::mutex logMutex;
    std::lock_guard<std::mutex> lock(logMutex);
    ensureLogFileOpen();
    if (!m_logFile.is_open()) {
        return;
    }
    
    // 构建日志条目
    std::ostringstream logEntry;
    logEntry << "[" << getTimestamp() << "] "
             << "[T" << getThreadId() << "] "
             << "[" << levelToString(level) << "]";
    
    // 添加文件和行号信息（如果有）
    if (file && line > 0) {
        std::string fileStr(file);
        size_t lastSlash = fileStr.find_last_of("/\\");
        std::string filename = (lastSlash != std::string::npos) ? fileStr.substr(lastSlash + 1) : fileStr;
        logEntry << " [" << filename << ":" << line << "]";
    }
    
    logEntry << " " << message << "\n";
    
    std::string logStr = logEntry.str();
    
    // 输出到控制台
    if (m_consoleOutput) {
        std::cout << logStr;
        std::cout.flush();
    }

#ifdef Q_OS_ANDROID
    // 同时输出到 Android logcat
    {
        int androidLevel;
        switch (level) {
        case Level::Debug:    androidLevel = ANDROID_LOG_DEBUG; break;
        case Level::Warning:  androidLevel = ANDROID_LOG_WARN;  break;
        case Level::Error:    androidLevel = ANDROID_LOG_ERROR; break;
        case Level::Critical: androidLevel = ANDROID_LOG_FATAL; break;
        default:              androidLevel = ANDROID_LOG_INFO;  break;
        }
        __android_log_print(androidLevel, "Leticia", "%s", message.c_str());
    }
#endif

    // 输出到文件
    m_logFile << logStr;
    m_logFile.flush();
}

std::string Logger::getTimestamp() const
{
    QDateTime now = QDateTime::currentDateTime();
    return now.toString("yyyy-MM-dd hh:mm:ss.zzz").toStdString();
}

std::string Logger::getThreadId() const
{
    std::ostringstream ss;
    ss << std::hex << reinterpret_cast<uintptr_t>(QThread::currentThread());
    return ss.str();
}

std::string Logger::levelToString(Level level) const
{
    switch (level) {
    case Level::Debug: return "DEBUG";
    case Level::Info: return "INFO";
    case Level::Warning: return "WARN";
    case Level::Error: return "ERROR";
    case Level::Critical: return "CRITICAL";
    default: return "UNKNOWN";
    }
}

void Logger::ensureLogFileOpen()
{
    // 检查日期是否变化
    QDate currentDate = QDate::currentDate();
    int year = currentDate.year();
    int month = currentDate.month();
    int day = currentDate.day();
    
    // 如果日期变化或文件未打开，重新打开文件
    if (year != m_currentYear || month != m_currentMonth || day != m_currentDay || !m_logFile.is_open()) {
        if (m_logFile.is_open()) {
            m_logFile.close();
        }
        
        m_currentYear = year;
        m_currentMonth = month;
        m_currentDay = day;
        
        std::string logPath = generateLogPath();
        
        // 以追加模式打开文件
        m_logFile.open(logPath, std::ios::app);
        if (!m_logFile.is_open()) {
            std::cerr << "Failed to open log file: " << logPath << std::endl;
        } else {
            // 添加文件头（如果是新文件）
            // 检查文件是否为空
            std::ifstream test(logPath);
            if (test.peek() == std::ifstream::traits_type::eof()) {
                m_logFile << "=== LeticiaMusic Log Started ===" << std::endl;
                m_logFile << "=== Date: " << getTimestamp() << " ===" << std::endl;
                m_logFile.flush();
            }
        }
    }
}

std::string Logger::generateLogPath() const
{
    // 生成文件名：LeticiaMusic-YYYY-MM-DD.log
    std::ostringstream filename;
    filename << "LeticiaMusic-"
             << m_currentYear << "-"
             << std::setfill('0') << std::setw(2) << m_currentMonth << "-"
             << std::setfill('0') << std::setw(2) << m_currentDay
             << ".log";
    
    // 构建完整路径
    QDir dir(QString::fromStdString(m_logDir));
    QString fullPath = dir.filePath(QString::fromStdString(filename.str()));
    return fullPath.toStdString();
}

std::string Logger::currentLogPath() const
{
    return generateLogPath();
}

void Logger::flush()
{
    if (m_logFile.is_open()) {
        m_logFile.flush();
    }
}

// Qt字符串重载实现
void Logger::debug(const QString& message)
{
    if (m_minLevel <= Level::Debug) {
        logImpl(Level::Debug, message.toStdString());
    }
}

void Logger::info(const QString& message)
{
    if (m_minLevel <= Level::Info) {
        logImpl(Level::Info, message.toStdString());
    }
}

void Logger::warning(const QString& message)
{
    if (m_minLevel <= Level::Warning) {
        logImpl(Level::Warning, message.toStdString());
    }
}

void Logger::error(const QString& message)
{
    if (m_minLevel <= Level::Error) {
        logImpl(Level::Error, message.toStdString());
    }
}

void Logger::critical(const QString& message)
{
    if (m_minLevel <= Level::Critical) {
        logImpl(Level::Critical, message.toStdString());
    }
}

// 带位置信息的日志实现（用于替换LOG_*宏）
void Logger::debug(const QString& message, const char* file, int line)
{
    if (m_minLevel <= Level::Debug) {
        logImpl(Level::Debug, message.toStdString(), file, line);
    }
}

void Logger::info(const QString& message, const char* file, int line)
{
    if (m_minLevel <= Level::Info) {
        logImpl(Level::Info, message.toStdString(), file, line);
    }
}

void Logger::warning(const QString& message, const char* file, int line)
{
    if (m_minLevel <= Level::Warning) {
        logImpl(Level::Warning, message.toStdString(), file, line);
    }
}

void Logger::error(const QString& message, const char* file, int line)
{
    if (m_minLevel <= Level::Error) {
        logImpl(Level::Error, message.toStdString(), file, line);
    }
}

// Qt消息处理
void Logger::qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Level level = Level::Info;
    switch (type) {
    case QtDebugMsg:
        level = Level::Debug;
        break;
    case QtInfoMsg:
        level = Level::Info;
        break;
    case QtWarningMsg:
        level = Level::Warning;
        break;
    case QtCriticalMsg:
        level = Level::Error;
        break;
    case QtFatalMsg:
        level = Level::Critical;
        break;
    }
    
    // 构建完整消息
    QString fullMessage = msg;
    if (context.file && context.line) {
        fullMessage += QString(" (File: %1, Line: %2, Function: %3)")
            .arg(context.file).arg(context.line).arg(context.function ? context.function : "unknown");
    }
    
    // 记录日志
    get_instance().logImpl(level, fullMessage.toStdString(), context.file, context.line);
    
    // 对于致命错误，退出程序
    if (type == QtFatalMsg) {
        std::abort();
    }
}

void Logger::installQtMessageHandler()
{
    qInstallMessageHandler(qtMessageHandler);
}