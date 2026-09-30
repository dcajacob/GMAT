#pragma once
#include <QDialog>
class AboutDialog final : public QDialog
{
public:
   AboutDialog(const QString &rootPath,QWidget *parent=nullptr);
};
