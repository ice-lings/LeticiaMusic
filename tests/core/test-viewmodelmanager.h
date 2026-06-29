#ifndef TEST_VIEWMODELMANAGER_H
#define TEST_VIEWMODELMANAGER_H

#include <QObject>
#include <QString>
#include "../../src/core/interfaces/IMusicEntityViewModel.h"

// 测试ViewModel，追踪构造函数/析构函数调用
class TestMusicViewModel : public IMusicEntityViewModel {
    Q_OBJECT
public:
    static int constructorCount;
    static int destructorCount;
    static bool throwOnConstruction;
    
    explicit TestMusicViewModel(QObject* parent = nullptr) 
        : IMusicEntityViewModel(parent) {
        constructorCount++;
        if (throwOnConstruction) {
            throw std::runtime_error("模拟构造失败");
        }
    }
    
    ~TestMusicViewModel() {
        destructorCount++;
    }
    
    // QAbstractListModel接口
    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        return 0;
    }
    
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        return QVariant();
    }
    
    // IMusicEntityViewModel接口
    void initialize() override {}
    void clearModel() override {}
    void refreshModel() override {}
    
    static void resetCounters() {
        constructorCount = 0;
        destructorCount = 0;
        throwOnConstruction = false;
    }
};

// 另一个测试类型，用于基于类型的查询
class AnotherTestViewModel : public IMusicEntityViewModel {
    Q_OBJECT
public:
    explicit AnotherTestViewModel(QObject* parent = nullptr) 
        : IMusicEntityViewModel(parent) {}
    
    int rowCount(const QModelIndex &parent = QModelIndex()) const override { return 0; }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override { return QVariant(); }
    void initialize() override {}
    void clearModel() override {}
    void refreshModel() override {}
};

#endif // TEST_VIEWMODELMANAGER_H
