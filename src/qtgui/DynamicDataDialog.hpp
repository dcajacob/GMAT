#pragma once
#include <QDialog>
#include <QMap>
#include <QString>
#include <vector>
#include "DynamicDataStruct.hpp"
class GmatBase;
class QTableWidget;
class QSpinBox;
class QLabel;
QString dynamicDataSettings(GmatBase &object);
void applyDynamicDataSettings(GmatBase &object,const QString &settings);
class DynamicDataDialog final : public QDialog
{
public:
   DynamicDataDialog(GmatBase &object,const QMap<QString,QString> &pending,QWidget *parent=nullptr);
   QMap<QString,QString> settings() const;
private:
   void resizeGrid();
   void refresh();
   void editCell(int row,int column);
   std::vector<std::vector<DDD>> cells;
   QTableWidget *grid;
   QSpinBox *rows,*columns;
   QLabel *error;
};
