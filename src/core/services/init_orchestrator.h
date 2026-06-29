#ifndef INIT_ORCHESTRATOR_H
#define INIT_ORCHESTRATOR_H

#include <QObject>
#include <QThread>
#include <QSqlDatabase>
#include <vector>
#include <memory>

#include "init_step.h"

class InitOrchestrator : public QObject
{
    Q_OBJECT
public:
    explicit InitOrchestrator(QObject* parent = nullptr);

    template<typename T, typename... Args>
    void addStep(Args&&... args)
    {
        m_steps.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    void runPreQml();
    void startAsyncInit(const QString& dbPath);
    bool isReady() const { return m_ready; }

signals:
    void progressChanged(int step, int total, const QString& name);
    void initialized();

private:
    void runPostQml();
    std::vector<std::unique_ptr<IInitStep>> m_steps;
    bool m_ready = false;
};

class AsyncRunner : public QObject
{
    Q_OBJECT
public:
    AsyncRunner(const QString& dbPath, const std::vector<IInitStep*>& steps)
        : m_dbPath(dbPath), m_asyncSteps(steps) {}

public slots:
    void run();

signals:
    void stepProgress(int step, int total, const QString& name);
    void finished();

private:
    QString m_dbPath;
    std::vector<IInitStep*> m_asyncSteps;
};

#endif
