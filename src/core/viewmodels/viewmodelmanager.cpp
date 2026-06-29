#include "viewmodelmanager.h"

#include "../interfaces/IMusicEntityViewModel.h"

#include "../utils/logger.h"

ViewModelManager::ViewModelManager(QObject *parent)
    : QObject{parent}
{}

ViewModelManager::~ViewModelManager()
{
    // 先清空集合，避免悬空指针
    // 注意：Qt的父子机制会自动删除实际对象
    m_viewModels.clear();
    m_viewModelsByType.clear();
    m_factories.clear();
}

bool ViewModelManager::initialize()
{
    for (auto& viewModel : m_viewModels) {
        viewModel->initialize();
    }

    return true;
}

IMusicEntityViewModel* ViewModelManager::createViewModel(
    const QString& typeName,
    const QString& viewModelName
    )
{
    if (m_viewModels.contains(viewModelName)) {
        Log.warning("ViewModel already exists:" + viewModelName);
        return m_viewModels[viewModelName];
    }

    if (!m_factories.contains(typeName)) {
        Log.warning("Unknown view model type:" + typeName);
        return nullptr;
    }

    if (auto factory = m_factories.value(typeName)) {
        IMusicEntityViewModel* viewModel = factory(viewModelName);
        viewModel->setParent(this);
        m_viewModels[viewModelName] = viewModel;
        
        // 在类型索引中注册，支持按类型查询
        m_viewModelsByType.insert(typeName, viewModel);
        
        return viewModel;
    }
    return nullptr;
}

IMusicEntityViewModel *ViewModelManager::getViewModel(const QString &name)
{
    return m_viewModels.value(name, nullptr);
}


QList<IMusicEntityViewModel*> ViewModelManager::getViewModelsByType(const QString& typeName) const {
    return m_viewModelsByType.values(typeName);
}

void ViewModelManager::refreshAll() {
    for (auto& viewModel : m_viewModels) {
        viewModel->refreshModel();
    }
}

void ViewModelManager::refreshType(const QString& typeName) {
    auto viewModels = m_viewModelsByType.values(typeName);
    for (auto& viewModel : viewModels) {
        viewModel->refreshModel();
    }
}
