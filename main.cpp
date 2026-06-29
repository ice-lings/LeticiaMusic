#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStandardPaths>
#include <QTimer>
#include <QElapsedTimer>
#include <QList>
#include <QQmlError>


#include "src/core/application/appcontext.h"
#include "src/core/application/appconfig.h"
#include "src/core/services/service_locator.h"
#include "src/core/design/design_system.h"
#include "src/core/design/qml_tools.h"


#include "src/core/utils/logger.h"
#include "src/core/utils/platform_path_helper.h"


int main(int argc, char *argv[])
{
    AppContext::recordStartupTime();

    QElapsedTimer startupTimer;
    startupTimer.start();

    Logger::attachDebugConsole();

#if defined(Q_OS_WIN) && defined(QT_DEBUG)
    qputenv("QT_QUICK_CONTROLS_STYLE", "Fusion");
#endif

    QGuiApplication app(argc, argv);

    const QString logDir = PlatformPathHelper::logDir();
#ifdef QT_DEBUG
    Logger::get_instance().init(logDir.toStdString(), Logger::Level::Debug, true);
#else
    Logger::get_instance().init(logDir.toStdString(), Logger::Level::Info, false);
#endif

    Logger::installQtMessageHandler();
    Log.info("========================================= Application starting =========================================");
    Log.info("Qt version: " + QString(qVersion()));

    Log.info("Initializing AppContext...");
    AppConfig::loadOrInit();
    auto& appCtx = AppContext::instance();
    appCtx.initOrchestrator().runPreQml();
    Log.info("AppContext PreQml init complete");
    Log.info(QString("[Timing] PreQml phase done: %1ms").arg(startupTimer.elapsed()));

    try {
        appCtx.initOrchestrator().startAsyncInit(AppConfig::instance().databasePath());

        DesignSystem designSystem;
        QmlTools qmlTools;
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("Design", &designSystem);
        engine.rootContext()->setContextProperty("Tools", &qmlTools);
        engine.rootContext()->setContextProperty("appContext", &appCtx);
#ifdef Q_OS_WIN
        const QUrl url(QStringLiteral("qrc:/LeticiaMusic/src/qml/win/Main.qml"));
#elif defined(Q_OS_ANDROID)
        const QUrl url(QStringLiteral("qrc:/LeticiaMusic/src/qml/android/Main.qml"));
#endif

        QObject::connect(
            &engine,
            &QQmlApplicationEngine::objectCreationFailed,
            &app,
            [](const QUrl &url) {
                Log.error(QString("QML object creation failed - exiting, url=%1").arg(url.toString()));
                QCoreApplication::exit(-1);
            },
            Qt::QueuedConnection);

        QObject::connect(
            &engine,
            &QQmlEngine::warnings,
            &app,
            [](const QList<QQmlError> &warnings) {
                for (const auto &w : warnings) {
                    Log.error(QString("[QML] %1").arg(w.toString()));
                }
            },
            Qt::QueuedConnection);
        engine.load(url);

#ifdef Q_OS_ANDROID
        QNativeInterface::QAndroidApplication::hideSplashScreen(200);
#endif

        Log.info("QML engine loaded");
        Log.info(QString("[Timing] QML engine loaded: %1ms from main()").arg(startupTimer.elapsed()));

        return app.exec();

    } catch (const std::exception &e) {
        Log.error("Fatal error: " + QString(e.what()));
        return -1;
    }
}

