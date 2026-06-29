#include "sqlitedatabaseservice.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QDebug>
#include <QThread>
#include <QSqlRecord>
#include <QRegularExpression>
#include <QStandardPaths>

#include "../database_exception.h"

#include "../../utils/logger.h"

SqliteDatabaseService::SqliteDatabaseService(const QString& dbPath, QObject *parent):
    IDatabaseService(parent), m_databasePath(dbPath), m_creationThread(QThread::currentThread())
{
    if (m_databasePath.isEmpty()) {
#ifdef Q_OS_ANDROID
        m_databasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/musicplayer.db";
#else
        m_databasePath = QCoreApplication::applicationDirPath() + "/musicplayer.db";
#endif
    }
    QDir().mkpath(QFileInfo(m_databasePath).path());
}


bool SqliteDatabaseService::createTable(const QString &tableName, const QStringList &columns, bool useTransaction)
{
    // 延迟初始化
    ensureInitialized();

    if (!m_database.isValid() || !m_database.isOpen()) {
        Log.error("Database connection invalid or not open");
        return false;
    }

    if (columns.isEmpty()) {
        Log.warning("Column definitions cannot be empty");
        return false;
    }

    // 验证表名和列名安全（防止SQL注入）
    QRegularExpression validNameRegex("^[a-zA-Z_][a-zA-Z0-9_]*$");
    if (!validNameRegex.match(tableName).hasMatch()) {
        Log.error(QString("[DB] Invalid table name: %1").arg(tableName));
        return false;
    }
    
    // 验证列名安全
    for (const QString& column : columns) {
        QString columnName = column.section(' ', 0, 0).trimmed();
        if (!validNameRegex.match(columnName).hasMatch()) {
            Log.error(QString("[DB] Invalid column name: %1").arg(columnName));
            return false;
        }
    }

    // 构建CREATE TABLE语句
    QString sql = QString("CREATE TABLE IF NOT EXISTS %1 (").arg(tableName);
    sql += columns.join(", ");
    sql += ");";

    // 事务处理
    bool transactionActive = false;
    if (useTransaction && m_database.transaction()) {
        transactionActive = true;
    }

    QSqlQuery query(m_database);
    if (!query.exec(sql)) {
        QSqlError error = query.lastError();
        qCritical().nospace()
            << "Failed to create table " << tableName
            << "\n  SQL: " << sql
            << "\n  Error: " << error.text()
            << " (" << error.nativeErrorCode() << ")";

        if (transactionActive) {
            m_database.rollback();
        }

        return false;
    }

    if (transactionActive && !m_database.commit()) {
        qCritical() << "Commit failed:" << m_database.lastError().text();
        return false;
    }

    Log.debug(QString("Table ready: %1").arg(tableName));

    return true;
}


bool SqliteDatabaseService::tableExists(const QString &tableName) const {
    // 延迟初始化
    const_cast<SqliteDatabaseService*>(this)->ensureInitialized();

    if (!m_database.isOpen()) {
        return false;
    }

    return m_database.tables().contains(tableName, Qt::CaseInsensitive);
}


bool SqliteDatabaseService::isTableEmpty(const QString &tableName) const
{
    // 延迟初始化
    const_cast<SqliteDatabaseService*>(this)->ensureInitialized();

    if (!m_database.isOpen()) {
        Log.error("Database is not open");
        return false;
    }

    // 检查表是否存在
    if (!tableExists(tableName)) {
        Log.warning("Table does not exist: " + tableName);
        return false;
    }

    // 执行COUNT查询
    QSqlQuery query(m_database);
    query.prepare(QString("SELECT COUNT(*) FROM %1").arg(tableName));

    if (!query.exec()) {
        Log.error("Failed to execute query: " + query.lastError().text());
        return false;
    }

    // 获取查询结果
    if (query.next()) {
        int count = query.value(0).toInt();
        return (count == 0);
    }

    Log.warning("No result returned from COUNT query");
    return false;
}

