#pragma once
#include <QDialog>
#include <QMap>
#include <QString>
class GmatBase;
class QGroupBox;
class QComboBox;
class QLineEdit;
class QCheckBox;
class QLabel;
QMap<QString,QString> externalForceSettings(GmatBase &model);
QString validateExternalForceSettings(const QMap<QString,QString> &settings);
QString externalForceScript(const QString &source,const QString &model,const QMap<QString,QString> &settings,const QString &firstMissionStatement);
QString externalForceSettingsError(GmatBase &model,const QMap<QString,QString> &settings);
class ExternalForceDialog final : public QDialog
{
public:
   ExternalForceDialog(GmatBase &model,const QMap<QString,QString> &pending,QWidget *parent=nullptr);
   QMap<QString,QString> values() const { return accepted; }
private:
   QGroupBox *enabled;
   QComboBox *module;
   QLineEdit *function;
   QCheckBox *exclusive;
   QLabel *error;
   QMap<QString,QString> accepted;
};
