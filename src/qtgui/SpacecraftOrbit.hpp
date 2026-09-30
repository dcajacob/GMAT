#pragma once
#include <QMap>
#include <QSet>
#include <QStringList>
class GmatBase;
class CoordinateSystem;
CoordinateSystem *spacecraftOrbitFrame(const QString &name);
QStringList spacecraftOrbitRepresentations(CoordinateSystem &frame);
QStringList spacecraftOrbitElementNames(GmatBase &object);
QStringList spacecraftOrbitAnomalies(GmatBase &object);
QSet<QString> applySpacecraftOrbitProperties(GmatBase &object,const QMap<QString,QString> &values);
QString spacecraftOrbitScript(GmatBase &object);
QString preserveSpacecraftOrbits(QString script,GmatBase *exclude=nullptr);
