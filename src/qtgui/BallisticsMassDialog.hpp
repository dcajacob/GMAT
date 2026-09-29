#pragma once
#include <QDialog>
#include <QMap>
#include <QStringList>

class BallisticsMassDialog final : public QDialog
{
public:
   BallisticsMassDialog(const QMap<QString,QString> &values,
                        const QMap<QString,QStringList> &choices, QWidget *parent=nullptr);
   QMap<QString,QString> values() const;
};
