#pragma once
#include <QDialog>
#include <QStringList>
class QListWidget;
// Keeps a local ordered selection. Engine validation happens on command Apply.
class ReportParameterDialog final : public QDialog
{
public:
   ReportParameterDialog(const QStringList &selected,QWidget *parent=nullptr);
   QStringList selection() const;
private:
   QListWidget *list;
};
