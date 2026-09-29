#pragma once
#include <QMainWindow>
#include <memory>
#include <QList>
#include <QMap>
class QMdiArea;
class QPlainTextEdit;
class QTreeWidget;
class QCloseEvent;
class QtMessageReceiver;
class QtInterpreter;
class QAction;
class MainWindow : public QMainWindow
{
public:
   MainWindow();
   ~MainWindow() override;
   bool initialize(const QString &startup);
   bool loadScript(const QString &path);
   bool buildScript();
   enum class RunResult { Completed, Stopped, Failed, Busy };
   RunResult runMission();
   void pauseMission();
   void resumeMission();
   void stopMission();
   bool isRunning() const { return running; }
   QString applyResourceChanges(const QString &name, const QMap<QString, QString> &changes,
                                const QString &expectedScript);
protected:
   void closeEvent(QCloseEvent *event) override;
private:
   void refreshTrees();
   void newMission();
   bool saveScript(bool saveAs = false);
   bool confirmDiscard();
   void updateTitle();
   void setRunning(bool value);
   QMdiArea *workspace;
   QPlainTextEdit *editor;
   QPlainTextEdit *messages;
   QTreeWidget *resources;
   QTreeWidget *mission;
   QTreeWidget *output;
   std::unique_ptr<QtMessageReceiver> receiver;
   std::unique_ptr<QtInterpreter> interpreter;
   QList<QAction *> editingActions;
   QAction *runAction = nullptr;
   QAction *pauseAction = nullptr;
   QAction *stopAction = nullptr;
   QString scriptPath;
   QString builtScript;
   bool modelValid = false;
   bool ready = false;
   bool running = false;
   bool paused = false;
   bool stopRequested = false;
};