QSqlQuery SqliteDatabaseService::executeQuery(const QString &queryStr, const QVariantMap &params) const{
    // 线程安全检查：确保数据库操作在创建线程中执行
    if (QThread::currentThread() != m_creationThread) {
        Log.error("[DB] Thread safety violation: Database query called from wrong thread");
        throw db::DatabaseException(
            db::DatabaseErrorCode::QueryFailed,
            QString("Database operations must be performed on the creation thread"),
            queryStr
        );
    }

    QMutexLocker locker(&m_mutex);

    // 检查数据库连接状态
    if (!m_database.isOpen()) {
        Log.error(QString("[DB] Database not open. Query: %1").arg(queryStr.left(100)));
        throw db::DatabaseException(
            db::DatabaseErrorCode::NotConnected,
            QString("Database is not connected"),
            queryStr
        );
    }
    
    // 准备查询
    QSqlQuery sqlQuery(m_database);
    if (!sqlQuery.prepare(queryStr)) {
        QString error = sqlQuery.lastError().text();
        Log.error(QString("[DB] Failed to prepare query: %1 | SQL: %2")
                      .arg(error.left(200))
                      .arg(queryStr.left(100)));
        throw db::DatabaseException(
            db::DatabaseErrorCode::PrepareFailed,
            QString("Failed to prepare SQL query: %1").arg(error),
            queryStr
        );
    }
    
    // 绑定参数
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        QString key = it.key().startsWith(':') ? it.key() : ":" + it.key();
        sqlQuery.bindValue(key, it.value());
    }
    
    // 执行查询
    if (!sqlQuery.exec()) {
        QString error = sqlQuery.lastError().text();
        Log.error(QString("[DB] Query execution failed: %1 | SQL: %2")
                      .arg(error.left(200))
                      .arg(queryStr.left(200)));
        
        // 根据错误类型分类
        db::DatabaseErrorCode code = db::DatabaseErrorCode::QueryFailed;
        if (error.contains("UNIQUE constraint failed", Qt::CaseInsensitive)) {
            code = db::DatabaseErrorCode::ConstraintViolation;
        } else if (error.contains("no such table", Qt::CaseInsensitive)) {
            code = db::DatabaseErrorCode::NotFound;
        }
        
        throw db::DatabaseException(
            code,
            QString("Query execution failed: %1").arg(error),
            queryStr
        );
    }

    // 成功时不打印日志（避免稀释关键日志）
    return sqlQuery;
}


int SqliteDatabaseService::insert(const QString &tableName, const QVariantMap &data) {
    // 延迟初始化
    ensureInitialized();

    if (data.isEmpty()) {
        Log.error(QString("[DB] Insert failed: data is empty for table: %1").arg(tableName));
        return -1;
    }

    try {
        // 1. 拼接SQL语句
        QStringList keys = data.keys();
        QString columns = keys.join(", ");
        QString placeholders = ":" + keys.join(", :");
        QString queryStr = QString("INSERT INTO %1 (%2) VALUES (%3)").arg(tableName,columns,placeholders);

        // 2. 执行查询
        QSqlQuery query = executeQuery(queryStr, data);

    // 3. 核心改造1：打印全量调试信息（SQL语句+绑定参数），方便排查
    qDebug() << "[SQL调试] 执行插入语句：" << queryStr;
    qDebug() << "[SQL调试] 绑定的参数：" << data;

    // 4. 核心改造2：修正插入成功的判断逻辑（你的表无自增ID，lastInsertId无效！）
    // SQLite中，非自增主键表插入成功的判断标准：查询激活+受影响行数>0
    bool insertSuccess = query.isActive() && query.numRowsAffected() > 0;
    if (insertSuccess) {
        qDebug() << "[SQL插入成功] 表：" << tableName << "，受影响行数：" << query.numRowsAffected();
        return query.lastInsertId().toInt();
    }

    // 5. 核心改造3：打印具体的失败原因（数据库原生错误，最关键！）
    QSqlError sqlError = query.lastError();
    qWarning() << "[SQL插入失败] ======================================";
    qWarning() << "[SQL插入失败] 表名：" << tableName;
    qWarning() << "[SQL插入失败] 错误类型：" << sqlError.type(); // 错误类型（比如主键冲突、语法错误）
    qWarning() << "[SQL插入失败] 错误编码：" << sqlError.nativeErrorCode();
    qWarning() << "[SQL插入失败] 错误文本（原生）：" << sqlError.text(); // 数据库原生错误描述
    qWarning() << "[SQL插入失败] 错误驱动文本：" << sqlError.driverText();
    qWarning() << "[SQL插入失败] ======================================";

    } catch (const db::DatabaseException& e) {
        Log.error(QString("[DB] Insert failed: %1").arg(e.toString()));
        return -1;
    }
    
    return -1;
}

