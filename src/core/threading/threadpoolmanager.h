#ifndef THREADPOOLMANAGER_H
#define THREADPOOLMANAGER_H

#include <QObject>
#include <QThreadPool>

class ThreadPoolManager : public QObject
{
    Q_OBJECT
public:
    explicit ThreadPoolManager(QObject *parent = nullptr);

    enum PoolType {
        IO_POOL,
        CPU_POOL,
        DB_POOL
    };
    Q_ENUM(PoolType)

    static QThreadPool* pool(PoolType type);

    static void shutdownAll();

private:

    static QThreadPool* ioPool();

    static QThreadPool* cpuPool();

    static QThreadPool* dbPool();

    static QThreadPool* globalPool();

};

#endif // THREADPOOLMANAGER_H
