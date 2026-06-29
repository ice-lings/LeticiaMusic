#include "init_orchestrator.h"
#include "../application/appconfig.h"
#include "../utils/logger.h"
#include <algorithm>
#include <QElapsedTimer>
#include <QDateTime>

InitOrchestrator::InitOrchestrator(QObject* parent)
    : QObject(parent)
{
}

void InitOrchestrator::runPreQml()
{
    QElapsedTimer timer;
    timer.start();

    std::vector<IInitStep*> steps;
    for (auto& s : m_steps) {
        if (s->phase() == InitPhase::PreQml)
            steps.push_back(s.get());
    }
    std::sort(steps.begin(), steps.end(), [](IInitStep* a, IInitStep* b) {
        return a->priority() < b->priority();
    });

    for (auto* step : steps) {
        Log.info(QString("Init [PreQml] %1").arg(step->name()));
        step->execute();
    }
    Log.info(QString("Init PreQml phase completed (%1ms)").arg(timer.elapsed()));
}

void InitOrchestrator::startAsyncInit(const QString& dbPath)
{
    if (m_steps.empty()) {
        runPostQml();
        return;
    }

    auto* thread = new QThread(this);

    std::vector<IInitStep*> asyncSteps;
    for (auto& s : m_steps) {
        if (s->phase() == InitPhase::Async)
            asyncSteps.push_back(s.get());
    }
    std::sort(asyncSteps.begin(), asyncSteps.end(), [](IInitStep* a, IInitStep* b) {
        return a->priority() < b->priority();
    });

    if (asyncSteps.empty()) {
        runPostQml();
        return;
    }

    auto* runner = new AsyncRunner(dbPath, asyncSteps);
    runner->moveToThread(thread);

    qint64 asyncStartMs = QDateTime::currentMSecsSinceEpoch();

    connect(thread, &QThread::started, runner, &AsyncRunner::run);
    connect(runner, &AsyncRunner::stepProgress, this, &InitOrchestrator::progressChanged);
    connect(runner, &AsyncRunner::finished, this, [this, thread, runner, asyncStartMs]() {
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - asyncStartMs;
        Log.info(QString("Async phase took %1ms").arg(elapsed));
        thread->quit();
        thread->wait();
        runner->deleteLater();
        thread->deleteLater();
        runPostQml();
    });

    thread->start();
}

void InitOrchestrator::runPostQml()
{
    QElapsedTimer timer;
    timer.start();

    std::vector<IInitStep*> steps;
    for (auto& s : m_steps) {
        if (s->phase() == InitPhase::PostQml)
            steps.push_back(s.get());
    }
    std::sort(steps.begin(), steps.end(), [](IInitStep* a, IInitStep* b) {
        return a->priority() < b->priority();
    });

    for (auto* step : steps) {
        Log.info(QString("Init [PostQml] %1").arg(step->name()));
        step->execute();
    }

    m_ready = true;
    Log.info(QString("Init all phases completed, emitting initialized (%1ms)").arg(timer.elapsed()));
    emit initialized();
}

void AsyncRunner::run()
{
    {
        QSqlDatabase workerDb = QSqlDatabase::addDatabase("QSQLITE", "init_worker");
        workerDb.setDatabaseName(m_dbPath);
        if (!workerDb.open()) {
            Log.error(QString("[AsyncRunner] Failed to open worker DB: %1").arg(m_dbPath));
            emit finished();
            return;
        }

        for (size_t i = 0; i < m_asyncSteps.size(); ++i) {
            emit stepProgress(static_cast<int>(i), static_cast<int>(m_asyncSteps.size()),
                              m_asyncSteps[i]->name());
            m_asyncSteps[i]->execute();
        }
    }
    QSqlDatabase::removeDatabase("init_worker");
    Log.info("[AsyncRunner] Background init complete");
    emit finished();
}
