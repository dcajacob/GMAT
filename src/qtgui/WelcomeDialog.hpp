#pragma once
#include <QDialog>
#include <QStringList>
#include <functional>
class QLabel;
class QListWidget;
QStringList recentMissionFiles();
void rememberMissionFile(const QString &path);
class WelcomeDialog final : public QDialog
{
public:
   WelcomeDialog(const QString &root,std::function<bool(const QString &)> open,
      std::function<bool()> create,std::function<void(const QString &)> help,QWidget *parent=nullptr);
   void refreshRecent();
private:
   QListWidget *recent;
   QLabel *status;
};
