#pragma once
#include <QString>
#include <memory>
class GmatBase;
// An unregistered template: opening/canceling the creator cannot mutate a mission.
std::unique_ptr<GmatBase> resourceDraft(const QString &type,const QString &name);
