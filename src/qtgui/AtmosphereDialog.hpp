#pragma once
#include <QDialog>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <memory>
class GmatBase;
class QComboBox;
class QLabel;
QSet<QString> applyAtmosphereProperties(GmatBase &object,const QMap<QString,QString> &values);
class AtmosphereDialog final : public QDialog
{
public:
   AtmosphereDialog(GmatBase &forceModel,const QMap<QString,QString> &pending,QWidget *parent=nullptr);
   ~AtmosphereDialog() override;
   QMap<QString,QString> values() const { return acceptedValues; }
private:
   void update();
   std::unique_ptr<GmatBase> original;
   QMap<QString,QWidget *> controls;
   QMap<QString,QString> acceptedValues;
   QComboBox *body,*model;
   QLabel *error;
};
