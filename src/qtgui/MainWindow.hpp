#pragma once
#include <QMainWindow>
#include <memory>
class QMdiArea;
class QPlainTextEdit;
class QTreeWidget;
class QCloseEvent;
class QtMessageReceiver;
class MainWindow : public QMainWindow
{
public:
   MainWindow();
   ~MainWindow() override;
   bool initialize(const QString &startup);
   bool loadScript(const QString &path);
   bool buildScript();
protected:
   void closeEvent(QCloseEvent *event) override;
private:
   void refreshTrees();
   void newMission();
   bool saveScript(bool saveAs = false);
   bool confirmDiscard();
   void updateTitle();
   QMdiArea *workspace;
   QPlainTextEdit *editor;
   QPlainTextEdit *messages;
   QTreeWidget *resources;
   QTreeWidget *mission;
   QTreeWidget *output;
   std::unique_ptr<QtMessageReceiver> receiver;
   QString scriptPath;
   bool ready = false;
};
