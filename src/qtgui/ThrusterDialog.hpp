#pragma once
#include <QDialog>
#include <QMap>
#include <QSet>
#include <QString>
class GmatBase;
class QComboBox;
class QLineEdit;
class QLabel;
void validateThrusterProperties(GmatBase &thruster);
class ThrusterDialog final : public QDialog
{
public:
   ThrusterDialog(GmatBase &thruster,const QMap<QString,QString> &pending,QWidget *parent=nullptr);
   QMap<QString,QString> settings() const;
private:
   QMap<QString,QComboBox *> choices;
   QMap<QString,QLineEdit *> numbers;
   QLabel *error;
};
