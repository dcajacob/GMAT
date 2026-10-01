#pragma once
#include <QDialog>
#include <QString>
class QCheckBox;
class QLineEdit;
class PropagationColorDialog final : public QDialog
{
public:
   explicit PropagationColorDialog(const QString &source,QWidget *parent=nullptr);
   static bool supports(const QString &source);
   QString statement() const;
private:
   QString source;
   QCheckBox *overrideColor;
   QLineEdit *color;
};
