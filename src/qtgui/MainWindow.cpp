#include "ReportViewer.hpp"
#include "MainWindow.hpp"
#include "QtMessageReceiver.hpp"
#include "QtInterpreter.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "SpacecraftOrbit.hpp"
#include "AtmosphereDialog.hpp"
#include "GroundStationDialog.hpp"
#include "OrbitViewDialog.hpp"
#include "ThrusterDialog.hpp"
#include "BurnDialog.hpp"
#include "EphemerisDialog.hpp"
#include "EphemerisFile.hpp"
#include "EventLocatorDialog.hpp"
#include "CommandEditor.hpp"
#include "MissionModel.hpp"
#include "StartupCompatibility.hpp"
#include "ScriptCompatibility.hpp"
#include "ScriptEditor.hpp"
#include "FindReplaceDialog.hpp"
#include <QDialog>
#include <QDir>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QRegularExpression>
#include "QtPlotReceiver.hpp"
#include "QtSolverListener.hpp"
#include "ListenerManagerInterface.hpp"
#include "PlotInterface.hpp"
#include "Moderator.hpp"
#include "MessageInterface.hpp"
#include "BaseException.hpp"
#include "FileManager.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QOpenGLWidget>
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
         Gmat::ODE_MODEL,Gmat::COORDINATE_SYSTEM,Gmat::SOLVER,Gmat::SUBSCRIBER,Gmat::FUNCTION,Gmat::EVENT_LOCATOR})
      for (const auto &type : Moderator::Instance()->GetListOfViewableItems(category))
         result.append(QString::fromStdString(type));
   result.append({"Variable","String","Array"});
   result.removeDuplicates(); result.sort(); return result;
}
}

