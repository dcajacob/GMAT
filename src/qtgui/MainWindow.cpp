#include "MainWindow.hpp"
#include "QtMessageReceiver.hpp"
#include "QtInterpreter.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "CommandEditor.hpp"
#include "MissionModel.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include "QtPlotReceiver.hpp"
#include "PlotInterface.hpp"
#include "Moderator.hpp"
#include "MessageInterface.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSaveFile>
#include <QStringDecoder>
#include <QSettings>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextDocument>
#include <QTextEdit>
#include <QToolBar>
#include <QTreeWidget>
#include <QStyle>
#include <sstream>
#include <set>

namespace {
QStringList creatableResourceTypes()
{
   QStringList result;
   for (const auto category : {Gmat::SPACECRAFT,Gmat::HARDWARE,Gmat::BURN,Gmat::PROP_SETUP,
         Gmat::ODE_MODEL,Gmat::COORDINATE_SYSTEM,Gmat::SOLVER,Gmat::SUBSCRIBER})
      for (const auto &type : Moderator::Instance()->GetListOfViewableItems(category))
         result.append(QString::fromStdString(type));
   result.append({"Variable","String"});
   result.removeDuplicates(); result.sort(); return result;
}
}

MainWindow::MainWindow()
{
   resize(1280, 850);
   workspace = new QMdiArea(this);
   workspace->setObjectName("workspace");
   workspace->setBackground(palette().mid());
   setCentralWidget(workspace);
   plots = std::make_unique<QtPlotReceiver>(workspace);
   auto *navigation = new QDockWidget("Mission workspace", this);
   navigation->setObjectName("navigation");
   auto *tabs = new QTabWidget(navigation);
   auto makeTree = [tabs](const QString &name) {
      auto *tree = new QTreeWidget(tabs);
      tree->setObjectName(name);
      tree->setHeaderHidden(true);
      tree->setMinimumWidth(230);
      tree->setAlternatingRowColors(false);
      tabs->addTab(tree, name);
      return tree;
   };
   resources = makeTree("Resources");
   mission = makeTree("Mission");
   output = makeTree("Output");
   mission->setContextMenuPolicy(Qt::CustomContextMenu);
   connect(mission, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
      if (item->data(0,Qt::UserRole).isValid()) openCommandEditor(item->data(0,Qt::UserRole).toInt(),MissionEdit::Replace);
   });
   connect(mission, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint &position) {
      if (!ready || running || !modelValid) return;
      auto *item=mission->itemAt(position);
      const int index=item && item->data(0,Qt::UserRole).isValid() ? item->data(0,Qt::UserRole).toInt() : -1;
      QMenu menu(this);
      QAction *edit=nullptr,*before=nullptr,*after=nullptr,*remove=nullptr;
      if (index>=0 && index<missionState.nodes.size()) {
         edit=menu.addAction("Edit command…"); edit->setEnabled(missionState.nodes[index].editable);
         before=menu.addAction("Insert before…"); before->setEnabled(missionState.nodes[index].type!="BeginMissionSequence");
         after=menu.addAction("Insert after…");
         remove=menu.addAction("Delete command"); remove->setEnabled(missionState.nodes[index].editable);
         menu.addSeparator();
      }
      auto *append=menu.addAction("Append command…");
      auto *chosen=menu.exec(mission->viewport()->mapToGlobal(position));
      if (!chosen) return;
      if (chosen==append) openCommandEditor(-1,MissionEdit::Append);
      else if (chosen==edit) openCommandEditor(index,MissionEdit::Replace);
      else if (chosen==before) openCommandEditor(index,MissionEdit::InsertBefore);
      else if (chosen==after) openCommandEditor(index,MissionEdit::InsertAfter);
      else if (chosen==remove) {
         const auto snapshot=missionState;
         if (QMessageBox::question(this,"Delete command","Delete this command and any commands inside its branch?",
             QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel)==QMessageBox::Yes) {
            const auto error=applyMissionChange(snapshot,index,MissionEdit::Remove,{});
            if (!error.isEmpty()) statusBar()->showMessage(error);
         }
      }
   });
   plots->changed = [this] { refreshOutput(); };
   connect(output, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
      const auto name = item->data(0, Qt::UserRole).toString();
      if (item->data(0, Qt::UserRole+1).toString()=="report") {
         if (running) { statusBar()->showMessage("Wait until the mission stops before opening its report"); return; }
         QFile file(name);
         if (!file.open(QIODevice::ReadOnly)) { statusBar()->showMessage("Report is not available: " + file.errorString()); return; }
         constexpr qint64 limit = 16 * 1024 * 1024;
         const auto bytes = file.read(limit);
         if (file.error()!=QFileDevice::NoError) { statusBar()->showMessage("Report read failed: " + file.errorString()); return; }
         auto *viewer = new QPlainTextEdit;
         viewer->setObjectName("report:" + item->text(0));
         viewer->setReadOnly(true);
         viewer->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
         viewer->setLineWrapMode(QPlainTextEdit::NoWrap);
         viewer->setPlainText(QString::fromUtf8(bytes));
         if (!file.atEnd()) viewer->appendPlainText("\n[Preview limited to the first 16 MiB. The complete report is saved at " + name + "]");
         auto *child = workspace->addSubWindow(viewer);
         child->setAttribute(Qt::WA_DeleteOnClose);
         child->setWindowTitle(item->text(0) + " — Report");
         child->resize(750,500); child->show();
      } else if (!name.isEmpty()) plots->show(name);
   });
   navigation->setWidget(tabs);
   addDockWidget(Qt::LeftDockWidgetArea, navigation);
   auto *console = new QDockWidget("Message Window", this);
   console->setObjectName("messages");
   messages = new QPlainTextEdit(console);
   messages->setReadOnly(true);
   messages->setMaximumBlockCount(10000);
   messages->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
   console->setWidget(messages);
   addDockWidget(Qt::BottomDockWidgetArea, console);
   resizeDocks({navigation}, {270}, Qt::Horizontal);
   resizeDocks({console}, {160}, Qt::Vertical);
   editor = new QPlainTextEdit;
   editor->setObjectName("scriptEditor");
   editor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
   editor->setLineWrapMode(QPlainTextEdit::NoWrap);
   auto *document = workspace->addSubWindow(editor);
   document->setWindowTitle("Script");
   document->setWindowFlags(document->windowFlags() & ~Qt::WindowCloseButtonHint);
   document->resize(850, 540);
   document->showMaximized();
   connect(editor->document(), &QTextDocument::modificationChanged, this, [this] { updateTitle(); });
   auto *file = menuBar()->addMenu("&File");
   auto *edit = menuBar()->addMenu("&Edit");
   auto *view = menuBar()->addMenu("&View");
   auto *run = menuBar()->addMenu("&Mission");
   auto *windows = menuBar()->addMenu("&Window");
   auto *help = menuBar()->addMenu("&Help");
   auto *toolbar = addToolBar("Standard");
   toolbar->setObjectName("standardToolbar");
   auto add = [this, toolbar](QMenu *menu, const QString &label, QStyle::StandardPixmap icon, const QKeySequence &key, auto callback) {
      auto *action = menu->addAction(style()->standardIcon(icon), label);
      action->setShortcut(key);
      connect(action, &QAction::triggered, this, callback);
      toolbar->addAction(action);
      editingActions.append(action);
      return action;
   };
   add(file, "&New mission", QStyle::SP_FileIcon, QKeySequence::New, [this] { if (confirmDiscard()) newMission(); });
   add(file, "&Open script…", QStyle::SP_DialogOpenButton, QKeySequence::Open, [this] {
      const auto path = QFileDialog::getOpenFileName(this, "Open GMAT script", scriptPath, "GMAT scripts (*.script);;All files (*)");
      if (!path.isEmpty() && confirmDiscard() && loadScript(path)) buildScript();
   });
   add(file, "&Save", QStyle::SP_DialogSaveButton, QKeySequence::Save, [this] { saveScript(); });
   auto *saveAs = file->addAction("Save &As…");
   saveAs->setShortcut(QKeySequence::SaveAs);
   editingActions.append(saveAs);
   connect(saveAs, &QAction::triggered, this, [this] { saveScript(true); });
   file->addSeparator();
   connect(file->addAction("E&xit"), &QAction::triggered, this, &QWidget::close);
   connect(qApp,&QApplication::focusChanged,this,[this](QWidget *,QWidget *focused) {
      // Menus temporarily take focus; keep the editor they were opened from.
      if (!focused || qobject_cast<QMenu *>(focused) || focused==menuBar()) return;
      if (isAncestorOf(focused) && (qobject_cast<QPlainTextEdit *>(focused) ||
          qobject_cast<QTextEdit *>(focused) || qobject_cast<QLineEdit *>(focused))) textEditTarget=focused;
      else textEditTarget.clear();
   });
   auto editAction = [edit, this](const QString &label, const QKeySequence &key, const char *slot) {
      auto *action = edit->addAction(label); action->setShortcut(key);
      action->setObjectName(QString("edit_")+slot);
      connect(action, &QAction::triggered, this, [this,slot] {
         if (textEditTarget && textEditTarget->isEnabled()) QMetaObject::invokeMethod(textEditTarget,slot,Qt::DirectConnection);
      });
      editingActions.append(action);
   };
   editAction("&Undo", QKeySequence::Undo, "undo");
   editAction("&Redo", QKeySequence::Redo, "redo");
   edit->addSeparator();
   editAction("Cu&t", QKeySequence::Cut, "cut");
   editAction("&Copy", QKeySequence::Copy, "copy");
   editAction("&Paste", QKeySequence::Paste, "paste");
   edit->addSeparator();
   auto *create=edit->addAction("New &resource…");
   create->setObjectName("createResource"); editingActions.append(create);
   connect(create,&QAction::triggered,this,&MainWindow::showCreateResource);
   resources->setContextMenuPolicy(Qt::CustomContextMenu);
   connect(resources,&QTreeWidget::customContextMenuRequested,this,[this,create](const QPoint &position) {
      QMenu menu(this); menu.addAction(create); menu.exec(resources->viewport()->mapToGlobal(position));
   });
   view->addAction(navigation->toggleViewAction());
   view->addAction(console->toggleViewAction());
   toolbar->addSeparator();
   add(run, "&Build script", QStyle::SP_BrowserReload, QKeySequence("F7"), [this] { buildScript(); });
   runAction = add(run, "&Run mission", QStyle::SP_MediaPlay, QKeySequence("F5"), [this] {
      if (paused) resumeMission(); else runMission();
   });
   runAction->setObjectName("runMission");
   editingActions.removeOne(runAction);
   pauseAction = add(run, "&Pause", QStyle::SP_MediaPause, QKeySequence("F6"), [this] { pauseMission(); });
   pauseAction->setObjectName("pauseMission");
   editingActions.removeOne(pauseAction);
   stopAction = add(run, "&Stop", QStyle::SP_MediaStop, QKeySequence("Shift+F5"), [this] { stopMission(); });
   stopAction->setObjectName("stopMission");
   editingActions.removeOne(stopAction);
   setRunning(false);
   windows->setObjectName("windowMenu");
   connect(windows,&QMenu::aboutToShow,this,[this,windows] {
      windows->clear();
      connect(windows->addAction("&Tile"),&QAction::triggered,workspace,&QMdiArea::tileSubWindows);
      connect(windows->addAction("&Cascade"),&QAction::triggered,workspace,&QMdiArea::cascadeSubWindows);
      connect(windows->addAction("&Next window"),&QAction::triggered,workspace,&QMdiArea::activateNextSubWindow);
      connect(windows->addAction("&Previous window"),&QAction::triggered,workspace,&QMdiArea::activatePreviousSubWindow);
      windows->addSeparator();
      for (auto *child:workspace->subWindowList()) {
         if (!child->isVisible()) continue;
         auto *action=windows->addAction(child->windowTitle().replace("&","&&"));
         action->setObjectName("windowEntry"); action->setCheckable(true);
         action->setChecked(child==workspace->activeSubWindow());
         action->setData(child->windowTitle());
         const QPointer<QMdiSubWindow> guarded=child;
         connect(action,&QAction::triggered,this,[this,guarded] {
            if (!guarded) return;
            if (guarded->isMinimized()) guarded->showNormal();
            workspace->setActiveSubWindow(guarded); guarded->raise();
            guarded->widget()->setFocus();
         });
      }
   });
   connect(help->addAction("&About GMAT"), &QAction::triggered, this, [this] {
      QMessageBox::about(this, "GMAT", "General Mission Analysis Tool\nQt 6 desktop interface");
   });
   connect(resources, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
      if (!ready || running || item->data(0, Qt::UserRole).toString().isEmpty()) return;
      auto *object = Moderator::Instance()->GetConfiguredObject(item->text(0).toStdString());
      if (!object) return;
      if (!modelValid || editor->toPlainText() != builtScript) {
         statusBar()->showMessage("Build the edited script before opening a resource panel");
         return;
      }
      const QString name = item->text(0);
      const QString snapshot = builtScript;
      for (auto *child:workspace->subWindowList()) {
         if (child->property("resourceName").toString()!=name) continue;
         auto *panel=dynamic_cast<ResourceEditor *>(child->widget());
         if (!panel) continue;
         if (child->property("sourceScript").toString()==snapshot || panel->hasChanges()) {
            if (child->isMinimized()) child->showNormal();
            workspace->setActiveSubWindow(child); child->raise(); panel->setFocus();
            if (child->property("sourceScript").toString()!=snapshot)
               statusBar()->showMessage("This panel has changes from an older mission. Close and discard them before reopening.");
            return;
         }
         child->close();
      }
      auto *panel = new ResourceEditor(*object, [this, name, snapshot](const QMap<QString, QString> &changes) {
         return applyResourceChanges(name, changes, snapshot);
      });
      auto *child = new EditorSubWindow;
      child->setWidget(panel);
      workspace->addSubWindow(child);
      child->setAttribute(Qt::WA_DeleteOnClose);
      child->setProperty("configurationPanel", true);
      child->setProperty("resourceName",name);
      child->setProperty("sourceScript",snapshot);
      child->setWindowTitle(name); child->resize(680, 540); child->show();
   });
   QSettings settings;
   restoreGeometry(settings.value("geometry").toByteArray());
   restoreState(settings.value("windowState").toByteArray());
   updateTitle();
}
MainWindow::~MainWindow()
{
   plots->changed = {};
   Moderator::SetUiInterpreter(nullptr);
   if (ready) Moderator::Instance()->Finalize();
   PlotInterface::SetPlotReceiver(nullptr);
   MessageInterface::SetMessageReceiver(nullptr);
}
bool MainWindow::initialize(const QString &startup)
{
   receiver = std::make_unique<QtMessageReceiver>();
   receiver->SetMessageCallback(this, [this](const QString &text) {
      messages->moveCursor(QTextCursor::End); messages->insertPlainText(text);
   });
   MessageInterface::SetMessageReceiver(receiver.get());
   PlotInterface::SetPlotReceiver(plots.get());
   try {
      ready = Moderator::Instance()->Initialize(startup.toStdString(), true);
      if (ready) {
         interpreter = std::make_unique<QtInterpreter>();
         Moderator::SetUiInterpreter(interpreter.get());
         newMission();
      }
   } catch (BaseException &error) { messages->appendPlainText(QString::fromStdString(error.GetFullMessage())); }
   statusBar()->showMessage(ready ? "Ready" : "Runtime initialization failed");
   return ready;
}
void MainWindow::newMission()
{
   if (!ready || running) return;
   plots->clear();
   Moderator::Instance()->LoadDefaultMission();
   editor->setPlainText(QString::fromStdString(Moderator::Instance()->GetScript(Gmat::SCRIPTING)));
   builtScript = editor->toPlainText(); modelValid = true;
   scriptPath.clear(); editor->document()->setModified(false); refreshTrees(); updateTitle();
}
bool MainWindow::loadScript(const QString &path)
{
   if (running) return false;
   for (auto *child : workspace->subWindowList()) {
      auto *panel = dynamic_cast<EditablePanel *>(child->widget());
      if (panel && panel->hasChanges()) return false;
   }
   QFile file(path);
   if (!file.open(QIODevice::ReadOnly)) { QMessageBox::warning(this, "Open failed", file.errorString()); return false; }
   const QByteArray bytes = file.readAll();
   if (file.error() != QFileDevice::NoError) { QMessageBox::warning(this, "Read failed", file.errorString()); return false; }
   QStringDecoder decoder(QStringDecoder::Utf8,QStringConverter::Flag::Stateless);
   const QString text=decoder(bytes);
   if (decoder.hasError()) {
      QMessageBox::warning(this,"Open failed","This script is not valid UTF-8. Convert its encoding before opening it.");
      return false;
   }
   editor->setPlainText(text);
   scriptPath = path; editor->document()->setModified(false); updateTitle(); return true;
}
bool MainWindow::saveScript(bool saveAs)
{
   if (running) return false;
   QString path = scriptPath;
   if (saveAs || path.isEmpty()) path = QFileDialog::getSaveFileName(this, "Save GMAT script", path, "GMAT scripts (*.script)");
   return saveScriptTo(path);
}
bool MainWindow::saveScriptTo(const QString &path)
{
   if (running) return false;
   if (path.isEmpty()) return false;
   QSaveFile file(path);
   const QByteArray bytes = editor->toPlainText().toUtf8();
   if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
      QMessageBox::warning(this, "Save failed", file.errorString()); return false;
   }
   scriptPath = path; editor->document()->setModified(false); updateTitle(); return true;
}
bool MainWindow::confirmDiscard()
{
   for (auto *child : workspace->subWindowList()) {
      auto *panel = dynamic_cast<EditablePanel *>(child->widget());
      if (panel && panel->hasChanges()) {
         if (QMessageBox::question(this, "Unapplied panel changes",
             "Panels contain unapplied changes. Discard them and continue?",
             QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Discard)
            return false;
         break;
      }
   }
   if (editor->document()->isModified()) {
      const auto answer = QMessageBox::question(this, "Unsaved script", "Save your changes before continuing?", QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
      if (answer != QMessageBox::Discard && !(answer == QMessageBox::Save && saveScript())) return false;
   }
   for (auto *child : workspace->subWindowList()) {
      if (auto *panel = dynamic_cast<EditablePanel *>(child->widget())) {
         panel->discardChanges();
         child->close();
      }
   }
   return true;
}
bool MainWindow::buildScript()
{
   if (!ready || running) return false;
   for (auto *child : workspace->subWindowList()) {
      auto *panel = dynamic_cast<EditablePanel *>(child->widget());
      if (panel && panel->hasChanges()) {
         statusBar()->showMessage("Apply or discard the open panel changes before building or running");
         return false;
      }
   }
   bool success = false;
   plots->clear();
   try {
      std::istringstream stream(editor->toPlainText().toStdString());
      success = Moderator::Instance()->InterpretScript(&stream, true);
   } catch (BaseException &error) { messages->appendPlainText(QString::fromStdString(error.GetFullMessage())); }
   catch (const std::exception &error) { messages->appendPlainText(QString::fromUtf8(error.what())); }
   catch (...) { messages->appendPlainText("Unexpected error while building the script."); }
   modelValid = success;
   if (success) builtScript = editor->toPlainText();
   refreshTrees(); statusBar()->showMessage(success ? "Build succeeded" : "Build failed — see Message Window"); return success;
}
void MainWindow::refreshTrees()
{
   resources->clear(); mission->clear(); output->clear();
   reportFiles.clear();
   auto *root = new QTreeWidgetItem(resources, {"Resources"});
   const std::pair<const char *, UnsignedInt> groups[] = {
      {"Spacecraft", Gmat::SPACECRAFT}, {"Hardware", Gmat::HARDWARE},
      {"Formations", Gmat::FORMATION}, {"Ground Stations", Gmat::GROUND_STATION},
      {"Propagators", Gmat::PROP_SETUP}, {"Burns", Gmat::BURN},
      {"Force Models", Gmat::ODE_MODEL},
      {"Coordinate Systems", Gmat::COORDINATE_SYSTEM}, {"Solvers", Gmat::SOLVER},
      {"Output", Gmat::SUBSCRIBER}, {"Variables, Arrays, Strings", Gmat::PARAMETER},
      {"Functions", Gmat::FUNCTION}};
   for (const auto &group : groups) {
      auto *category = new QTreeWidgetItem(root, {group.first});
      for (const auto &name : Moderator::Instance()->GetListOfObjects(group.second)) {
         if (group.second == Gmat::SUBSCRIBER) {
            auto *object = Moderator::Instance()->GetConfiguredObject(name);
            if (object && object->IsOfType(Gmat::REPORT_FILE))
               reportFiles.insert(QString::fromStdString(name), QString::fromStdString(object->GetStringParameter("FullPathFileName")));
         }
         if (group.second == Gmat::PARAMETER) {
            auto *parameter = dynamic_cast<Parameter *>(Moderator::Instance()->GetConfiguredObject(name));
            if (!parameter || parameter->GetKey() == GmatParam::SYSTEM_PARAM) continue;
         }
         auto *item = new QTreeWidgetItem(category, {QString::fromStdString(name)});
         item->setData(0, Qt::UserRole, QString::fromStdString(name));
      }
   }
   root->setExpanded(true); resources->expandToDepth(1);
   auto *sequence = new QTreeWidgetItem(mission, {"Mission Sequence"});
   missionState = {};
   if (modelValid) {
      try {
         missionState = snapshotMission(Moderator::Instance()->GetFirstCommand(),
            QString::fromStdString(Moderator::Instance()->GetScript(Gmat::SCRIPTING)), builtScript);
         std::function<void(int,QTreeWidgetItem *)> addNode = [&](int index, QTreeWidgetItem *parent) {
            const auto &node = missionState.nodes[index];
            auto *item = new QTreeWidgetItem(parent, {node.label});
            item->setData(0, Qt::UserRole, index);
            item->setData(0, Qt::UserRole+1, node.type);
            item->setToolTip(0, node.statement);
            for (const auto child : node.children) addNode(child,item);
         };
         for (const auto index : missionState.roots) addNode(index,sequence);
      } catch (BaseException &error) { messages->appendPlainText(QString::fromStdString(error.GetFullMessage())); }
      catch (const std::exception &error) { messages->appendPlainText(QString::fromUtf8(error.what())); }
   }
   mission->expandToDepth(2);
   refreshOutput();
}
void MainWindow::refreshOutput()
{
   output->clear();
   auto *reports = new QTreeWidgetItem(output, {"Reports"});
   for (auto it = reportFiles.cbegin(); it != reportFiles.cend(); ++it) {
      auto *item = new QTreeWidgetItem(reports, {it.key()});
      item->setData(0, Qt::UserRole, it.value());
      item->setData(0, Qt::UserRole+1, "report");
   }
   reports->setExpanded(true);
   auto *folder = new QTreeWidgetItem(output, {"Plots"});
   for (const auto &name : plots->names()) {
      auto *item = new QTreeWidgetItem(folder, {name});
      item->setData(0, Qt::UserRole, name);
   }
   folder->setExpanded(true);
}
void MainWindow::updateTitle()
{
   setWindowTitle(QString("%1%2 — GMAT Qt 6").arg(scriptPath.isEmpty() ? "Untitled" : QFileInfo(scriptPath).fileName(), editor->document()->isModified() ? " *" : ""));
}
void MainWindow::closeEvent(QCloseEvent *event)
{
   if (running) {
      stopMission();
      statusBar()->showMessage("Stopping mission — close again after execution ends");
      event->ignore();
      return;
   }
   if (!confirmDiscard()) { event->ignore(); return; }
   QSettings settings; settings.setValue("geometry", saveGeometry()); settings.setValue("windowState", saveState());
   event->accept();
}

void MainWindow::setRunning(bool value)
{
   running = value;
   for (auto *action : editingActions) action->setEnabled(!value);
   editor->setReadOnly(value);
   for (auto *child : workspace->subWindowList())
      if (child->property("configurationPanel").toBool()) child->widget()->setEnabled(!value);
   resources->setEnabled(!value);
   mission->setEnabled(!value);
   runAction->setEnabled(!value);
   runAction->setText("&Run mission");
   pauseAction->setEnabled(value);
   stopAction->setEnabled(value);
}
MainWindow::RunResult MainWindow::runMission()
{
   if (running) return RunResult::Busy;
   if (!buildScript()) return RunResult::Failed;
   paused = false;
   stopRequested = false;
   setRunning(true);
   statusBar()->showMessage("Running mission…");
   RunResult result = RunResult::Failed;
   try {
      const auto status = Moderator::Instance()->RunMission();
      if (status == 1) result = RunResult::Completed;
      else if (status == -4 && stopRequested) result = RunResult::Stopped;
   } catch (BaseException &error) {
      messages->appendPlainText(QString::fromStdString(error.GetFullMessage()));
   } catch (const std::exception &error) {
      messages->appendPlainText(QString::fromUtf8(error.what()));
   } catch (...) {
      messages->appendPlainText("Unexpected error during mission execution.");
   }
   paused = false;
   setRunning(false);
   statusBar()->showMessage(result == RunResult::Completed ? "Mission completed" :
      result == RunResult::Stopped ? "Mission stopped" : "Mission failed — see Message Window");
   return result;
}
void MainWindow::pauseMission()
{
   if (!running || paused || stopRequested) return;
   Moderator::Instance()->ChangeRunState("Pause");
   paused = true;
   runAction->setText("&Resume mission");
   runAction->setEnabled(true);
   pauseAction->setEnabled(false);
   statusBar()->showMessage("Mission paused");
}
void MainWindow::resumeMission()
{
   if (!running || !paused || stopRequested) return;
   Moderator::Instance()->ChangeRunState("Resume");
   paused = false;
   runAction->setText("&Run mission");
   runAction->setEnabled(false);
   pauseAction->setEnabled(true);
   statusBar()->showMessage("Running mission…");
}
void MainWindow::stopMission()
{
   if (!running || stopRequested) return;
   stopRequested = true;
   Moderator::Instance()->ChangeRunState("Stop");
   runAction->setEnabled(false);
   pauseAction->setEnabled(false);
   stopAction->setEnabled(false);
   statusBar()->showMessage("Stopping mission…");
}

QString MainWindow::applyResourceChanges(const QString &name,
      const QMap<QString, QString> &changes, const QString &expectedScript)
{
   if (running) return "Stop the mission before editing resources.";
   if (!modelValid || expectedScript != builtScript || editor->toPlainText() != builtScript)
      return "The mission has changed. Build the current script and reopen this panel.";
   if (changes.isEmpty()) return {};
   auto *moderator = Moderator::Instance();
   auto *object = moderator->GetConfiguredObject(name.toStdString());
   if (!object) return "This resource no longer exists. Reopen the panel.";
   QString candidate;
   try {
      std::unique_ptr<GmatBase> proposed(object->Clone());
      if (!proposed) return "This resource cannot be edited.";
      for (auto it = changes.cbegin(); it != changes.cend(); ++it) {
         if (isResourceList(*proposed,it.key())) continue;
         try { setResourceProperty(*proposed, it.key(), it.value()); }
         catch (BaseException &error) { return it.key() + ": " + QString::fromStdString(error.GetFullMessage()); }
         catch (const std::exception &error) { return it.key() + ": " + QString::fromUtf8(error.what()); }
      }
      if (!proposed->Validate()) return "The resource rejected these settings.";
      candidate = QString::fromStdString(moderator->GetScript(Gmat::SCRIPTING));
      const auto oldBlock = QString::fromStdString(object->GetGeneratingString(Gmat::SCRIPTING));
      auto newBlock = QString::fromStdString(proposed->GetGeneratingString(Gmat::SCRIPTING));
      for (auto it=changes.cbegin();it!=changes.cend();++it)
         if (isResourceList(*proposed,it.key())) newBlock=replaceResourceList(*proposed,newBlock,it.key(),it.value());
      if (oldBlock.isEmpty() || candidate.count(oldBlock) != 1)
         return "This resource requires a specialized editor. Use its script settings for now.";
      candidate.replace(candidate.indexOf(oldBlock), oldBlock.size(), newBlock);
   } catch (BaseException &error) { return QString::fromStdString(error.GetFullMessage()); }
   catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
   return applyModelScript(candidate);
}

QString MainWindow::createResource(const QString &type,const QString &name,const QString &expectedScript)
{
   if (!ready || running || !modelValid || expectedScript!=builtScript || editor->toPlainText()!=builtScript)
      return "Build the current script before creating a resource.";
   for (auto *child:workspace->subWindowList())
      if (auto *panel=dynamic_cast<EditablePanel *>(child->widget());panel && panel->hasChanges())
         return "Apply or discard the open panel changes before creating a resource.";
   static const QRegularExpression identifier("^[A-Za-z][A-Za-z0-9_]*$");
   if (!identifier.match(name).hasMatch()) return "Use a name starting with a letter, followed by letters, digits or underscores.";
   if (!creatableResourceTypes().contains(type)) return "Select an available resource type.";
   if (Moderator::Instance()->GetConfiguredObject(name.toStdString())) return "That resource name is already in use.";
   return applyModelScript("Create "+type+" "+name+";\n"+builtScript);
}

void MainWindow::showCreateResource()
{
   if (!ready || running || !modelValid || editor->toPlainText()!=builtScript) {
      statusBar()->showMessage("Build the current script before creating a resource"); return;
   }
   const QString snapshot=builtScript;
   QDialog dialog(this); dialog.setWindowTitle("New resource"); dialog.setObjectName("newResourceDialog");
   auto *layout=new QFormLayout(&dialog);
   auto *type=new QComboBox(&dialog); type->setObjectName("resourceType");
   type->addItems(creatableResourceTypes()); type->setCurrentText("Spacecraft");
   auto *name=new QLineEdit(&dialog); name->setObjectName("resourceName");
   auto *status=new QLabel("Create the resource, then edit its properties.",&dialog); status->setWordWrap(true);
   layout->addRow("Type",type); layout->addRow("Name",name); layout->addRow(status);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);
   buttons->button(QDialogButtonBox::Ok)->setText("Create"); layout->addRow(buttons);
   connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,&dialog,[&] {
      const auto error=createResource(type->currentText(),name->text().trimmed(),snapshot);
      if (error.isEmpty()) dialog.accept(); else status->setText(error);
   });
   dialog.resize(460,180); name->setFocus();
   if (dialog.exec()==QDialog::Accepted) {
      auto *navigation=findChild<QDockWidget *>("navigation");
      navigation->show(); navigation->findChild<QTabWidget *>()->setCurrentWidget(resources);
      const auto items=resources->findItems(name->text().trimmed(),Qt::MatchExactly|Qt::MatchRecursive);
      if (items.size()==1) { resources->setCurrentItem(items.first()); resources->scrollToItem(items.first()); resources->itemDoubleClicked(items.first(),0); }
   }
}