int SqliteDatabaseService::insertIgnore(const QString &tableName, const QVariantMap &data) {
    ensureInitialized();

    if (data.isEmpty()) {
        Log.error(QString("[DB] InsertIgnore failed: data is empty for table: %1").arg(tableName));
        return -1;
    }

    try {
        QStringList keys = data.keys();
        QString columns = keys.join(", ");
        QString placeholders = ":" + keys.join(", :");
        QString queryStr = QString("INSERT OR IGNORE INTO %1 (%2) VALUES (%3)").arg(tableName, columns, placeholders);

        QSqlQuery query = executeQuery(queryStr, data);

        if (query.numRowsAffected() > 0) {
            QVariant lastId = query.lastInsertId();
            return lastId.isValid() ? lastId.toInt() : 0;
        }

        return -2;

    } catch (const db::DatabaseException& e) {
        Log.error(QString("[DB] InsertIgnore failed: %1").arg(e.toString()));
        return -1;
    }
}

bool SqliteDatabaseService::beginTransaction()
{
    ensureInitialized();
    QMutexLocker locker(&m_mutex);
    if (!m_database.isOpen()) {
        Log.error("[DB] Cannot begin transaction: database not open");
        return false;
    }
    return m_database.transaction();
}

bool SqliteDatabaseService::commitTransaction()
{
    QMutexLocker locker(&m_mutex);
    if (!m_database.isOpen()) {
        Log.error("[DB] Cannot commit transaction: database not open");
        return false;
    }
    return m_database.commit();
}

bool SqliteDatabaseService::rollbackTransaction()
{
    QMutexLocker locker(&m_mutex);
    if (!m_database.isOpen()) {
        Log.error("[DB] Cannot rollback transaction: database not open");
        return false;
    }
    return m_database.rollback();
}

