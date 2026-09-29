#include "MainWindow.hpp"
#include "QtMessageReceiver.hpp"
#include "QtInterpreter.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotInterface.hpp"
#include "Moderator.hpp"
#include "MessageInterface.hpp"
#include "BaseException.hpp"
#include <QAction>
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
#include <QSettings>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextDocument>
#include <QToolBar>
#include <QTreeWidget>
#include <QStyle>
#include <sstream>
#include <set>

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
   auto editAction = [edit, this](const QString &label, const QKeySequence &key, auto slot) {
      auto *action = edit->addAction(label); action->setShortcut(key);
      connect(action, &QAction::triggered, editor, slot);
      editingActions.append(action);
   };
   editAction("&Undo", QKeySequence::Undo, &QPlainTextEdit::undo);
   editAction("&Redo", QKeySequence::Redo, &QPlainTextEdit::redo);
   edit->addSeparator();
   editAction("Cu&t", QKeySequence::Cut, &QPlainTextEdit::cut);
   editAction("&Copy", QKeySequence::Copy, &QPlainTextEdit::copy);
   editAction("&Paste", QKeySequence::Paste, &QPlainTextEdit::paste);
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
   connect(windows->addAction("&Tile"), &QAction::triggered, workspace, &QMdiArea::tileSubWindows);
   connect(windows->addAction("&Cascade"), &QAction::triggered, workspace, &QMdiArea::cascadeSubWindows);
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
      auto *panel = new ResourceEditor(*object, [this, name, snapshot](const QMap<QString, QString> &changes) {
         return applyResourceChanges(name, changes, snapshot);
      });
      auto *child = new ResourceSubWindow;
      child->setWidget(panel);
      workspace->addSubWindow(child);
      child->setAttribute(Qt::WA_DeleteOnClose);
      child->setProperty("resourcePanel", true);
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
      auto *panel = dynamic_cast<ResourceEditor *>(child->widget());
      if (panel && panel->hasChanges()) return false;
   }
   QFile file(path);
   if (!file.open(QIODevice::ReadOnly)) { QMessageBox::warning(this, "Open failed", file.errorString()); return false; }
   const QByteArray bytes = file.readAll();
   if (file.error() != QFileDevice::NoError) { QMessageBox::warning(this, "Read failed", file.errorString()); return false; }
   editor->setPlainText(QString::fromUtf8(bytes));
   scriptPath = path; editor->document()->setModified(false); updateTitle(); return true;
}
bool MainWindow::saveScript(bool saveAs)
{
   if (running) return false;
   QString path = scriptPath;
   if (saveAs || path.isEmpty()) path = QFileDialog::getSaveFileName(this, "Save GMAT script", path, "GMAT scripts (*.script)");
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
      auto *panel = dynamic_cast<ResourceEditor *>(child->widget());
      if (panel && panel->hasChanges()) {
         if (QMessageBox::question(this, "Unapplied resource changes",
             "Resource panels contain unapplied changes. Discard them and continue?",
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
      if (auto *panel = dynamic_cast<ResourceEditor *>(child->widget())) {
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
      auto *panel = dynamic_cast<ResourceEditor *>(child->widget());
      if (panel && panel->hasChanges()) {
         statusBar()->showMessage("Apply or discard the open resource changes before building or running");
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
   std::set<GmatCommand *> seen;
   for (auto *command = Moderator::Instance()->GetFirstCommand(); command && seen.insert(command).second; command = command->GetNext())
      new QTreeWidgetItem(sequence, {QString::fromStdString(command->GetTypeName())});
   sequence->setExpanded(true);
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
      if (child->property("resourcePanel").toBool()) child->widget()->setEnabled(!value);
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
         try { setResourceProperty(*proposed, it.key(), it.value()); }
         catch (BaseException &error) { return it.key() + ": " + QString::fromStdString(error.GetFullMessage()); }
         catch (const std::exception &error) { return it.key() + ": " + QString::fromUtf8(error.what()); }
      }
      if (!proposed->Validate()) return "The resource rejected these settings.";
      candidate = QString::fromStdString(moderator->GetScript(Gmat::SCRIPTING));
      const auto oldBlock = QString::fromStdString(object->GetGeneratingString(Gmat::SCRIPTING));
      const auto newBlock = QString::fromStdString(proposed->GetGeneratingString(Gmat::SCRIPTING));
      if (oldBlock.isEmpty() || candidate.count(oldBlock) != 1)
         return "This resource requires a specialized editor. Use its script settings for now.";
      candidate.replace(candidate.indexOf(oldBlock), oldBlock.size(), newBlock);
   } catch (BaseException &error) { return QString::fromStdString(error.GetFullMessage()); }
   catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
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
   statusBar()->showMessage("Resource updated — save the script to keep changes");
   return {};
}
