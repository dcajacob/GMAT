#pragma once
#include <QString>
QString userParameterLiteral(const QString &type,const QString &value);
QString setUserParameterValue(const QString &source,const QString &name,const QString &type,const QString &value,const QString &firstMissionStatement={});
