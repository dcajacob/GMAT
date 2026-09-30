#pragma once
#include <QMainWindow>
#include <memory>
#include <QList>
#include <QMap>
#include <QPointer>
#include "MissionModel.hpp"
class QMdiArea;
class QPlainTextEdit;
class QTreeWidget;
class QCloseEvent;
class QtMessageReceiver;
class QtInterpreter;
class QtPlotReceiver;
class QtSolverListenerManager;
class QAction;
class MainWindow : public QMainWindow
{
public:
   MainWindow();
   ~MainWindow() override;
   bool initialize(const QString &startup);
   bool loadScript(const QString &path);
   bool saveScriptTo(const QString &path);
   bool buildScript();
   bool convertOpenFramesScript();
   QStringList availableEngineTypes() const;
   enum class RunResult { Completed, Stopped, Failed, Busy };
   RunResult runMission();
   void pauseMission();
   void resumeMission();
   void stopMission();
   bool isRunning() const { return running; }
   QtPlotReceiver *plotReceiver() const { return plots.get(); }
   const MissionSnapshot &missionSnapshot() const { return missionState; }
   QString applyMissionChange(const MissionSnapshot &snapshot, int index,
                              MissionEdit operation, const QString &replacement);
   QString applyResourceChanges(const QString &name, const QMap<QString, QString> &changes,
                                const QString &expectedScript);
   QString createResource(const QString &type, const QString &name, const QString &expectedScript, int rows=1, int columns=1);
   QString deleteResource(const QString &name, const QString &expectedScript);
protected:
   void closeEvent(QCloseEvent *event) override;
private:
   QString savePlotProjection(const QString &name,bool perspective,double fov);
   void refreshTrees();
   void refreshOutput();
   void openCommandEditor(int index, MissionEdit operation);
   void showCreateResource();
   void showFileComparison(const QString &baseline={});
   QString applyModelScript(const QString &candidate);
   bool restoreBuiltModel();
   void setScriptDirectory();
   void newMission();
   bool saveScript(bool saveAs = false);
   void saveAndBuildScript(bool run);
   bool confirmDiscard();
   void updateTitle();
   void setRunning(bool value);
   QMdiArea *workspace;
   QPlainTextEdit *editor;
   QPlainTextEdit *messages;
   QPointer<QWidget> textEditTarget;
   QTreeWidget *resources;
   QTreeWidget *mission;
   QTreeWidget *output;
   std::unique_ptr<QtMessageReceiver> receiver;
   std::unique_ptr<QtInterpreter> interpreter;
   std::unique_ptr<QtPlotReceiver> plots;
   std::unique_ptr<QtSolverListenerManager> solverListeners;
   QList<QAction *> editingActions;
   QAction *runAction = nullptr;
   QAction *pauseAction = nullptr;
   QAction *stopAction = nullptr;
   QString scriptPath;
   QString startupDirectory;
   QString builtScript;
   QMap<QString, QString> reportFiles;
   QMap<QString,QPair<QString,QString>> ephemerisFiles;
   MissionSnapshot missionState;
   bool modelValid = false;
   bool ready = false;
   bool running = false;
   bool paused = false;
   bool stopRequested = false;
};
