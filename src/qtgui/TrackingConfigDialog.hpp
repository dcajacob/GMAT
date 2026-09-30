#pragma once
#include <QDialog>
#include <QStringList>
class GmatBase;
class QTableWidget;
class QLabel;
QStringList trackingMeasurementTypes();
QString trackingConfigurations(GmatBase &object);
QString replaceTrackingConfigurations(GmatBase &object,const QString &block,const QString &settings);
class TrackingConfigDialog final : public QDialog
{
public:
   TrackingConfigDialog(GmatBase &object,const QString &pending,QWidget *parent=nullptr);
   QString settings() const;
private:
   void addRow(const QString &path={},const QString &types={});
   void choosePath();
   void chooseTypes();
   QTableWidget *table;
   QLabel *error;
};
