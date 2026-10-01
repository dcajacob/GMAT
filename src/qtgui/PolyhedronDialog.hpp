#pragma once
#include <QDialog>
#include <QJsonArray>
#include <QString>
class GmatBase;
class QTableWidget;
class QLabel;
QJsonArray polyhedronSettings(GmatBase &model);
QString validatePolyhedronSettings(const QJsonArray &settings);
QString polyhedronScript(const QString &source,GmatBase &model,const QJsonArray &settings,const QString &firstCommand);
QString polyhedronSettingsError(GmatBase &model,const QJsonArray &settings);
class PolyhedronDialog final : public QDialog
{
public:
   PolyhedronDialog(const QJsonArray &settings,QWidget *parent=nullptr);
   QJsonArray values() const { return accepted; }
private:
   void addRow(const QString &body={},const QString &shape={},const QString &density="1000");
   QTableWidget *table;
   QLabel *error=nullptr;
   QJsonArray accepted;
};
