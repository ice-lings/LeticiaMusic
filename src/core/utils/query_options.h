#ifndef QUERYOPTIONS_H
#define QUERYOPTIONS_H

#include <QString>
#include <QStringList>
#include <QVariantMap>

struct QueryOptions {

    QStringList columns;
    QString whereClause;
    QVariantMap whereParams;
    QString orderBy;     // 排序
    int limit = -1;      // 分页
    int offset = -1;
    QString groupBy;    // 分组
    QString joinClause;  // 连接条件（可选）

    QueryOptions() = default;

    QueryOptions& setColumns(const QStringList &cols) {
        columns = cols;
        return *this;
    }

    QueryOptions& setWhere(const QString &where, const QVariantMap &params = QVariantMap()) {
        whereClause = where;
        whereParams = params;
        return *this;
    }

    QueryOptions& setOrderBy(const QString &order) {
        orderBy = order;
        return *this;
    }

    QueryOptions& setLimit(int lim, int off = -1) {
        limit = lim;
        offset = off;
        return *this;
    }

    QueryOptions& setGroupBy(const QString &group) {
        groupBy = group;
        return *this;
    }

    QString buildSelectQuery(const QString& tableName) const
    {
        QString query = "SELECT ";

        // 处理列选择
        if (columns.isEmpty()) {
            query += "*";
        } else {
            query += columns.join(", ");
        }

        query += " FROM " + tableName;

        // 处理JOIN条件
        if (!joinClause.isEmpty()) {
            query += " " + joinClause;
        }

        // 处理WHERE条件
        if (!whereClause.isEmpty()) {
            query += " WHERE " + whereClause;
        }

        // 处理GROUP BY
        if (!groupBy.isEmpty()) {
            query += " GROUP BY " + groupBy;
        }

        // 处理ORDER BY
        if (!orderBy.isEmpty()) {
            query += " ORDER BY " + orderBy;
        }

        // 处理LIMIT/OFFSET
        if (limit > 0) {
            query += " LIMIT " + QString::number(limit);
            if (offset >= 0) {  // 修正：offset可以为0
                query += " OFFSET " + QString::number(offset);
            }
        }

        return query;
    }
};

#endif // QUERYOPTIONS_H
