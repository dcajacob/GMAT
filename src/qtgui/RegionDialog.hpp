#pragma once
#include <QDialog>
#include <QMap>
class GmatBase;
class QComboBox;
class QLineEdit;
class QTableWidget;
class RegionDialog final : public QDialog
{
public:
   explicit RegionDialog(GmatBase &region,QWidget *parent=nullptr);
   QMap<QString,QString> settings() const;
private:
   QComboBox *body,*source;
   QLineEdit *file;
   QTableWidget *vertices;
};
