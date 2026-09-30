#pragma once
#include <QString>
#include <QStringList>
QString userParameterLiteral(const QString &type,const QString &value);
QString setUserParameterValue(const QString &source,const QString &name,const QString &type,const QString &value,const QString &firstMissionStatement={});
// Replace only named configuration assignments, retaining comments and the
// entire mission suffix. Values in the ordered block are already serialized.
QString setConfigurationBlock(const QString &source,const QString &name,const QStringList &properties,const QString &block,const QString &firstMissionStatement={});
