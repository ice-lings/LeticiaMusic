#ifndef SERVICE_LOCATOR_H
#define SERVICE_LOCATOR_H

#include <QObject>
#include <QHash>
#include <type_traits>
#include "../utils/singleton_holder.h"

class ServiceLocator : public QObject, public SingletonHolder<ServiceLocator>
{
    Q_OBJECT
    friend class SingletonHolder<ServiceLocator>;
    ServiceLocator(QObject* parent = nullptr) : QObject(parent) {}

public:
    void initialize();
    static ServiceLocator& instance() { return get_instance(); }

    template<typename T>
    T* get() {
        const char* key = T::staticMetaObject.className();
        if (auto it = m_services.find(key); it != m_services.end()) {
            return static_cast<T*>(it.value());
        }
        if (m_initialized && !m_services.contains("SyncManager")) {
            const_cast<ServiceLocator*>(this)->buildSyncComponents();
            return get<T>();
        }
        return nullptr;
    }

    template<typename T, typename... Args>
    T* registerService(Args&&... args) {
        static_assert(std::is_base_of_v<QObject, T>,
                      "ServiceLocator::registerService requires T to be a QObject subclass");
        auto* instance = new T(std::forward<Args>(args)...);
        instance->setParent(this);
        m_services[T::staticMetaObject.className()] = instance;
        return instance;
    }

    template<typename T>
    void registerExisting(T& instance) {
        static_assert(std::is_base_of_v<QObject, T>,
                      "ServiceLocator::registerExisting requires T to be a QObject subclass");
        m_services[T::staticMetaObject.className()] = &instance;
        instance.setParent(this);
    }

    void markInitialized() { m_initialized = true; }

private:
    void buildInfrastructure();
    void buildManagers();
    void buildViewModels();
    void buildServices();
    void buildSyncComponents();

    bool m_initialized = false;
    QHash<QString, QObject*> m_services;
};

#endif
