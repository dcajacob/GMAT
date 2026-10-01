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
#include <QVector>
#include <array>
#include <optional>
struct QtCameraPreset {
   QString name,reference,target;
   QString automaticTrajectory;
   QString automaticBody;
   QString segmentFrame;
   std::array<double,3> eye{0,-30000,0},center{},up{0,0,1};
   bool perspective=true;
   double fieldOfView=45;
   bool bodyRelative=false;
   bool lookAtRotation=false, shortestAngle=false;
};
struct QtCameraSetting {
   bool perspective=false; double fieldOfView=50;
   std::optional<std::array<double,3>> up;
   QString primaryName;
   QVector<QtCameraPreset> views;
   bool bodyRelative=false;
   bool lookAtRotation=false, shortestAngle=false;
   std::optional<std::array<double,3>> centerOffset;
   QString automaticTrajectory;
   QString automaticBody;
   QString segmentFrame;
   QMap<QString,bool> objectLabels,objectTrajectories,objectCenters,objectEndpoints;
   QMap<QString,quint32> objectMarkerSizes;
   QMap<QString,double> objectLineWidths;
   QMap<QString,quint32> objectFontSizes;
   QMap<QString,QString> objectFontPositions;
};
QMap<QString,quint32> qtObjectFontSizes(const QString &json);
QMap<QString,QString> qtObjectFontPositions(const QString &json);
QString qtObjectFontPositionsJson(const QMap<QString,QString> &positions);
QMap<QString,double> qtObjectWidths(const QString &json);
QString qtObjectWidthsJson(const QMap<QString,double> &widths);
QMap<QString,quint32> qtObjectSizes(const QString &json);
QString qtObjectSizesJson(const QMap<QString,quint32> &sizes);
QMap<QString,bool> qtObjectFlags(const QString &json);
QString qtObjectFlagsJson(const QMap<QString,bool> &flags);
QMap<QString,QtCameraSetting> qtCameraSettings(const QString &source);
QString qtCameraDirective(const QString &plot,const QtCameraSetting &setting);
QString retainQtCameraSettings(const QString &original,const QString &candidate,const QString &removedPlot={});
QString removeQtCameraSetting(const QString &source,const QString &plot);
QString setQtCameraSetting(const QString &source,const QString &plot,const QtCameraSetting &setting);
QString arrayExpressions(const QString &source,const QString &name);
QString setArrayExpressions(const QString &source,const QString &name,const QString &cells,int rows,int columns);
