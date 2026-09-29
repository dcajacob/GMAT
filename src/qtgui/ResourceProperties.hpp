#pragma once
#include <QString>
#include <QStringList>
#include <QVector>
class GmatBase;
struct ResourceProperty
{
   QString name;
   QString value;
   QString unit;
   QStringList choices;
};
QVector<ResourceProperty> resourceProperties(GmatBase &object);
void setResourceProperty(GmatBase &object, const QString &name, const QString &value);
