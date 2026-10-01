#pragma once
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QSet>
class GmatBase;
struct ResourceProperty
{
   QString name;
   QString value;
   QString unit;
   QString help;
   QStringList choices;
   QStringList references;
   QStringList hiddenReferences;
   bool color = false;
   bool filename = false;
   bool fileList = false;
   bool fileInput = false;
   bool fileOutput = false;
   bool list = false;
   bool resizableVector = false;
   int rows = 0, columns = 0;
};
QVector<ResourceProperty> resourceProperties(GmatBase &object);
QStringList pythonModuleNames();
QStringList dataInterfaceFields(GmatBase &object);
void validateResourceProperties(GmatBase &object);
void setResourceProperty(GmatBase &object, const QString &name, const QString &value);
QSet<QString> applyAttitudeProperties(GmatBase &spacecraft, const QMap<QString,QString> &values);
QStringList splitResourceReferences(const QString &value);
bool isResourceFileList(GmatBase &object, const QString &name);
bool isResourceList(GmatBase &object, const QString &name);
bool isResizableResourceVector(GmatBase &object,const QString &name);
QString replaceResourceList(GmatBase &object, const QString &block, const QString &name, const QString &value, const QString *mixture = nullptr);
