#include "threadpoolmanager.h"

#include <QThreadPool>
#include <QMutex>

ThreadPoolManager::ThreadPoolManager(QObject *parent)
    : QObject{parent}
{}

QThreadPool *ThreadPoolManager::pool(PoolType type)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    switch(type) {
    case IO_POOL:
        return ioPool();
    case CPU_POOL:
        return cpuPool();
    case DB_POOL:
        return dbPool();
    default:
        return globalPool();
    }
}

void ThreadPoolManager::shutdownAll()
{
    ioPool()->waitForDone();
    cpuPool()->waitForDone();
    dbPool()->waitForDone();
}

QThreadPool *ThreadPoolManager::ioPool()
{
    static QThreadPool pool;
    pool.setMaxThreadCount(qMax(4, QThread::idealThreadCount() * 2));
    return &pool;
}

QThreadPool *ThreadPoolManager::cpuPool()
{
    static QThreadPool pool;
    pool.setMaxThreadCount(QThread::idealThreadCount());
    return &pool;
}

QThreadPool *ThreadPoolManager::dbPool()
{
    static QThreadPool pool;
    pool.setMaxThreadCount(1); // 数据库单线程访问
    return &pool;
}

QThreadPool *ThreadPoolManager::globalPool()
{
    return QThreadPool::globalInstance();
}