MainWindow::MainWindow()
{
   // Establish OpenGL composition before the top-level window is first shown.
   // Otherwise Qt replaces the native surface when the first OrbitView is
   // added during Run, which can disrupt Wayland input/focus on the old surface.
   // Keep this hidden child alive even when all plot windows have been closed.
   if (QGuiApplication::platformName()!="offscreen" && QGuiApplication::platformName()!="minimal") {
      auto *compositionAnchor=new QOpenGLWidget(this);
      compositionAnchor->setObjectName("compositionAnchor");
      compositionAnchor->resize(1,1);
      compositionAnchor->hide();
   }
   resize(1280, 850);
   workspace = new QMdiArea(this);
   workspace->setObjectName("workspace");
   workspace->setBackground(palette().mid());
   setCentralWidget(workspace);
   plots = std::make_unique<QtPlotReceiver>(workspace);
   solverListeners=std::make_unique<QtSolverListenerManager>(workspace);
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
   plots->saveProjection=[this](const QString &name,bool perspective,double fov) { return savePlotProjection(name,perspective,fov); };
   connect(output, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
      const auto name = item->data(0, Qt::UserRole).toString();
      const auto type=item->data(0,Qt::UserRole+1).toString(),format=item->data(0,Qt::UserRole+2).toString();
      if (type=="ephemeris" && format!="CCSDS-OEM" && format!="STK-TimePosVel") {
         if (running) { statusBar()->showMessage("Wait until the mission stops before opening its ephemeris file"); return; }
         const QFileInfo file(name); if (!file.isFile()) { statusBar()->showMessage("Ephemeris file is not available: "+name); return; }
         auto *viewer=new QWidget; viewer->setObjectName("ephemerisFileDetails"); auto *layout=new QVBoxLayout(viewer);
         auto *details=new QLabel(QString("Binary ephemeris file\nFormat: %1\nPath: %2\nSize: %3 bytes").arg(format,name).arg(file.size()),viewer); details->setObjectName("ephemerisFileInfo"); details->setWordWrap(true); details->setTextInteractionFlags(Qt::TextSelectableByMouse); layout->addWidget(details);
         auto *folder=new QPushButton("Open folder",viewer); folder->setObjectName("ephemerisOpenFolder"); layout->addWidget(folder); connect(folder,&QPushButton::clicked,viewer,[file] { QDesktopServices::openUrl(QUrl::fromLocalFile(file.absolutePath())); });
         auto *copy=new QPushButton("Copy path",viewer); copy->setObjectName("ephemerisCopyPath"); layout->addWidget(copy); connect(copy,&QPushButton::clicked,viewer,[name] { QApplication::clipboard()->setText(name); }); layout->addStretch();
         auto *child=workspace->addSubWindow(viewer); child->setAttribute(Qt::WA_DeleteOnClose); child->setWindowTitle(item->text(0)+" — "+format); child->resize(650,250); child->show();
      } else if (type=="report" || type=="ephemeris") {
         if (running) { statusBar()->showMessage("Wait until the mission stops before opening its report"); return; }
         QFile file(name);
         if (!file.open(QIODevice::ReadOnly)) { statusBar()->showMessage("Report is not available: " + file.errorString()); return; }
         auto *viewer = new ReportViewer(name,item->text(0));
         auto *child = workspace->addSubWindow(viewer);
         child->setAttribute(Qt::WA_DeleteOnClose);
         child->setWindowTitle(item->text(0) + " — " + name);
         viewer->setToolTip(name);
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
   editor = new ScriptEditor;
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
   auto *searchDialog=new FindReplaceDialog(editor,this);
   auto searchAction=[&](const QString &label,const QString &name,QKeySequence key,auto callback) {
      auto *action=edit->addAction(label); action->setObjectName(name); action->setShortcut(key);
      connect(action,&QAction::triggered,this,[this,document,callback] {
         if (document->isMinimized()) document->showNormal();
         workspace->setActiveSubWindow(document); callback();
      });
   };
   searchAction("&Find…","scriptFind",QKeySequence::Find,[searchDialog] { searchDialog->openSearch(); });
   searchAction("&Replace…","scriptReplace",QKeySequence::Replace,[searchDialog] { searchDialog->openSearch(); });
   searchAction("Find next","scriptFindNext",QKeySequence::FindNext,[searchDialog] { searchDialog->findNext(); });
   searchAction("Find previous","scriptFindPrevious",QKeySequence::FindPrevious,[searchDialog] { searchDialog->findNext(true); });

   edit->addSeparator();
   auto *create=edit->addAction("New &resource…");
   create->setObjectName("createResource"); editingActions.append(create);
   connect(create,&QAction::triggered,this,&MainWindow::showCreateResource);
   auto *convert=edit->addAction("Convert OpenFrames views for Qt");
   convert->setObjectName("convertOpenFramesViews"); editingActions.append(convert);
   connect(convert,&QAction::triggered,this,[this] { convertOpenFramesScript(); });
   resources->setContextMenuPolicy(Qt::CustomContextMenu);
   connect(resources,&QTreeWidget::customContextMenuRequested,this,[this,create](const QPoint &position) {
      auto *item=resources->itemAt(position);
      const QString name=item && !item->data(0,Qt::UserRole).toString().isEmpty() ? item->text(0) : QString();
      const QString snapshot=builtScript;
      QMenu menu(this); menu.addAction(create);
      auto *remove=menu.addAction("Delete resource…");
      remove->setEnabled(ready && !running && modelValid && !name.isEmpty());
      if (menu.exec(resources->viewport()->mapToGlobal(position))==remove &&
          QMessageBox::question(this,"Delete resource","Delete "+name+" from this mission?",
             QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel)==QMessageBox::Yes) {
         const auto error=deleteResource(name,snapshot);
         if (!error.isEmpty()) QMessageBox::warning(this,"Cannot delete resource",error);
      }
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
   auto *capabilities=help->addAction("Available engine types…");
   capabilities->setObjectName("engineCapabilities");
   connect(capabilities,&QAction::triggered,this,[this] {
      QDialog dialog(this); dialog.setWindowTitle("Available engine types");
      auto *layout=new QVBoxLayout(&dialog);
      auto *description=new QLabel("These types are registered by this runtime and its loaded plugins. They are available to scripts; some properties still require the script editor. OpenFrames viewer definitions can be converted from the Edit menu. wx-only plugin panels are not loaded by Qt.",&dialog);
      description->setWordWrap(true); layout->addWidget(description);
      auto *types=new QPlainTextEdit(availableEngineTypes().join('\n'),&dialog); types->setReadOnly(true); layout->addWidget(types);
      auto *buttons=new QDialogButtonBox(QDialogButtonBox::Close,&dialog); layout->addWidget(buttons);
      connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
      dialog.resize(650,550); dialog.exec();
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
      },nullptr,snapshot);
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
   ListenerManagerInterface::SetListenerManager(nullptr);
   PlotInterface::SetPlotReceiver(nullptr);
   MessageInterface::SetMessageReceiver(nullptr);
}
bool MainWindow::initialize(const QString &startup)
{
   const auto compatibilityError=qtStartupCompatibilityError(startup);
   if (!compatibilityError.isEmpty()) {
      messages->appendPlainText(compatibilityError);
      statusBar()->showMessage("Runtime initialization failed");
      return false;
   }
   receiver = std::make_unique<QtMessageReceiver>();
   receiver->SetMessageCallback(this, [this](const QString &text) {
      messages->moveCursor(QTextCursor::End); messages->insertPlainText(text);
   });
   MessageInterface::SetMessageReceiver(receiver.get());
   PlotInterface::SetPlotReceiver(plots.get());
   ListenerManagerInterface::SetListenerManager(solverListeners.get());
   try {
      ready = Moderator::Instance()->Initialize(startup.toStdString(), true);
      if (ready) {
         QStringList keywords;
         for (const auto &word:Moderator::Instance()->GetListOfAllFactoryItems()) keywords.append(QString::fromStdString(word));
         static_cast<ScriptEditor *>(editor)->setKeywords(keywords);
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
   plots->cameraSettings.clear();
   plots->clear(true);
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
   plots->clear(true);
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
QStringList MainWindow::availableEngineTypes() const
{
   QStringList result;
   if (ready) for (const auto &name:Moderator::Instance()->GetListOfAllFactoryItems()) result.append(QString::fromStdString(name));
   result.removeDuplicates(); result.sort(); return result;
}
bool MainWindow::convertOpenFramesScript()
{
   if (!ready || running) return false;
   for (auto *child:workspace->subWindowList())
      if (auto *panel=dynamic_cast<EditablePanel *>(child->widget());panel && panel->hasChanges()) {
         statusBar()->showMessage("Apply or discard the open panel changes before converting views"); return false;
      }
   const auto converted=convertOpenFramesViews(editor->toPlainText());
   if (!converted.error.isEmpty()) { messages->appendPlainText(converted.error); return false; }
   if (converted.script==editor->toPlainText()) { statusBar()->showMessage("No OpenFrames viewer definitions to convert"); return true; }
   const auto error=applyModelScript(converted.script);
   if (!error.isEmpty()) { messages->appendPlainText("View conversion rejected: "+error); return false; }
   messages->appendPlainText(QString("Converted %1 OpenFrames viewer(s) for Qt. Review the visual differences listed in the script before saving.").arg(converted.plots));
   for (const auto &note:converted.notes) messages->appendPlainText(note);
   statusBar()->showMessage("Views converted — review the script; Undo restores the original");
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
   // File > Open and command-line loading both build through here, as does Run.
   // Offer conversion before the engine rejects unavailable OpenFrames types.
   const auto conversion=convertOpenFramesViews(editor->toPlainText());
   if (!conversion.error.isEmpty()) {
      QMessageBox::warning(this,"OpenFrames views need manual conversion",
         "This script uses OpenFrames features that cannot be converted automatically.\n\n"+conversion.error);
      statusBar()->showMessage("Build stopped — OpenFrames views need manual conversion");
      return false;
   }
   if (conversion.script!=editor->toPlainText()) {
      QMessageBox prompt(QMessageBox::Question,"Convert OpenFrames views?",
         "This script uses OpenFrames views, which are unavailable in the Qt interface. "
         "Convert them to Qt views and continue?\n\n"
         "Mission calculations will be preserved. Some visual settings differ. "
         "Changes stay unsaved, and Undo restores the original script.",
         QMessageBox::Yes|QMessageBox::No,this);
      prompt.setObjectName("openFramesConversionPrompt");
      prompt.button(QMessageBox::Yes)->setText("Convert views");
      prompt.button(QMessageBox::No)->setText("Keep original");
      prompt.setDefaultButton(QMessageBox::Yes);
      prompt.setEscapeButton(QMessageBox::No);
      if (!conversion.notes.isEmpty()) prompt.setDetailedText(conversion.notes.join("\n"));
      if (prompt.exec()!=QMessageBox::Yes) {
         statusBar()->showMessage("Build canceled — original OpenFrames script kept");
         return false;
      }
      if (!convertOpenFramesScript()) {
         QMessageBox::warning(this,"Conversion could not be completed",
            "The converted script could not be built. The original script is unchanged. See the Message Window for details.");
         return false;
      }
      return true; // Conversion already validated and rebuilt the mission.
   }
   bool success = false;
   plots->clear();
   try {
      const auto cameras=qtCameraSettings(editor->toPlainText());
      std::istringstream stream(editor->toPlainText().toStdString());
      success = Moderator::Instance()->InterpretScript(&stream, true);
      if (success) { QtPlotReceiver::validateCameraReferences(cameras); plots->cameraSettings=cameras; }
   } catch (BaseException &error) { messages->appendPlainText(QString::fromStdString(error.GetFullMessage())); }
   catch (const std::exception &error) { success=false; messages->appendPlainText(QString::fromUtf8(error.what())); }
   catch (...) { messages->appendPlainText("Unexpected error while building the script."); }
   modelValid = success;
   if (success) builtScript = editor->toPlainText();
   refreshTrees(); statusBar()->showMessage(success ? "Build succeeded" : "Build failed — see Message Window"); return success;
}
void MainWindow::refreshTrees()
{
   resources->clear(); mission->clear(); output->clear();
   reportFiles.clear();
   ephemerisFiles.clear();
   auto *root = new QTreeWidgetItem(resources, {"Resources"});
   const std::pair<const char *, UnsignedInt> groups[] = {
      {"Spacecraft", Gmat::SPACECRAFT}, {"Hardware", Gmat::HARDWARE},
      {"Formations", Gmat::FORMATION}, {"Ground Stations", Gmat::GROUND_STATION},
      {"Propagators", Gmat::PROP_SETUP}, {"Burns", Gmat::BURN},
      {"Force Models", Gmat::ODE_MODEL},
      {"Coordinate Systems", Gmat::COORDINATE_SYSTEM}, {"Solvers", Gmat::SOLVER},
      {"Output", Gmat::SUBSCRIBER}, {"Variables, Arrays, Strings", Gmat::PARAMETER},
      {"Functions", Gmat::FUNCTION}, {"Event Locators",Gmat::EVENT_LOCATOR}};
   for (const auto &group : groups) {
      auto *category = new QTreeWidgetItem(root, {group.first});
      for (const auto &name : Moderator::Instance()->GetListOfObjects(group.second)) {
         if (group.second == Gmat::SUBSCRIBER) {
            auto *object = Moderator::Instance()->GetConfiguredObject(name);
            if (object && object->IsOfType(Gmat::REPORT_FILE))
               reportFiles.insert(QString::fromStdString(name), QString::fromStdString(object->GetStringParameter("FullPathFileName")));
            if (auto *ephemeris=dynamic_cast<EphemerisFile *>(object)) {
               const auto format=ephemeris->GetStringParameter("FileFormat");
               const auto resolved=ephemeris->GetStringParameter("FullPathFileName");
               // Engine writing retains custom extensions except for SPK.
               const auto path=format!="SPK" && !QFileInfo(QString::fromStdString(resolved)).suffix().isEmpty() ? resolved : ephemeris->GetProperFileName(resolved,format,false);
               ephemerisFiles.insert(QString::fromStdString(name),{QString::fromStdString(path),QString::fromStdString(format)});
            }
         }
         if (group.second==Gmat::EVENT_LOCATOR || group.second==Gmat::SOLVER) {
            auto *object=Moderator::Instance()->GetConfiguredObject(name);
            try {
               const auto filename=QString::fromStdString(object->GetStringParameter(group.second==Gmat::SOLVER ? "ReportFile" : "Filename"));
               if (!filename.isEmpty()) reportFiles.insert(QString::fromStdString(name),QFileInfo(filename).isAbsolute() ? filename :
                  QDir(QString::fromStdString(FileManager::Instance()->GetFullPathname("OUTPUT_PATH"))).filePath(filename));
            } catch (BaseException &) {} // Plugins need not provide a report file.
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
   auto *ephemerides=new QTreeWidgetItem(output,{"Ephemeris files"});
   for (auto it=ephemerisFiles.cbegin();it!=ephemerisFiles.cend();++it) {
      auto *item=new QTreeWidgetItem(ephemerides,{it.key()}); item->setData(0,Qt::UserRole,it.value().first); item->setData(0,Qt::UserRole+1,"ephemeris"); item->setData(0,Qt::UserRole+2,it.value().second); item->setToolTip(0,it.value().first);
   }
   ephemerides->setExpanded(true);
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
   solverListeners->missionStarted();
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
   solverListeners->missionFinished(result==RunResult::Stopped,result==RunResult::Failed);
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
   if (changes.contains("@ArrayExpressions") && !object->IsOfType("Array")) return "Cell expressions require an Array.";
   if (changes.contains("@ArrayExpressions") && changes.size()==1) {
      try {
         return applyModelScript(setArrayExpressions(expectedScript,name,changes["@ArrayExpressions"],
            object->GetIntegerParameter("NumRows"),object->GetIntegerParameter("NumCols")));
      } catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
   }
   QString candidate;
   try {
      std::unique_ptr<GmatBase> proposed(object->Clone());
      if (!proposed) return "This resource cannot be edited.";
      const bool pairedMixture=object->IsOfType("Thruster") && changes.contains("Tank") && changes.contains("MixRatio");
      const QString mixture=changes.value("MixRatio");
      const auto orbitChanges=applySpacecraftOrbitProperties(*proposed,changes);
      const auto attitudeChanges=applyAttitudeProperties(*proposed,changes);
      const auto atmosphereChanges=applyAtmosphereProperties(*proposed,changes);
      const auto stationChanges=applyGroundStationLocation(*proposed,changes);
      const auto eventChanges=applyEventLocatorProperties(*proposed,changes);
      const auto viewChanges=applyOrbitViewProperties(*proposed,changes);
      const auto burnChanges=applyBurnProperties(*proposed,changes);
      const auto ephemerisChanges=applyEphemerisProperties(*proposed,changes);
      for (auto it = changes.cbegin(); it != changes.cend(); ++it) {
         if (orbitChanges.contains(it.key()) || attitudeChanges.contains(it.key()) || atmosphereChanges.contains(it.key()) || stationChanges.contains(it.key()) || eventChanges.contains(it.key()) || viewChanges.contains(it.key()) || burnChanges.contains(it.key()) || ephemerisChanges.contains(it.key()) || it.key()=="@ArrayExpressions" || (pairedMixture && it.key()=="MixRatio")) continue;
         if (isResourceList(*proposed,it.key())) continue;
         try { setResourceProperty(*proposed, it.key(), it.value()); }
         catch (BaseException &error) { return it.key() + ": " + QString::fromStdString(error.GetFullMessage()); }
         catch (const std::exception &error) { return it.key() + ": " + QString::fromUtf8(error.what()); }
      }
      validateResourceProperties(*proposed);
      if (proposed->GetTypeName()=="XYPlot" && proposed->GetBooleanParameter("ShowPlot")) {
         const bool emptyY=changes.contains("YVariables") ? splitResourceReferences(changes.value("YVariables")).isEmpty() : proposed->GetStringArrayParameter("YVariables").empty();
         if (emptyY) return "Select at least one Y parameter, or turn off Show plot.";
      }
      if ((proposed->GetTypeName()=="GroundTrackPlot" || proposed->GetTypeName()=="GroundTrack") && proposed->GetBooleanParameter("ShowPlot")) {
         const bool empty=changes.contains("Add") ? splitResourceReferences(changes.value("Add")).isEmpty() : proposed->GetStringArrayParameter("Add").empty();
         if (empty) return "Select an object, or turn off Show plot.";
      }
      if (!proposed->Validate()) return "The resource rejected these settings.";
      candidate = QString::fromStdString(moderator->GetScript(Gmat::SCRIPTING));
      auto serialize=[](GmatBase &resource) {
         if (!resource.IsOfType(Gmat::PROP_SETUP)) return QString::fromStdString(resource.GetGeneratingString(Gmat::SCRIPTING));
         // The full script writes force models in their own section. Match that
         // convention without changing the configured object's output flags.
         std::unique_ptr<GmatBase> copy(resource.Clone());
         copy->TakeAction("ExcludeODEModel");
         return QString::fromStdString(copy->GetGeneratingString(Gmat::SCRIPTING));
      };
      const auto oldBlock = serialize(*object);
      auto newBlock = proposed->IsOfType("Spacecraft") ? spacecraftOrbitScript(*proposed) : proposed->IsOfType("OrbitView") ? orbitViewScript(*proposed) : (proposed->IsOfType("ImpulsiveBurn") || proposed->IsOfType("FiniteBurn")) ? burnResourceScript(*proposed) : serialize(*proposed);
      for (auto it=changes.cbegin();it!=changes.cend();++it)
         if (it.key()!="@ArrayExpressions" && !orbitChanges.contains(it.key()) && !atmosphereChanges.contains(it.key()) && !eventChanges.contains(it.key()) && !viewChanges.contains(it.key()) && !burnChanges.contains(it.key()) && isResourceList(*proposed,it.key())) newBlock=replaceResourceList(*proposed,newBlock,it.key(),it.value(),pairedMixture && it.key()=="Tank" ? &mixture : nullptr);
      if (oldBlock.isEmpty() || candidate.count(oldBlock) != 1)
         return "This resource requires a specialized editor. Use its script settings for now.";
      candidate.replace(candidate.indexOf(oldBlock), oldBlock.size(), newBlock);
      // Unrelated resource edits also reconstruct the mission. Avoid repeated
      // representation drift or omitted anomaly elements in other spacecraft.
      candidate=preserveSpacecraftOrbits(candidate,object);
      // Resource edits must not normalize/rewrite the existing mission commands,
      // including cell formulas, labels and user comments.
      const QRegularExpression missionStart("^[ \t]*BeginMissionSequence\\b",QRegularExpression::MultilineOption);
      const auto originalMission=missionStart.match(expectedScript),rebuiltMission=missionStart.match(candidate);
      if (originalMission.hasMatch() && rebuiltMission.hasMatch())
         candidate=candidate.left(rebuiltMission.capturedStart())+expectedScript.mid(originalMission.capturedStart());
      if (object->IsOfType("Array") && (changes.contains("@ArrayExpressions") || changes.contains("RmatValue")))
         candidate=setArrayExpressions(candidate,name,changes.value("@ArrayExpressions",arrayExpressions(expectedScript,name)),
            proposed->GetIntegerParameter("NumRows"),proposed->GetIntegerParameter("NumCols"));
      // Explicit camera controls take precedence over imported primary-camera
      // behavior. Named secondary views remain independent.
      const QStringList cameraFields={"ViewPointReference","ViewPointVector","ViewScaleFactor","ViewDirection","ViewUpAxis","ViewUpCoordinateSystem"};
      const bool cameraEdited=std::any_of(cameraFields.cbegin(),cameraFields.cend(),[&](const QString &field) { return changes.contains(field); });
      if (object->IsOfType("OrbitView") && cameraEdited) {
         const auto settings=qtCameraSettings(expectedScript);
         if (settings.contains(name)) {
            auto setting=settings.value(name);
            if (!setting.automaticTrajectory.isEmpty() || !setting.automaticBody.isEmpty()) {
               if (!setting.automaticBody.isEmpty()) setting.bodyRelative=false;
               setting.automaticTrajectory.clear(); setting.automaticBody.clear(); setting.centerOffset.reset(); setting.lookAtRotation=false;
            }
            if (changes.contains("ViewUpAxis") || changes.contains("ViewUpCoordinateSystem")) setting.up.reset();
            if (changes.contains("ViewDirection")) { setting.centerOffset.reset(); setting.lookAtRotation=false; }
            candidate=setQtCameraSetting(candidate,name,setting);
         }
      }
   } catch (BaseException &error) { return QString::fromStdString(error.GetFullMessage()); }
   catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
   return applyModelScript(candidate);
}

QString MainWindow::createResource(const QString &type,const QString &name,const QString &expectedScript,int rows,int columns)
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
   if (type=="Array" && (rows<1 || columns<1 || rows>100 || columns>100))
      return "Choose array dimensions from 1 to 100. Larger arrays can be created in the script editor.";
   const auto dimensions=type=="Array" ? QString("[%1,%2]").arg(rows).arg(columns) : QString();
   return applyModelScript("Create "+type+" "+name+dimensions+";\n"+builtScript);
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
   layout->addRow("Type",type); layout->addRow("Name",name);
   auto *rows=new QSpinBox(&dialog); rows->setObjectName("arrayRows"); rows->setRange(1,100);
   auto *columns=new QSpinBox(&dialog); columns->setObjectName("arrayColumns"); columns->setRange(1,100);
   layout->addRow("Rows",rows); layout->addRow("Columns",columns);
   const auto dimensions=[=] {
      const bool array=type->currentText()=="Array";
      layout->setRowVisible(rows,array); layout->setRowVisible(columns,array);
   };
   connect(type,&QComboBox::currentTextChanged,&dialog,dimensions); dimensions();
   layout->addRow(status);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);
   buttons->button(QDialogButtonBox::Ok)->setText("Create"); layout->addRow(buttons);
   connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,&dialog,[&] {
      const auto error=createResource(type->currentText(),name->text().trimmed(),snapshot,rows->value(),columns->value());
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

QString MainWindow::deleteResource(const QString &name,const QString &expectedScript)
{
   if (!ready || running || !modelValid || expectedScript!=builtScript || editor->toPlainText()!=builtScript)
      return "Build the current script before deleting a resource.";
   for (auto *child:workspace->subWindowList()) {
      auto *panel=dynamic_cast<EditablePanel *>(child->widget());
      if (child->property("resourceName").toString()==name && panel && panel->hasChanges())
         return "Apply or discard this resource's panel changes before deleting it.";
   }
   auto *moderator=Moderator::Instance();
   auto *object=moderator->GetConfiguredObject(name.toStdString());
   if (!object) return "This resource no longer exists.";
   QString candidate;
   try {
      const auto canonical=QString::fromStdString(moderator->GetScript(Gmat::SCRIPTING));
      const QRegularExpression declaration("^[ \\t]*Create[ \\t]+[A-Za-z0-9_]+[ \\t]+[^;\\n]*\\b"+
         QRegularExpression::escape(name)+"\\b[^;\\n]*;",QRegularExpression::MultilineOption);
      if (!QRegularExpression("^[A-Za-z][A-Za-z0-9_]*$").match(name).hasMatch() || !declaration.match(canonical).hasMatch())
         return "Built-in resources and generated parameters cannot be deleted here.";
      if (!moderator->RemoveObject(object->GetType(),name.toStdString(),true))
         return "This resource is used by another resource or mission command. See Message Window for details.";
      candidate=preserveSpacecraftOrbits(QString::fromStdString(moderator->GetScript(Gmat::SCRIPTING)));
   } catch (BaseException &error) {
      const auto detail=QString::fromStdString(error.GetFullMessage());
      return restoreBuiltModel() ? detail : detail+" Restoration failed; rebuild the script.";
   } catch (const std::exception &error) {
      const auto detail=QString::fromUtf8(error.what());
      return restoreBuiltModel() ? detail : detail+" Restoration failed; rebuild the script.";
   }
   const auto error=applyModelScript(candidate);
   if (error.isEmpty()) {
      for (auto *child:workspace->subWindowList()) if (child->property("resourceName").toString()==name) child->close();
      statusBar()->showMessage("Resource deleted — Undo restores it; save to keep changes");
   }
   return error;
}

bool MainWindow::restoreBuiltModel()
{
   plots->clear(); modelValid=false;
   try {
      std::istringstream previous(builtScript.toStdString());
      modelValid=Moderator::Instance()->InterpretScript(&previous,true);
   } catch (...) { }
   refreshTrees(); return modelValid;
}

QString MainWindow::applyModelScript(const QString &requested)
{
   QString candidate;
   QMap<QString,QtCameraSetting> cameras;
   try {
      candidate=retainQtCameraSettings(editor->toPlainText(),requested);
      cameras=qtCameraSettings(candidate);
   } catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
   auto *moderator = Moderator::Instance();
   QString error;
   plots->clear();
   try {
      std::istringstream stream(candidate.toStdString());
      if (!moderator->InterpretScript(&stream, true)) error = "The mission rejected these changes. See Message Window.";
      else QtPlotReceiver::validateCameraReferences(cameras);
   } catch (BaseException &exception) { error = QString::fromStdString(exception.GetFullMessage()); }
   catch (const std::exception &exception) { error = QString::fromUtf8(exception.what()); }
   if (!error.isEmpty()) {
      // Interpretation reconstructs the model. Restore the entire prior model,
      // not just the last field, without touching the editor or its undo stack.
      restoreBuiltModel();
      if (!modelValid) error += " Restoration failed; rebuild the script before continuing.";
      return error;
   }
   auto cursor = editor->textCursor();
   cursor.beginEditBlock();
   cursor.select(QTextCursor::Document);
   cursor.insertText(candidate);
   cursor.endEditBlock();
   builtScript = candidate;
   plots->cameraSettings=cameras;
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
   auto firstType=[&](UnsignedInt category,const char *type,const char *fallback) {
      for (const auto &name:Moderator::Instance()->GetListOfObjects(category)) {
         auto *object=Moderator::Instance()->GetConfiguredObject(name);
         if (object && object->IsOfType(type)) return QString::fromStdString(name);
      }
      return QString::fromLatin1(fallback);
   };
   const auto solver=firstType(Gmat::SOLVER,"DifferentialCorrector","SolverName");
   const auto optimizer=firstType(Gmat::SOLVER,"Optimizer","OptimizerName");
   const auto finite=firstType(Gmat::BURN,"FiniteBurn","FiniteBurnName");
   const auto report=firstType(Gmat::SUBSCRIBER,"ReportFile","ReportName");
   QMap<QString,QString> templates={
      {"Propagate",QString("Propagate %1(%2) {%2.ElapsedSecs = 600};").arg(prop,sat)},
      {"Maneuver",QString("Maneuver %1(%2);").arg(burn,sat)},
      {"BeginFiniteBurn",QString("BeginFiniteBurn %1(%2);").arg(finite,sat)},
      {"EndFiniteBurn",QString("EndFiniteBurn %1(%2);").arg(finite,sat)},
      {"Report",QString("Report %1 %2.ElapsedSecs %2.X %2.Y %2.Z;").arg(report,sat)},
      {"Assignment","VariableName = 1;"},
      {"If","If VariableName > 0;\n   % Insert commands here.\nElse;\n   % Insert alternate commands here.\nEndIf;"},
      {"While","While VariableName < 10;\n   VariableName = VariableName + 1;\nEndWhile;"},
      {"For","For VariableName = 1:1:10;\n   % Insert commands here.\nEndFor;"},
      {"Target",QString("Target %1 {SolveMode = Solve, ExitMode = DiscardAndContinue, ShowProgressWindow = true};\n   % Add Vary, mission commands, and Achieve here.\nEndTarget;").arg(solver)},
      {"Vary",QString("Vary %1(%2.Element1 = 0.1, {Perturbation = 0.0001, Lower = -10, Upper = 10, MaxStep = 0.2});").arg(solver,burn)},
      {"Achieve",QString("Achieve %1(%2.Earth.RMAG = 8000, {Tolerance = 0.1});").arg(solver,sat)},
      {"Optimize",QString("Optimize %1 {SolveMode = Solve, ExitMode = DiscardAndContinue, ShowProgressWindow = true};\n   % Add Vary, mission commands, constraints and Minimize here.\nEndOptimize;").arg(optimizer)},
      {"Minimize",QString("Minimize %1(ObjectiveVariable);").arg(optimizer)},
      {"NonlinearConstraint",QString("NonlinearConstraint %1(ConstraintVariable <= 1);").arg(optimizer)},
      {"FindEvents",QString("FindEvents %1 {Append = false};").arg(firstType(Gmat::EVENT_LOCATOR,"EventLocator","LocatorName"))},
      {"Call function",QString("[OutputVariable] = %1(InputVariable);").arg(firstType(Gmat::FUNCTION,"GmatFunction","FunctionName"))},
      {"Stop","Stop;"}, {"Script event","BeginScript;\n   % Insert commands here.\nEndScript;"}};
   if (availableEngineTypes().contains("Save")) templates.insert("Save",QString("Save %1;").arg(sat));
   QStringList propagationChoices,spacecraftChoices,formationChoices;
   for (const auto &value:propagators) propagationChoices.append(QString::fromStdString(value));
   for (const auto &value:spacecraft) spacecraftChoices.append(QString::fromStdString(value));
   for (const auto &value:Moderator::Instance()->GetListOfObjects(Gmat::FORMATION)) formationChoices.append(QString::fromStdString(value));
   auto *panel=new CommandEditor(statement,operation!=MissionEdit::Replace,templates,
      [this,snapshot,index,operation](const QString &replacement) {
         return applyMissionChange(snapshot,index,operation,replacement);
      },propagationChoices,spacecraftChoices,nullptr,formationChoices);
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

QString MainWindow::savePlotProjection(const QString &name,bool perspective,double fov)
{
   if (!ready || running) return "Stop the mission before saving projection settings.";
   if (!modelValid || editor->toPlainText()!=builtScript) return "Build the current script before saving projection settings.";
   for (auto *child:workspace->subWindowList())
      if (auto *panel=dynamic_cast<EditablePanel *>(child->widget());panel && panel->hasChanges())
         return "Apply or discard panel changes before saving projection settings.";
   auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString());
   if (!object || !object->IsOfType("OrbitView")) return "This OrbitView no longer exists in the current mission.";
   QString candidate;
   QtCameraSetting setting;
   try {
      setting=qtCameraSettings(builtScript).value(name);
      const auto model=plots->model(name);
      if (model && model->selectedCamera>0 && model->selectedCamera<=setting.views.size()) {
         auto &view=setting.views[model->selectedCamera-1]; view.perspective=perspective; view.fieldOfView=fov;
      } else { setting.perspective=perspective; setting.fieldOfView=fov; }
      candidate=setQtCameraSetting(builtScript,name,setting);
   }
   catch (const std::exception &error) { return QString::fromUtf8(error.what()); }
   if (candidate==builtScript) return {};
   // This changes a validated comment only. Do not rebuild/delete the viewer
   // from within its own action callback or reconstruct scientific objects.
   auto cursor=editor->textCursor(); cursor.beginEditBlock(); cursor.select(QTextCursor::Document);
   cursor.insertText(candidate); cursor.endEditBlock();
   builtScript=candidate; plots->cameraSettings[name]=setting;
   refreshTrees();
   statusBar()->showMessage("Projection added to script — save to keep it; Undo restores the previous settings");
   return {};
}
