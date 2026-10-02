#pragma once
#include <QDialog>
#include <QMap>
#include <QString>
#include <memory>
class GmatBase;
class QComboBox;
class QLabel;
class QVBoxLayout;
class AttitudeDialog final : public QDialog
{
public:
   AttitudeDialog(GmatBase &spacecraft, const QMap<QString,QString> &pending, QWidget *parent=nullptr);
   ~AttitudeDialog() override;
   QMap<QString,QString> values() const { return acceptedValues; }
   QMap<QString,QString> settings();
   QMap<QString,QString> pendingValues() const;
private:
   void rebuild();
   bool submit(const QString &selector={}, const QString &selected={});
   std::unique_ptr<GmatBase> candidate;
   QMap<QString,std::shared_ptr<GmatBase>> models;
   QMap<QString,QWidget *> controls;
   QMap<QString,QString> loaded,acceptedValues;
   QComboBox *model;
   QLabel *error;
   QVBoxLayout *layout;
   QWidget *fields=nullptr;
};
