#ifndef IDATABASESERVICE_H
#define IDATABASESERVICE_H

#include <QObject>
#include <QSqlQuery>
#include <QVariantMap>
#include <QVector>
#include <QStringList>

#include "../utils/query_options.h"

class IDatabaseService : public QObject {
    Q_OBJECT
public:
    explicit IDatabaseService(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IDatabaseService() = default;

//    virtual bool execute(const QString &query, const QVariantMap &params = QVariantMap()) = 0;

//     // 表管理

    /**
     * @brief 创建数据库表格
     * @param tableName 表名称
     * @param columns 列定义列表（每个元素为"列名 数据类型 约束"）
     * @param useTransaction 是否使用事务（默认true）
     * @return 是否创建成功
     */
    virtual bool createTable(const QString &tableName, const QStringList &columns, bool useTransaction = true) = 0;

    /**
     * @brief 检查表格是否存在
     * @param tableName 表名称
     * @return 是否存在表格
     */
    virtual bool tableExists(const QString &tableName) const = 0;

    /**
     * @brief 检查表格是否为空表格
     * @param tableName 表名称
     * @return 是否为空表格
     */
    virtual bool isTableEmpty(const QString& tableName) const = 0;

//     virtual bool dropTable(const QString &tableName) = 0;

//     // CRUD 操作
//     virtual QSqlQuery executeQuery(const QString &query, const QVariantMap &params = QVariantMap()) const = 0;

    virtual int insert(const QString &tableName, const QVariantMap &data) = 0;

    virtual int insertIgnore(const QString &tableName, const QVariantMap &data) = 0;

    virtual bool beginTransaction() = 0;
    virtual bool commitTransaction() = 0;
    virtual bool rollbackTransaction() = 0;

    /**
     * @brief 更新数据库记录
     * @param tableName 目标表
     * @param data 字段->值（参与 SET）
     * @param whereClause WHERE 子句（例如： "id = :id"）
     * @param whereParams WHERE 参数（例如：{ "id" : 3 }）
     * @return 更新成功（受影响行数 > 0）
     */
    virtual bool update(const QString &tableName,
                         const QVariantMap &data,
                         const QString &whereClause,
                         const QVariantMap &whereParams) = 0;

    /**
     * @brief 删除数据库记录
     * @param tableName 目标表
     * @param whereClause WHERE 子句（例如： "id = :id"）
     * @param whereParams WHERE 参数（例如：{ "id" : 3 }）
     * @return 删除成功（受影响行数 > 0）
     */
    virtual bool remove(const QString &tableName, const QString &whereClause, const QVariantMap &whereParams) = 0;

    virtual QVector<QVariantMap> select(const QString &tableName, const QueryOptions &options = QueryOptions()) const = 0;

    /**
     * @brief 执行原始SQL语句（用于DDL迁移等，不返回结果集）
     * @param sql 完整的SQL语句
     * @return 执行成功返回true
     */
    virtual bool executeRaw(const QString &sql) = 0;

    virtual bool executeUpdate(const QString &sql, const QVariantMap &params) = 0;

//     virtual bool verifyTableSchema(const QString &tableName, const QStringList &requiredColumns) const = 0;
//     // 新增索引管理方法
//     virtual bool createIndex(const QString &indexName, const QString &tableName, const QStringList &columns, bool unique = false) = 0;
//     virtual bool indexExists(const QString &indexName) const = 0;
//     virtual QStringList getTableColumns(const QString &tableName) const = 0;

//     // 实用方法
    virtual QString lastError() const = 0;
//     virtual QString databasePath() const = 0;

    virtual QVariantMap queryToVariantMap(const QSqlQuery &query) const = 0;

// signals:
//     void connectionEstablished();
//     void connectionLost();
};

#endif // IDATABASESERVICE_H
