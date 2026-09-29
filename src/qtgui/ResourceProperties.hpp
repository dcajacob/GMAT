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
   QStringList references;
   bool filename = false;
   bool list = false;
   int rows = 0, columns = 0;
};
QVector<ResourceProperty> resourceProperties(GmatBase &object);
void validateResourceProperties(GmatBase &object);
void setResourceProperty(GmatBase &object, const QString &name, const QString &value);
bool isResourceList(GmatBase &object, const QString &name);
QString replaceResourceList(GmatBase &object, const QString &block, const QString &name, const QString &value);
