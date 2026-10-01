#pragma once
#include <QDialog>
#include <QMap>
#include <QStringList>
class QTableWidget;
class OrbitObjectDrawingDialog final : public QDialog
{
public:
   OrbitObjectDrawingDialog(const QStringList &names,const QMap<QString,bool> &labels,const QMap<QString,bool> &trajectories,QWidget *parent=nullptr);
   QMap<QString,QString> settings() const;
private:
   QTableWidget *objects;
};
