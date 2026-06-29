#ifndef IMUSICENTITYVIEWMODEL_H
#define IMUSICENTITYVIEWMODEL_H

#include <QAbstractListModel>

class IMusicEntityViewModel : public QAbstractListModel
{
    Q_OBJECT;
public:
    explicit IMusicEntityViewModel(QObject* parent = nullptr): QAbstractListModel(parent){}
    virtual ~IMusicEntityViewModel() = default;

    QString viewModelName() const { return m_viewModelName; }
    void setViewModelName(const QString& name) { m_viewModelName = name; }

    // 初始化视图模型数据
    virtual void initialize() = 0;

    // 清空模型数据
    virtual void clearModel() = 0;

    // 刷新模型数据
    virtual void refreshModel() = 0;

    virtual QObject* modelObject() { return this; }

signals:
    void modelInitialized();
    void modelUpdated();

protected:
    QString m_viewModelName;
};




#endif // IMUSICENTITYVIEWMODEL_H
