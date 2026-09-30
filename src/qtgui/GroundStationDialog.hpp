#pragma once
#include <QDialog>
#include <QMap>
#include <QSet>
#include <QString>
#include <memory>
class GmatBase;
class QComboBox;
class QLabel;
class QLineEdit;
QSet<QString> applyGroundStationLocation(GmatBase &object,const QMap<QString,QString> &values);
class GroundStationDialog final : public QDialog
{
public:
   GroundStationDialog(GmatBase &station,const QMap<QString,QString> &pending,QWidget *parent=nullptr);
   ~GroundStationDialog() override;
   QMap<QString,QString> values() const { return acceptedValues; }
private:
   QMap<QString,QString> settings() const;
   void convert();
   void refresh(GmatBase &station,bool updateValues=true);
   std::unique_ptr<GmatBase> original;
   QComboBox *body,*type,*horizon;
   QMap<QString,QLineEdit *> fields;
   QLabel *labels[3],*units[3],*error;
   QString previousBody,previousType,previousHorizon;
   QMap<QString,QString> acceptedValues;
};
