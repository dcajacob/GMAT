#ifndef GMAT_QT_BURN_DIALOG_HPP
#define GMAT_QT_BURN_DIALOG_HPP
#include <QDialog>
#include <QMap>
#include <QSet>
class GmatBase;
class QComboBox;
class QLineEdit;
class QCheckBox;
class QLabel;

QSet<QString> applyBurnProperties(GmatBase &object,const QMap<QString,QString> &values);
void validateBurnProperties(GmatBase &object);
QString burnResourceScript(GmatBase &object);

class BurnDialog final : public QDialog
{
public:
   BurnDialog(GmatBase &burn,const QMap<QString,QString> &pending,QWidget *parent=nullptr);
   QMap<QString,QString> settings() const;
private:
   QMap<QString,QComboBox *> choices;
   QMap<QString,QLineEdit *> numbers;
   QCheckBox *decrement;
   QLabel *error;
};
#endif
