#ifndef DATABASE_EXCEPTION_H
#define DATABASE_EXCEPTION_H

#include <stdexcept>
#include <QString>

namespace db {

/**
 * @brief 数据库错误代码枚举
 */
enum class DatabaseErrorCode {
    Success = 0,
    NotConnected,        // 数据库未连接
    ConnectionFailed,    // 连接失败
    QueryFailed,         // 查询执行失败
    PrepareFailed,       // SQL预处理失败
    TransactionFailed,   // 事务失败
    ConstraintViolation, // 约束违反（唯一性等）
    NotFound,           // 记录未找到
    Unknown             // 未知错误
};

/**
 * @brief 数据库异常类
 * 
 * 提供详细的错误信息、错误代码和原始SQL语句
 */
class DatabaseException : public std::runtime_error {
public:
    DatabaseException(DatabaseErrorCode code, 
                      const QString& message,
                      const QString& sql = QString())
        : std::runtime_error(message.toStdString())
        , m_errorCode(code)
        , m_message(message)
        , m_sql(sql)
    {}

    DatabaseErrorCode errorCode() const { return m_errorCode; }
    QString message() const { return m_message; }
    QString sql() const { return m_sql; }

    /**
     * @brief 获取人类可读的错误描述
     */
    QString toString() const {
        QString result = QString("DatabaseException [%1]: %2")
            .arg(static_cast<int>(m_errorCode))
            .arg(m_message);
        if (!m_sql.isEmpty()) {
            result += QString(" | SQL: %1").arg(m_sql);
        }
        return result;
    }

private:
    DatabaseErrorCode m_errorCode;
    QString m_message;
    QString m_sql;
};

} // namespace db

#endif // DATABASE_EXCEPTION_H