QString MainWindow::applyModelScript(const QString &candidate)
{
   auto *moderator = Moderator::Instance();
   QString error;
   plots->clear();
   try {
      std::istringstream stream(candidate.toStdString());
      if (!moderator->InterpretScript(&stream, true)) error = "The mission rejected these changes. See Message Window.";
   } catch (BaseException &exception) { error = QString::fromStdString(exception.GetFullMessage()); }
   catch (const std::exception &exception) { error = QString::fromUtf8(exception.what()); }
   if (!error.isEmpty()) {
      // Interpretation reconstructs the model. Restore the entire prior model,
      // not just the last field, without touching the editor or its undo stack.
      modelValid = false;
      try {
         std::istringstream previous(builtScript.toStdString());
         modelValid = moderator->InterpretScript(&previous, true);
      } catch (...) { }
      refreshTrees();
      if (!modelValid) error += " Restoration failed; rebuild the script before continuing.";
      return error;
   }
   auto cursor = editor->textCursor();
   cursor.beginEditBlock();
   cursor.select(QTextCursor::Document);
   cursor.insertText(candidate);
   cursor.endEditBlock();
   builtScript = candidate;
   modelValid = true;
   refreshTrees();
   statusBar()->showMessage("Mission updated — save the script to keep changes");
   return {};
}

