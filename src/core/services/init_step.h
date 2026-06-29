#ifndef INIT_STEP_H
#define INIT_STEP_H

#include <QString>

enum class InitPhase {
    PreQml,
    Async,
    PostQml
};

class IInitStep
{
public:
    virtual ~IInitStep() = default;
    virtual QString name() const = 0;
    virtual InitPhase phase() const = 0;
    virtual int priority() const = 0;
    virtual void execute() = 0;
};

#endif