bool SqliteDatabaseService::update(const QString &tableName,
                                    const QVariantMap &data,
                                    const QString &whereClause,
                                    const QVariantMap &whereParams)
{
    // 延迟初始化
    ensureInitialized();

    if (data.isEmpty()) {
        qWarning() << "[SQL更新失败] data is empty";
        return false;
    }
    if (!m_database.isOpen()) {
        Log.error("Database not open");
        return false;
    }
    if (whereClause.isEmpty()) {
        Log.error("Update failed: whereClause is empty (refuse to update whole table)");
        return false;
    }

    QMutexLocker locker(&m_mutex);

    QStringList setParts;
    setParts.reserve(data.size());
    for (const auto &key : data.keys()) {
        const QString cleanKey = key.startsWith(':') ? key.mid(1) : key;
        setParts.push_back(QString("%1 = :%2").arg(cleanKey, cleanKey));
    }

    QString sql = QString("UPDATE %1 SET %2 WHERE %3")
                      .arg(tableName, setParts.join(", "), whereClause);

    QSqlQuery query(m_database);
    if (!query.prepare(sql)) {
        Log.error("Update prepare failed: " + query.lastError().text());
        return false;
    }

    // SET params
    for (const auto &key : data.keys()) {
        const QString cleanKey = key.startsWith(':') ? key.mid(1) : key;
        query.bindValue(":" + cleanKey, data.value(key));
    }

    // WHERE params
    for (auto it = whereParams.constBegin(); it != whereParams.constEnd(); ++it) {
        QString key = it.key();
        QString cleanKey = key.startsWith(':') ? key.mid(1) : key;
        query.bindValue(":" + cleanKey, it.value());
    }

    if (!query.exec()) {
        Log.error("Update execution failed: " + query.lastError().text() + "\nQuery: " + sql);
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool SqliteDatabaseService::remove(const QString &tableName, const QString &whereClause, const QVariantMap &whereParams)
{
    // 延迟初始化
    ensureInitialized();

    if (!m_database.isOpen()) {
        Log.error("Database not open");
        return false;
    }
    if (whereClause.isEmpty()) {
        Log.error("Remove failed: whereClause is empty (refuse to delete all records)");
        return false;
    }

    QMutexLocker locker(&m_mutex);

    QString sql = QString("DELETE FROM %1 WHERE %2").arg(tableName, whereClause);

    QSqlQuery query(m_database);
    if (!query.prepare(sql)) {
        Log.error("Remove prepare failed: " + query.lastError().text());
        return false;
    }

    // 绑定WHERE参数
    for (auto it = whereParams.constBegin(); it != whereParams.constEnd(); ++it) {
        QString key = it.key();
        QString cleanKey = key.startsWith(':') ? key.mid(1) : key;
        query.bindValue(":" + cleanKey, it.value());
    }

    if (!query.exec()) {
        Log.error("Remove execution failed: " + query.lastError().text() + "\nQuery: " + sql);
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool SqliteDatabaseService::executeRaw(const QString &sql)
{
    ensureInitialized();

    if (!m_database.isOpen()) {
        Log.error("[DB] Database not open for executeRaw");
        return false;
    }

    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_database);
    if (!query.exec(sql)) {
        Log.debug(QString("[DB] executeRaw (non-critical): %1").arg(query.lastError().text()));
        return false;
    }
    return true;
}


bool SqliteDatabaseService::executeUpdate(const QString &sql, const QVariantMap &params)
{
    ensureInitialized();

    if (!m_database.isOpen()) {
        Log.error("[DB] Database not open for executeUpdate");
        return false;
    }

    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_database);
    if (!query.prepare(sql)) {
        Log.error("[DB] executeUpdate prepare failed: " + query.lastError().text());
        return false;
    }

    for (auto it = params.begin(); it != params.end(); ++it)
        query.bindValue(it.key(), it.value());

    if (!query.exec()) {
        Log.error("[DB] executeUpdate exec failed: " + query.lastError().text());
        return false;
    }
    return true;
}


QVector<QVariantMap> SqliteDatabaseService::select(const QString &tableName, const QueryOptions &options) const
{
    // 延迟初始化
    const_cast<SqliteDatabaseService*>(this)->ensureInitialized();

    QVector<QVariantMap> results;
    results.reserve(5);

    try {
        // 执行查询
        QSqlQuery query = executeQuery(options.buildSelectQuery(tableName), options.whereParams);

        // 处理结果
        while (query.next()) {
            results.append(queryToVariantMap(query));
        }
    } catch (const db::DatabaseException& e) {
        Log.error(QString("[DB] Select query failed: %1").arg(e.toString()));
    } catch (const std::exception& e) {
        Log.error(QString("[DB] Select query failed with unexpected error: %1").arg(e.what()));
    }

    return results;
}


QVariantMap SqliteDatabaseService::queryToVariantMap(const QSqlQuery &query) const
{
    QVariantMap map;
    if (!query.isValid() || !query.isActive()) {
        return map;
    }

    QSqlRecord record = query.record();
    for (int i = 0; i < record.count(); ++i) {
        QString fieldName = record.fieldName(i);
        QVariant value = query.value(i);

        // 特殊处理BLOB类型数据
        if (value.userType() == QMetaType::QByteArray) {
            map.insert(fieldName, value.toByteArray().toBase64());
        } else {
            map.insert(fieldName, value);
        }
    }
    return map;
}


QString SqliteDatabaseService::lastError() const {
    QMutexLocker locker(&m_mutex);
    return m_database.lastError().text();
}

void SqliteDatabaseService::ensureInitialized() {
    QMutexLocker locker(&m_mutex);
    if (!m_initialized) {
        initializeDatabase();
    }
}

QSqlDatabase& SqliteDatabaseService::getDatabase() {
    ensureInitialized();
    return m_database;
}

bool SqliteDatabaseService::initializeDatabase() {
    if (m_initialized && m_database.isOpen()) {
        return true;
    }
    
    // 关闭现有连接（如果有）
    if (m_database.isOpen()) {
        m_database.close();
    }
    
    // 创建新的数据库连接
    static int connectionCounter = 0;
    QString connectionName = QString("musicplayer_connection_%1").arg(++connectionCounter);
    m_database = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    m_database.setDatabaseName(m_databasePath);
    
    if (!m_database.open()) {
        Log.error(QString("Failed to open database: %1").arg(m_database.lastError().text()));
        return false;
    }
    
    // 启用外键支持
    QSqlQuery query(m_database);
    if (!query.exec("PRAGMA foreign_keys = ON;")) {
        Log.warning(QString("Failed to enable foreign keys: %1").arg(query.lastError().text()));
    }
    
    // 设置WAL模式提高并发性能
    if (!query.exec("PRAGMA journal_mode=WAL;")) {
        Log.warning(QString("Failed to set WAL mode: %1").arg(query.lastError().text()));
    }
    
    Log.debug("QSql connection established");
    m_initialized = true;
    return true;
}