void MainWindow::openCommandEditor(int index,MissionEdit operation)
{
   if (running || !modelValid || editor->toPlainText()!=builtScript) {
      statusBar()->showMessage("Build the current script before editing mission commands"); return;
   }
   if (operation!=MissionEdit::Append && (index<0 || index>=missionState.nodes.size())) return;
   if (operation==MissionEdit::Replace && !missionState.nodes[index].editable) return;
   const auto snapshot=missionState;
   QString statement=operation==MissionEdit::Replace ? snapshot.nodes[index].statement : QString();
   const auto spacecraft=Moderator::Instance()->GetListOfObjects(Gmat::SPACECRAFT);
   const auto propagators=Moderator::Instance()->GetListOfObjects(Gmat::PROP_SETUP);
   const auto burns=Moderator::Instance()->GetListOfObjects(Gmat::IMPULSIVE_BURN);
   auto first=[](const StringArray &names,const char *fallback) { return QString::fromStdString(names.empty() ? fallback : names.front()); };
   const auto sat=first(spacecraft,"SpacecraftName"),prop=first(propagators,"PropagatorName"),burn=first(burns,"BurnName");
   const QMap<QString,QString> templates={
      {"Propagate",QString("Propagate %1(%2) {%2.ElapsedSecs = 600};").arg(prop,sat)},
      {"Maneuver",QString("Maneuver %1(%2);").arg(burn,sat)},
      {"Report",QString("Report ReportName %1.ElapsedSecs %1.X %1.Y %1.Z;").arg(sat)},
      {"Assignment","VariableName = 1;"},
      {"If","If VariableName > 0;\n   % Insert commands here.\nElse;\n   % Insert alternate commands here.\nEndIf;"},
      {"While","While VariableName < 10;\n   VariableName = VariableName + 1;\nEndWhile;"},
      {"For","For VariableName = 1:1:10;\n   % Insert commands here.\nEndFor;"},
      {"Target","Target SolverName;\n   % Add Vary, mission commands, and Achieve here.\nEndTarget;"},
      {"Stop","Stop;"}, {"Script event","BeginScript;\n   % Insert commands here.\nEndScript;"}};
   QStringList propagationChoices,spacecraftChoices;
   for (const auto &value:propagators) propagationChoices.append(QString::fromStdString(value));
   for (const auto &value:spacecraft) spacecraftChoices.append(QString::fromStdString(value));
   auto *panel=new CommandEditor(statement,operation!=MissionEdit::Replace,templates,
      [this,snapshot,index,operation](const QString &replacement) {
         return applyMissionChange(snapshot,index,operation,replacement);
      },propagationChoices,spacecraftChoices);
   auto *child=new EditorSubWindow;
   child->setWidget(panel); workspace->addSubWindow(child);
   child->setAttribute(Qt::WA_DeleteOnClose); child->setProperty("configurationPanel",true);
   child->setWindowTitle(operation==MissionEdit::Replace ? snapshot.nodes[index].label : "Insert mission command");
   child->resize(700,500); child->show();
   workspace->setActiveSubWindow(child);
}
QString MainWindow::applyMissionChange(const MissionSnapshot &snapshot,int index,MissionEdit operation,
                                     const QString &replacement)
{
   if (running) return "Stop the mission before editing commands.";
   if (!modelValid || snapshot.sourceScript!=builtScript || editor->toPlainText()!=builtScript)
      return "The mission has changed. Build the current script and reopen this panel.";
   try { return applyModelScript(editMission(snapshot,index,operation,replacement)); }
   catch (BaseException &error) { return QString::fromStdString(error.GetFullMessage()); }
   catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
}
