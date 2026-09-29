#include "MainWindow.hpp"
#include "QtMessageReceiver.hpp"
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
   connect(saveAs, &QAction::triggered, this, [this] { saveScript(true); });
   file->addSeparator();
   connect(file->addAction("E&xit"), &QAction::triggered, this, &QWidget::close);
   auto editAction = [edit, this](const QString &label, const QKeySequence &key, auto slot) {
      auto *action = edit->addAction(label); action->setShortcut(key);
      connect(action, &QAction::triggered, editor, slot);
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
   auto *runAction = add(run, "&Run mission", QStyle::SP_MediaPlay, QKeySequence("F5"), [this] {
      if (!buildScript()) return;
      try {
         const auto result = Moderator::Instance()->RunMission();
         statusBar()->showMessage(result == 1 ? "Mission completed" : "Mission failed");
      } catch (BaseException &error) { messages->appendPlainText(QString::fromStdString(error.GetFullMessage())); }
   });
   runAction->setObjectName("runMission");
   connect(windows->addAction("&Tile"), &QAction::triggered, workspace, &QMdiArea::tileSubWindows);
   connect(windows->addAction("&Cascade"), &QAction::triggered, workspace, &QMdiArea::cascadeSubWindows);
   connect(help->addAction("&About GMAT"), &QAction::triggered, this, [this] {
      QMessageBox::about(this, "GMAT", "General Mission Analysis Tool\nQt 6 desktop interface");
   });
   connect(resources, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
      if (!ready || item->data(0, Qt::UserRole).toString().isEmpty()) return;
      auto *object = Moderator::Instance()->GetConfiguredObject(item->text(0).toStdString());
      if (!object) return;
      auto *text = new QPlainTextEdit;
      text->setReadOnly(true);
      text->setPlainText(QString::fromStdString(object->GetGeneratingString(Gmat::SCRIPTING)));
      auto *child = workspace->addSubWindow(text);
      child->setAttribute(Qt::WA_DeleteOnClose);
      child->setWindowTitle(item->text(0)); child->resize(650, 450); child->show();
   });
   QSettings settings;
   restoreGeometry(settings.value("geometry").toByteArray());
   restoreState(settings.value("windowState").toByteArray());
   updateTitle();
}
MainWindow::~MainWindow()
{
   if (ready) Moderator::Instance()->Finalize();
   MessageInterface::SetMessageReceiver(nullptr);
}
bool MainWindow::initialize(const QString &startup)
{
   receiver = std::make_unique<QtMessageReceiver>();
   receiver->SetMessageCallback(this, [this](const QString &text) {
      messages->moveCursor(QTextCursor::End); messages->insertPlainText(text);
   });
   MessageInterface::SetMessageReceiver(receiver.get());
   try {
      ready = Moderator::Instance()->Initialize(startup.toStdString(), true);
      if (ready) newMission();
   } catch (BaseException &error) { messages->appendPlainText(QString::fromStdString(error.GetFullMessage())); }
   statusBar()->showMessage(ready ? "Ready" : "Runtime initialization failed");
   return ready;
}
void MainWindow::newMission()
{
   if (!ready) return;
   Moderator::Instance()->LoadDefaultMission();
   editor->setPlainText(QString::fromStdString(Moderator::Instance()->GetScript(Gmat::SCRIPTING)));
   scriptPath.clear(); editor->document()->setModified(false); refreshTrees(); updateTitle();
}
bool MainWindow::loadScript(const QString &path)
{
   QFile file(path);
   if (!file.open(QIODevice::ReadOnly)) { QMessageBox::warning(this, "Open failed", file.errorString()); return false; }
   const QByteArray bytes = file.readAll();
   if (file.error() != QFileDevice::NoError) { QMessageBox::warning(this, "Read failed", file.errorString()); return false; }
   editor->setPlainText(QString::fromUtf8(bytes));
   scriptPath = path; editor->document()->setModified(false); updateTitle(); return true;
}
bool MainWindow::saveScript(bool saveAs)
{
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
   if (!editor->document()->isModified()) return true;
   const auto answer = QMessageBox::question(this, "Unsaved script", "Save your changes before continuing?", QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
   return answer == QMessageBox::Discard || (answer == QMessageBox::Save && saveScript());
}
bool MainWindow::buildScript()
{
   if (!ready) return false;
   bool success = false;
   try {
      std::istringstream stream(editor->toPlainText().toStdString());
      success = Moderator::Instance()->InterpretScript(&stream, true);
   } catch (BaseException &error) { messages->appendPlainText(QString::fromStdString(error.GetFullMessage())); }
   refreshTrees(); statusBar()->showMessage(success ? "Build succeeded" : "Build failed — see Message Window"); return success;
}
void MainWindow::refreshTrees()
{
   resources->clear(); mission->clear(); output->clear();
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
   new QTreeWidgetItem(output, {"Reports"}); new QTreeWidgetItem(output, {"Plots"});
}
void MainWindow::updateTitle()
{
   setWindowTitle(QString("%1%2 — GMAT Qt 6").arg(scriptPath.isEmpty() ? "Untitled" : QFileInfo(scriptPath).fileName(), editor->document()->isModified() ? " *" : ""));
}
void MainWindow::closeEvent(QCloseEvent *event)
{
   if (!confirmDiscard()) { event->ignore(); return; }
   QSettings settings; settings.setValue("geometry", saveGeometry()); settings.setValue("windowState", saveState());
   event->accept();
}
