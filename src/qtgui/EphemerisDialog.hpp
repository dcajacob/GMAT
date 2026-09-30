#pragma once
#include <QDialog>
#include <QMap>
#include <QSet>
class GmatBase;
class QComboBox;
class QLineEdit;
class QCheckBox;
class QLabel;
QSet<QString> applyEphemerisProperties(GmatBase &object,const QMap<QString,QString> &values);
class EphemerisDialog final : public QDialog
{
public:
   EphemerisDialog(GmatBase &object,const QMap<QString,QString> &pending,QWidget *parent=nullptr);
   QMap<QString,QString> settings() const;
private:
   void changeFileFormat();
   void convertEpochs();
   void updateFrames(const QString &frame);
   QMap<QString,QComboBox *> choices;
   QMap<QString,QLineEdit *> text;
   QMap<QString,QCheckBox *> checks;
   QMap<QString,QMap<QString,QString>> variants;
   QLabel *error;
   QString previousFormat,previousEpochFormat;
};
