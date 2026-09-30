#pragma once
#include <QDialog>
#include <QMap>
#include <QStringList>
class QListWidget;
class QTableWidget;
class QLabel;
class QDialogButtonBox;

class BulkPropertyDialog final : public QDialog
{
public:
   BulkPropertyDialog(const QMap<QString,QStringList> &properties,const QString &owner,const QString &property,QWidget *parent=nullptr);
   QStringList selection() const;
private:
   void refresh();
   void validate();
   QMap<QString,QStringList> properties;
   QListWidget *owners;
   QTableWidget *table;
   QLabel *status;
   QDialogButtonBox *buttons;
};
