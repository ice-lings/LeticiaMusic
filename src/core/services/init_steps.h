#ifndef INIT_STEPS_H
#define INIT_STEPS_H

#include "init_step.h"

class DatabaseInitStep : public IInitStep
{
public:
    QString name() const override { return "数据库"; }
    InitPhase phase() const override { return InitPhase::PreQml; }
    int priority() const override { return 0; }
    void execute() override;
};

class PlayerInitStep : public IInitStep
{
public:
    QString name() const override { return "播放器"; }
    InitPhase phase() const override { return InitPhase::PreQml; }
    int priority() const override { return 1; }
    void execute() override;
};

class SchemaInitStep : public IInitStep
{
public:
    QString name() const override { return "数据库建表"; }
    InitPhase phase() const override { return InitPhase::PreQml; }
    int priority() const override { return 2; }
    void execute() override;
};

class PlaylistInitStep : public IInitStep
{
public:
    QString name() const override { return "歌单管理器"; }
    InitPhase phase() const override { return InitPhase::PreQml; }
    int priority() const override { return 3; }
    void execute() override;
};

class ViewModelRegisterStep : public IInitStep
{
public:
    QString name() const override { return "ViewModel"; }
    InitPhase phase() const override { return InitPhase::PreQml; }
    int priority() const override { return 4; }
    void execute() override;
};

class DataLoadInitStep : public IInitStep
{
public:
    QString name() const override { return "加载歌曲数据"; }
    InitPhase phase() const override { return InitPhase::Async; }
    int priority() const override { return 0; }
    void execute() override;
};

class ViewModelPopulateStep : public IInitStep
{
public:
    QString name() const override { return "填充歌曲列表"; }
    InitPhase phase() const override { return InitPhase::PostQml; }
    int priority() const override { return 0; }
    void execute() override;
};

class ServiceInitStep : public IInitStep
{
public:
    QString name() const override { return "服务初始化"; }
    InitPhase phase() const override { return InitPhase::PostQml; }
    int priority() const override { return 1; }
    void execute() override;
};

class SignalConnectStep : public IInitStep
{
public:
    QString name() const override { return "信号连接"; }
    InitPhase phase() const override { return InitPhase::PostQml; }
    int priority() const override { return 2; }
    void execute() override;
};

class RestorePlaybackStep : public IInitStep
{
public:
    QString name() const override { return "恢复播放"; }
    InitPhase phase() const override { return InitPhase::PostQml; }
    int priority() const override { return 3; }
    void execute() override;
};

#endif
