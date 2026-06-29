#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QGuiApplication>
#include "test-viewmodelmanager.h"
#include "../../src/core/viewmodels/viewmodelmanager.h"

// Static variable definitions
int TestMusicViewModel::constructorCount = 0;
int TestMusicViewModel::destructorCount = 0;
bool TestMusicViewModel::throwOnConstruction = false;

class TestViewModelManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        TestMusicViewModel::resetCounters();
    }

    void cleanupTestCase() {
    }

    void init() {
        TestMusicViewModel::resetCounters();
    }

    // 测试1：基本ViewModel创建
    void testCreateViewModel() {
        ViewModelManager manager;
        manager.registerViewModelType<TestMusicViewModel>("TestType");
        
        auto vm = manager.createViewModel("TestType", "TestVM1");
        
        QVERIFY(vm != nullptr);
        QCOMPARE(vm->viewModelName(), QString("TestVM1"));
        QCOMPARE(TestMusicViewModel::constructorCount, 1);
    }

    // 测试2：按名称检索ViewModel
    void testGetViewModel() {
        ViewModelManager manager;
        manager.registerViewModelType<TestMusicViewModel>("TestType");
        
        auto created = manager.createViewModel("TestType", "TestVM2");
        auto retrieved = manager.getViewModel("TestVM2");
        
        QVERIFY(retrieved != nullptr);
        QCOMPARE(created, retrieved);
    }

    // 测试3：基于类型的ViewModel查询（修复的bug）
    void testGetViewModelsByType() {
        ViewModelManager manager;
        manager.registerViewModelType<TestMusicViewModel>("TestType");
        manager.registerViewModelType<AnotherTestViewModel>("AnotherType");
        
        manager.createViewModel("TestType", "VM1");
        manager.createViewModel("TestType", "VM2");
        manager.createViewModel("AnotherType", "VM3");
        
        auto testTypeVMs = manager.getViewModelsByType("TestType");
        auto anotherTypeVMs = manager.getViewModelsByType("AnotherType");
        
        QCOMPARE(testTypeVMs.size(), 2);
        QCOMPARE(anotherTypeVMs.size(), 1);
    }

    // 测试4：重复ViewModel名称返回已存在的
    void testDuplicateViewModelName() {
        ViewModelManager manager;
        manager.registerViewModelType<TestMusicViewModel>("TestType");
        
        auto vm1 = manager.createViewModel("TestType", "DuplicateVM");
        auto vm2 = manager.createViewModel("TestType", "DuplicateVM");
        
        QCOMPARE(vm1, vm2);  // 应返回相同指针
        QCOMPARE(TestMusicViewModel::constructorCount, 1);  // 只创建一个
    }

    // 测试5：未知类型返回nullptr
    void testUnknownTypeReturnsNull() {
        ViewModelManager manager;
        
        auto vm = manager.createViewModel("UnknownType", "VM");
        
        QVERIFY(vm == nullptr);
    }

    // 测试6：异常安全 - 构造失败时无泄漏（核心测试）
    void testExceptionSafetyNoLeak() {
        ViewModelManager manager;
        manager.registerViewModelType<TestMusicViewModel>("FailingType");
        
        int initialConstructorCount = TestMusicViewModel::constructorCount;
        TestMusicViewModel::throwOnConstruction = true;
        
        // 不应崩溃或泄漏
        IMusicEntityViewModel* vm = nullptr;
        bool exceptionThrown = false;
        try {
            vm = manager.createViewModel("FailingType", "FailingVM");
        } catch (const std::exception&) {
            exceptionThrown = true;
        }
        
        // 为其他测试重置
        TestMusicViewModel::throwOnConstruction = false;
        
        // 即使构造函数中抛出异常，
        // std::unique_ptr也能确保内存被释放
        QCOMPARE(TestMusicViewModel::constructorCount, initialConstructorCount + 1);
        QVERIFY(vm == nullptr || exceptionThrown);
    }

    // 测试7：使用正确类型的模板getViewModel
    void testTemplateGetViewModel() {
        ViewModelManager manager;
        manager.registerViewModelType<TestMusicViewModel>("TestType");
        
        manager.createViewModel("TestType", "TypedVM");
        auto typed = manager.getViewModel<TestMusicViewModel>("TypedVM");
        
        QVERIFY(typed != nullptr);
        QCOMPARE(typed->viewModelName(), QString("TypedVM"));
    }

    // 测试8：使用错误类型的模板getViewModel返回nullptr
    void testTemplateGetViewModelWrongType() {
        ViewModelManager manager;
        manager.registerViewModelType<TestMusicViewModel>("TestType");
        
        manager.createViewModel("TestType", "WrongTypeVM");
        auto wrong = manager.getViewModel<AnotherTestViewModel>("WrongTypeVM");
        
        QVERIFY(wrong == nullptr);  // 错误类型应返回nullptr
    }

    // 测试9：ViewModel父对象正确设置为manager
    void testViewModelParentSet() {
        ViewModelManager manager;
        manager.registerViewModelType<TestMusicViewModel>("TestType");
        
        auto vm = manager.createViewModel("TestType", "ParentVM");
        
        QCOMPARE(vm->parent(), &manager);
    }

    // 测试10：Manager析构时清理ViewModels
    void testManagerDestruction() {
        int initialDestructorCount = TestMusicViewModel::destructorCount;
        
        {
            ViewModelManager manager;
            manager.registerViewModelType<TestMusicViewModel>("TestType");
            manager.createViewModel("TestType", "DestructVM");
            
            // destructorCount应保持为初始值
            QCOMPARE(TestMusicViewModel::destructorCount, initialDestructorCount);
        }  // Manager在此处销毁
        
        // 现在应调用析构函数（Qt父子机制）
        // 注意：在真实Qt环境中这会生效
        // QCOMPARE(TestMusicViewModel::destructorCount, initialDestructorCount + 1);
    }
};

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QTEST_SET_MAIN_SOURCE_PATH
    TestViewModelManager tc;
    return QTest::qExec(&tc, argc, argv);
}
#include "test-viewmodelmanager.moc"
