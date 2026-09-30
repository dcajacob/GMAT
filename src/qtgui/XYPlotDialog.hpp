#pragma once
#include <QDialog>
#include <QMap>
#include <QString>
class GmatBase;
class QLineEdit;
class QListWidget;
class QCheckBox;
class QComboBox;
class QLabel;
class XYPlotDialog final : public QDialog
{
public:
   XYPlotDialog(GmatBase &plot,const QMap<QString,QString> &pending,QWidget *parent=nullptr);
   QMap<QString,QString> settings() const;
private:
   QLineEdit *x;
   QListWidget *y;
   QCheckBox *show,*grid;
   QComboBox *solver;
   QLabel *error;
};
