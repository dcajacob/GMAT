#pragma once
#include <QString>
#include <QStringList>
struct QtScriptConversion {
   QString script,error;
   QStringList notes;
   int plots=0;
};
QtScriptConversion convertOpenFramesViews(const QString &source);

#include <QMap>
struct QtCameraSetting { bool perspective=false; double fieldOfView=50; };
QMap<QString,QtCameraSetting> qtCameraSettings(const QString &source);
QString qtCameraDirective(const QString &plot,const QtCameraSetting &setting);
QString retainQtCameraSettings(const QString &original,const QString &candidate);
QString setQtCameraSetting(const QString &source,const QString &plot,const QtCameraSetting &setting);
QString arrayExpressions(const QString &source,const QString &name);
QString setArrayExpressions(const QString &source,const QString &name,const QString &cells,int rows,int columns);
