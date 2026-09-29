#pragma once
#include <QDialog>
class QTableWidget;
class QDialogButtonBox;
class QLabel;
class ConditionDialog final : public QDialog
{
public:
   ConditionDialog(const QString &condition,QWidget *parent);
   static bool supports(const QString &condition);
   QString condition() const;
private:
   void addRow(const QString &join,const QString &left,const QString &relation,const QString &right);
   void validate();
   QTableWidget *table;
   QDialogButtonBox *buttons;
   QLabel *status;
};
