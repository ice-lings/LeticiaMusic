#ifndef VIEWMODELMANAGER_H
#define VIEWMODELMANAGER_H

#include <QObject>
#include <QMap>
#include <functional>
#include <memory>  // for std::unique_ptr, std::make_unique

#include "../interfaces/IMusicEntityViewModel.h"
#include "../playlist/playlistmanager.h"

class ViewModelManager : public QObject
{
    Q_OBJECT
public:
    explicit ViewModelManager(QObject *parent = nullptr);
    ~ViewModelManager();
    bool initialize();

    template<typename T>
    void registerViewModelType(const QString& typeName) {
        m_factories[typeName] = [this](const QString& name) -> IMusicEntityViewModel* {
            // 使用unique_ptr保护，防止异常导致内存泄漏
            auto viewModel = std::make_unique<T>();
            viewModel->setViewModelName(name);
            
            // 获取原始指针用于返回
            T* rawPtr = viewModel.get();
            
            // 将所有权转移给Qt父子机制
            viewModel->setParent(this);
            
            // 释放unique_ptr所有权（现在由Qt管理生命周期）
            viewModel.release();
            
            return rawPtr;
        };
    }

    template<typename T>
    void registerPlaylistViewModelType(const QString& typeName, PlaylistManager* mgr) {
        m_factories[typeName] = [this, mgr](const QString& name) -> IMusicEntityViewModel* {
            auto viewModel = std::make_unique<T>(mgr);
            viewModel->setViewModelName(name);
            T* rawPtr = viewModel.get();
            viewModel->setParent(this);
            viewModel.release();
            return rawPtr;
        };
    }



    /**
     * @brief 创建并管理视图模型实例（遵循单例模式或缓存机制）
     *
     * @param typeName 视图模型类型名称（如"AllMusic"、"Playlist"）
     * @param viewModelName 视图模型实例名称（用于唯一标识）
     * @return 成功创建的视图模型指针，若失败则返回nullptr
     */
    IMusicEntityViewModel* createViewModel(const QString& typeName, const QString& viewModelName);

    /**
     * @brief 获得已经创建的视图模型，若没有则返回空指针
     *
     * @param name 视图模型实例唯一标识名称
     * @return 视图模型指针，若失败则返回nullptr
     */
    IMusicEntityViewModel* getViewModel(const QString& name);


    template<typename T>
    T* getViewModel(const QString& name) {
        // 先通过基类版获取实例，再做类型安全的动态转换
        IMusicEntityViewModel* baseVm = getViewModel(name);
        if (!baseVm) {
            qWarning() << "ViewModelManager: 未找到名为" << name << "的视图模型实例";
            return nullptr;
        }
        // 动态转换，类型不匹配则返回nullptr，避免强制转换的野指针问题
        T* targetVm = dynamic_cast<T*>(baseVm);
        if (!targetVm) {
            qWarning() << "ViewModelManager: 名为" << name << "的实例类型不匹配，预期类型：" << typeid(T).name();
        }
        return targetVm;
    }

    /**
     * @brief 获得一类型的所有视图模型，若没有则返回空列表
     *
     * @param typeName 视图模型类型名称
     * @return 视图模型列表，若没有则返回空列表
     */
    QList<IMusicEntityViewModel*> getViewModelsByType(const QString& typeName) const;

    void refreshAll();
    void refreshType(const QString& typeName);
signals:

private:
    QMap<QString, std::function<IMusicEntityViewModel*(const QString&)>> m_factories;
    QMap<QString, IMusicEntityViewModel*> m_viewModels; // name -> instance
    QMultiMap<QString, IMusicEntityViewModel*> m_viewModelsByType; // type -> instances
};

#endif // VIEWMODELMANAGER_H
