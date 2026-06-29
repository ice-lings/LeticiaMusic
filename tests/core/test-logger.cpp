#include <QtTest/QtTest>
#include <QCoreApplication>
#include "../../src/core/utils/logger.h"

/**
 * @brief TestLogger - 用于测试的简单Logger派生类
 * 
 * 重写logImpl来捕获所有日志调用。
 */
class TestLogger : public Logger
{
    Q_OBJECT
public:
    struct LogEntry {
        Logger::Level level;
        std::string message;
        std::string file;
        int line;
    };

    TestLogger(QObject* parent = nullptr) : Logger(parent) {}
    
    // 重写logImpl来捕获日志
    void logImpl(Level level, const std::string& message,
                 const char* file = nullptr, int line = 0) override {
        LogEntry entry;
        entry.level = level;
        entry.message = message;
        entry.file = file ? file : "";
        entry.line = line;
        m_logs.push_back(entry);
    }
    
    // 测试辅助方法
    void clearLogs() { m_logs.clear(); }
    size_t logCount() const { return m_logs.size(); }
    
    bool hasLog(Level level, const std::string& messageSubstr) const {
        for (const auto& entry : m_logs) {
            if (entry.level == level && entry.message.find(messageSubstr) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
    
    const std::vector<LogEntry>& logs() const { return m_logs; }
    
    LogEntry getLastLog() const { 
        return m_logs.empty() ? LogEntry{} : m_logs.back(); 
    }

private:
    std::vector<LogEntry> m_logs;
};

/**
 * @brief TestLoggerClass - 测试Logger的测试替换功能
 */
class TestLoggerClass: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {
        // 确保测试开始时Logger处于干净状态
        Logger::clear_test_instance();
    }
    
    void cleanupTestCase() {
        // 测试结束后清理测试实例
        Logger::clear_test_instance();
    }
    
    void testLoggerSingletonReplacement() {
        // 测试Logger单例替换功能
        TestLogger* testLogger = new TestLogger(this);
        
        // 保存原始实例引用
        Logger& originalLogger = Logger::get_instance();
        
        // 设置测试实例
        Logger::set_test_instance(testLogger);
        
        // 验证现在获取到的是测试实例
        Logger& currentLogger = Logger::get_instance();
        QVERIFY(&currentLogger == testLogger);
        
        // 记录一些日志
        Log.info("Test info message");
        Log.warning("Test warning message");
        
        // 验证测试Logger记录了日志
        QCOMPARE(testLogger->logCount(), 2);
        QVERIFY(testLogger->hasLog(Logger::Level::Info, "Test info message"));
        QVERIFY(testLogger->hasLog(Logger::Level::Warning, "Test warning message"));
        
        // 清理测试实例
        Logger::clear_test_instance();
        
        // 验证恢复为原始实例
        Logger& restoredLogger = Logger::get_instance();
        QVERIFY(&restoredLogger == &originalLogger);
        
        // 清理
        delete testLogger;
    }
    
    void testLogMacroWorksWithTestInstance() {
        // 测试Log宏与测试实例一起工作
        TestLogger* testLogger = new TestLogger(this);
        testLogger->setMinLevel(Logger::Level::Debug);  // 设置为Debug级别以记录所有日志
        Logger::set_test_instance(testLogger);
        
        // 使用Log宏记录日志
        Log.debug("Debug message via macro");
        Log.error("Error message via macro");
        
        // 验证
        QCOMPARE(testLogger->logCount(), 2);
        QVERIFY(testLogger->hasLog(Logger::Level::Debug, "Debug message via macro"));
        QVERIFY(testLogger->hasLog(Logger::Level::Error, "Error message via macro"));
        
        // 清理
        Logger::clear_test_instance();
        delete testLogger;
    }
    
    void testLoggerLevelFiltering() {
        // 测试日志级别过滤
        TestLogger* testLogger = new TestLogger(this);
        testLogger->setMinLevel(Logger::Level::Warning);
        Logger::set_test_instance(testLogger);
        
        // 记录不同级别的日志
        Log.debug("This debug should be filtered out");
        Log.info("This info should be filtered out");
        Log.warning("This warning should be recorded");
        Log.error("This error should be recorded");
        
        // 只有warning和error级别的日志应该被记录
        QCOMPARE(testLogger->logCount(), 2);
        QVERIFY(testLogger->hasLog(Logger::Level::Warning, "This warning should be recorded"));
        QVERIFY(testLogger->hasLog(Logger::Level::Error, "This error should be recorded"));
        QVERIFY(!testLogger->hasLog(Logger::Level::Debug, "This debug should be filtered out"));
        QVERIFY(!testLogger->hasLog(Logger::Level::Info, "This info should be filtered out"));
        
        // 清理
        Logger::clear_test_instance();
        delete testLogger;
    }
    
    void testMultipleTestInstances() {
        // 测试多个测试实例替换
        TestLogger* testLogger1 = new TestLogger(this);
        TestLogger* testLogger2 = new TestLogger(this);
        
        // 设置第一个测试实例
        Logger::set_test_instance(testLogger1);
        Log.info("Message to logger 1");
        QCOMPARE(testLogger1->logCount(), 1);
        QCOMPARE(testLogger2->logCount(), 0);
        
        // 替换为第二个测试实例
        Logger::set_test_instance(testLogger2);
        Log.info("Message to logger 2");
        QCOMPARE(testLogger1->logCount(), 1); // 应该保持不变
        QCOMPARE(testLogger2->logCount(), 1);
        
        // 清理测试实例
        Logger::clear_test_instance();
        
        delete testLogger1;
        delete testLogger2;
    }
    
    void testLogWithFileAndLine() {
        // 测试带文件和行号的日志
        TestLogger* testLogger = new TestLogger(this);
        Logger::set_test_instance(testLogger);
        
        // 记录带位置信息的日志
        Log.info("Test message with location", __FILE__, __LINE__);
        
        // 验证
        QCOMPARE(testLogger->logCount(), 1);
        auto lastLog = testLogger->getLastLog();
        QVERIFY(lastLog.file.find("test-logger.cpp") != std::string::npos);
        QVERIFY(lastLog.line > 0);
        QVERIFY(testLogger->hasLog(Logger::Level::Info, "Test message with location"));
        
        // 清理
        Logger::clear_test_instance();
        delete testLogger;
    }
};

QTEST_MAIN(TestLoggerClass)
#include "test-logger.moc"