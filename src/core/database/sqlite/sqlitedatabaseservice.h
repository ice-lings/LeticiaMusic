#ifndef SQLITEDATABASESERVICE_H
#define SQLITEDATABASESERVICE_H

#include <QObject>
#include <QSqlDatabase>
#include <QMutex>
#include <QString>
#include <QVariantMap>
#include <QThread>
#include "../../interfaces/idatabaseservice.h"


class SqliteDatabaseService : public IDatabaseService
{
    Q_OBJECT
public:
    SqliteDatabaseService(const QString& dbPath = "", QObject *parent = nullptr);
    
    /**
     * @brief 获取底层QSqlDatabase引用（用于复杂查询）
     * @return QSqlDatabase引用
     */
    QSqlDatabase& getDatabase();

    /**
     * @brief 创建数据库表格
     * @param tableName 表名称
     * @param columns 列定义列表（每个元素为"列名 数据类型 约束"）
     * @param useTransaction 是否使用事务（默认true）
     * @return 是否创建成功
     */
    bool createTable(const QString &tableName, const QStringList &columns, bool useTransaction = true) override;

    /**
     * @brief 检查表格是否存在
     * @param tableName 表名称
     * @return 是否存在表格
     */
    bool tableExists(const QString &tableName) const override;

    /**
     * @brief 检查表格是否为空表格
     * @param tableName 表名称
     * @return 是否为空表格
     */
    bool isTableEmpty(const QString& tableName) const override;

    /**
     * @brief 向指定表插入一行数据
     * @param tableName 目标表名（注意：未做安全过滤，需确保来自可信源）
     * @param data 键值对形式的插入数据（字段名→值）
     * @return 插入成功返回自增主键ID，失败返回-1
     *
     * 示例：
     * insert("users", {{"name", "Alice"}, {"age", 25}})
     * 生成SQL: INSERT INTO users (name, age) VALUES (:name, :age)
    */
    int insert(const QString &tableName, const QVariantMap &data) override;

    int insertIgnore(const QString &tableName, const QVariantMap &data) override;

    bool beginTransaction() override;
    bool commitTransaction() override;
    bool rollbackTransaction() override;

    bool update(const QString &tableName,
                const QVariantMap &data,
                const QString &whereClause,
                const QVariantMap &whereParams) override;

    /**
     * @brief 删除数据库记录
     * @param tableName 目标表
     * @param whereClause WHERE 子句（例如： "id = :id"）
     * @param whereParams WHERE 参数（例如：{ "id" : 3 }）
     * @return 删除成功（受影响行数 > 0）
     */
    bool remove(const QString &tableName, const QString &whereClause, const QVariantMap &whereParams) override;

    /**
     * @brief 执行数据库表的查询操作并返回结果集
     * @param tableName 要查询的表名
     * @param options 查询选项对象，包含查询条件、排序等参数
     * @return 存储查询结果的QVector容器，每个元素为QVariantMap类型（字段名-值映射）
    */
    QVector<QVariantMap> select(const QString &tableName, const QueryOptions &options = QueryOptions()) const override;

    bool executeRaw(const QString &sql) override;
    bool executeUpdate(const QString &sql, const QVariantMap &params) override;


private:
    QSqlQuery executeQuery(const QString &query, const QVariantMap &params = QVariantMap()) const;

    QVariantMap queryToVariantMap(const QSqlQuery &query) const override;

    QString lastError() const override;

    QSqlDatabase m_database;
    QString m_databasePath;
    bool m_initialized = false;
    mutable QMutex m_mutex;
    QThread* m_creationThread = nullptr;
    
private:
    bool initializeDatabase();
    void ensureInitialized();
};

#endif // SQLITEDATABASESERVICE_H
