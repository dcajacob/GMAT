#include "MainWindow.hpp"
#include "PathSettings.hpp"
#include "PathSettingsDialog.hpp"
#include "ResourceEditor.hpp"
#include "TestSettings.hpp"
#include "FileManager.hpp"
#include "GmatGlobal.hpp"
#include "Moderator.hpp"
#include "MessageInterface.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QTreeWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QStatusBar>
#include <QProcess>
#include <QElapsedTimer>
#include <QThread>
#include <QTabWidget>
#include <iostream>
#include <stdexcept>
static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),qPrintable("Cannot read "+path)); return file.readAll(); }
static void write(const QString &path,const QByteArray &text) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(text)==text.size(),"Fixture write failed"); }
template<class Open,class Inspect> static void modal(QWidget &owner,Open open,Inspect inspect)
{
   std::exception_ptr failure; bool inspected=false;
   QTimer timer; timer.setSingleShot(true); QObject::connect(&timer,&QTimer::timeout,&owner,[&] { inspected=true; auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
      try { require(dialog,"Expected modal missing"); inspect(dialog); } catch (...) { failure=std::current_exception(); }
      if (dialog && dialog->isVisible()) dialog->reject();
   }); timer.start(0); open(); require(inspected,qPrintable("Modal not inspected: "+owner.objectName()+" "+owner.property("pathTestOperation").toString()+" "+(owner.findChild<QLabel *>("pathStatus") ? owner.findChild<QLabel *>("pathStatus")->text() : QString())+" "+(owner.findChild<QPlainTextEdit *>("messageWindow") ? owner.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().right(2000) : QString()))); if (failure) std::rethrow_exception(failure);
}
static void choose(QDialog *owner,const char *button,const QString &path,bool cancel=false)
{
   owner->setProperty("pathTestOperation",button);
   modal(*owner,[&] { owner->findChild<QPushButton *>(button)->click(); },[&](QDialog *dialog) {
      auto *chooser=qobject_cast<QFileDialog *>(dialog); require(chooser,"Path chooser missing");
      chooser->setOption(QFileDialog::DontConfirmOverwrite,true);
      if (!cancel) { chooser->setDirectory(QFileInfo(path).absolutePath()); chooser->selectFile(path); QMetaObject::invokeMethod(chooser,"accept",Qt::DirectConnection); }
   });
}
static QStringList functions()
{
   QStringList paths; for (const auto &entry:FileManager::Instance()->GetAllGmatFunctionPaths()) paths.append(QDir::cleanPath(QString::fromStdString(entry))); return paths;
}
static QString output() { return QDir::cleanPath(QString::fromStdString(FileManager::Instance()->GetAbsPathname("OUTPUT_PATH"))); }
static void apply(QDialog *dialog) { dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); QApplication::processEvents(); }
static void capture(QWidget *widget,const QString &path)
{
   if (path.isEmpty()) return; QElapsedTimer elapsed; elapsed.start();
   while (elapsed.elapsed()<300) { QApplication::processEvents(); QThread::msleep(5); }
   require(widget->grab().save(path),"Path screenshot failed");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtPaths");
   try {
      TestSettings settings; QTemporaryDir files; require(argc>1 && files.isValid(),"Fixture setup failed");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto screenshots=argc>2 ? QString::fromLocal8Bit(argv[2]) : QString();
      MainWindow window; window.show(); require(window.initialize(startup),"Initialize failed"); auto *fm=FileManager::Instance();
      const auto original=fm->CaptureState(); struct Restore { std::shared_ptr<const FileManager::State> state; ~Restore() { FileManager::Instance()->RestoreState(*state); } } restore{original};
      const auto first=files.filePath("first funcs.1"),second=files.filePath("second funcs"),out1=files.filePath("output one"),out2=files.filePath("output ü"),missionDir=files.filePath("missions");
      for (const auto &path:QStringList{first,second,out1,out2,missionDir}) require(QDir().mkpath(path),"Directory fixture failed");
      fm->SetAbsPathname("OUTPUT_PATH",out1.toStdString()); MessageInterface::SetLogPath(out1.toStdString(),true);
      write(first+"/Priority.gmf","function [y] = Priority(x)\nCreate Variable y;\nBeginMissionSequence;\ny = x * 2;\n");
      write(second+"/Priority.gmf","function [y] = Priority(x)\nCreate Variable y;\nBeginMissionSequence;\ny = x * 3;\n");
      const auto missionFile=missionDir+"/paths ü.script",savedStartup=files.filePath("startup ü.txt"),explicitReport=files.filePath("explicit.txt");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); editor->setPlainText("Create Variable x;\nx = 1;\nBeginMissionSequence;\nx = 5;\n"); require(window.saveScriptTo(missionFile) && window.buildScript(),"Initial mission failed");
      auto *action=window.findChild<QAction *>("setPaths"); require(action && action->isEnabled(),"Set paths action missing");
      const auto baselineFunctions=functions(); const auto baselineOutput=output(),baselineSource=editor->toPlainText();
      auto *tree=window.findChild<QTreeWidget *>("Resources"); QTreeWidgetItemIterator item(tree);
      while (*item && (*item)->text(0)!="x") ++item; require(*item,"Variable resource missing"); tree->itemDoubleClicked(*item,0); QApplication::processEvents();
      auto *workspace=window.findChild<QMdiArea *>("workspace"); auto *panel=dynamic_cast<ResourceEditor *>(workspace->activeSubWindow()->widget()); require(panel,"Resource panel missing"); panel->findChild<QLineEdit *>("parameterValue")->setText("2"); action->trigger(); require(window.statusBar()->currentMessage().contains("Apply or discard"),"Pending panel did not block path changes"); panel->discardChanges(); workspace->activeSubWindow()->close(); QApplication::processEvents();
      modal(window,[&] { action->trigger(); },[&](QDialog *dialog) {
         require(dialog->objectName()=="pathSettingsDialog" && dialog->findChild<QLineEdit *>("pathStartupFile")->text()==startup,"Startup identity wrong");
         auto *list=dialog->findChild<QListWidget *>("pathFunctions"); list->clear(); list->addItems({first,second}); dialog->findChild<QLineEdit *>("pathOutput")->setText(out2);
         choose(dialog,"pathOutputBrowse",out1,true); require(dialog->findChild<QLineEdit *>("pathOutput")->text()==out2,"Directory Cancel lost pending value");
      }); require(functions()==baselineFunctions && output()==baselineOutput && editor->toPlainText()==baselineSource,"Path Cancel changed session");
      modal(window,[&] { action->trigger(); },[&](QDialog *dialog) {
         auto *list=dialog->findChild<QListWidget *>("pathFunctions"); list->clear(); auto *entry=dialog->findChild<QLineEdit *>("pathFunctionEntry");
         entry->setText(files.filePath("missing")); dialog->findChild<QPushButton *>("pathFunctionAdd")->click(); require(list->count()==0,"Missing function directory added");
         choose(dialog,"pathFunctionBrowse",second); dialog->findChild<QPushButton *>("pathFunctionAdd")->click(); entry->setText(first); dialog->findChild<QPushButton *>("pathFunctionAdd")->click(); require(list->count()==2 && list->item(0)->text()==first,"Function Add order incorrect");
         entry->setText(first+"/"); dialog->findChild<QPushButton *>("pathFunctionAdd")->click(); require(list->count()==2,"Duplicate function directory added");
         list->setCurrentRow(0); dialog->findChild<QPushButton *>("pathFunctionDown")->click(); require(list->item(1)->text()==first,"Down failed"); dialog->findChild<QPushButton *>("pathFunctionUp")->click();
         entry->setText(second); dialog->findChild<QPushButton *>("pathFunctionReplace")->click(); require(list->item(0)->text()==first,"Duplicate Replace changed entry");
         entry->setText(missionDir); dialog->findChild<QPushButton *>("pathFunctionReplace")->click(); require(list->item(0)->text()==missionDir,"Replace failed"); dialog->findChild<QPushButton *>("pathFunctionRemove")->click(); entry->setText(first); dialog->findChild<QPushButton *>("pathFunctionAdd")->click();
         dialog->findChild<QLineEdit *>("pathOutput")->setText(files.filePath("missing output")); apply(dialog); require(output()==baselineOutput && functions()==baselineFunctions && dialog->findChild<QLabel *>("pathStatus")->text().contains("unavailable"),"Invalid Apply did not roll back");
         dialog->findChild<QLineEdit *>("pathOutput")->setText(out1); capture(dialog,screenshots.isEmpty() ? QString() : screenshots+".functions.png"); apply(dialog); require(dialog->isVisible() && functions()==QStringList{first,second},"Path Apply failed or closed dialog");
      }); require(editor->toPlainText()==baselineSource && !editor->document()->isModified(),"Paths changed mission source/dirty state");
      const QString source="Create GmatFunction Priority;\nCreate Variable x y;\nCreate ReportFile Relative Explicit;\nRelative.Filename = 'path-report.txt';\nRelative.WriteHeaders = false;\nExplicit.Filename = '"+explicitReport+"';\nExplicit.WriteHeaders = false;\nBeginMissionSequence;\nx = 5;\n[y] = Priority(x);\nReport Relative x y;\nReport Explicit x y;\n";
      editor->setPlainText(source); require(window.saveScriptTo(missionFile) && window.runMission()==MainWindow::RunResult::Completed,qPrintable("First priority function failed: "+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText())); require(Moderator::Instance()->GetInternalObject("y")->GetRealParameter("Value")==10 && QFileInfo::exists(out1+"/path-report.txt"),"First priority/output wrong");
      modal(window,[&] { action->trigger(); },[&](QDialog *dialog) {
         auto *list=dialog->findChild<QListWidget *>("pathFunctions"); auto found=list->findItems(first,Qt::MatchExactly); require(found.size()==1,"Current function path missing"); list->setCurrentItem(found.first()); dialog->findChild<QPushButton *>("pathFunctionDown")->click();
         choose(dialog,"pathOutputBrowse",out2); dialog->findChild<QTabWidget *>("pathTabs")->setCurrentIndex(1); capture(dialog,screenshots.isEmpty() ? QString() : screenshots+".output.png"); apply(dialog); require(output()==out2,"Output Browse/Apply failed");
      }); require(editor->toPlainText()==source && window.runMission()==MainWindow::RunResult::Completed && Moderator::Instance()->GetInternalObject("y")->GetRealParameter("Value")==15,"Reordered function result wrong");
      const auto report=read(out2+"/path-report.txt"); require(report==read(explicitReport),"Explicit report destination or calculations changed");
      auto *outputs=window.findChild<QTreeWidget *>("Output"); QTreeWidgetItemIterator reportItem(outputs);
      while (*reportItem && (*reportItem)->text(0)!="Relative") ++reportItem; require(*reportItem && (*reportItem)->data(0,Qt::UserRole).toString()==out2+"/path-report.txt","Output tree retained old report path"); outputs->itemDoubleClicked(*reportItem,0); QApplication::processEvents(); require(window.findChild<QPlainTextEdit *>("report:Relative")->toPlainText().toUtf8()==report,"Report viewer differs from relocated output");
      require(QString::fromStdString(MessageInterface::GetLogFileName()).startsWith(out2+"/"),"Log destination did not move with Output");
      require(window.saveScriptTo(missionFile) && window.loadScript(missionFile) && window.runMission()==MainWindow::RunResult::Completed && read(out2+"/path-report.txt")==report,"Mission reopen lost path behavior");
      QString scripted=source; scripted.replace("Create GmatFunction Priority;","Create GmatFunction Priority;\nPriority.FunctionPath = '"+second+"/Priority.gmf';"); editor->setPlainText(scripted); require(window.runMission()==MainWindow::RunResult::Completed && read(out2+"/path-report.txt")==report,"GUI and explicit function-path reports differ"); editor->setPlainText(source); require(window.saveScriptTo(missionFile) && window.buildScript(),"Reference restore failed");
      fm->AddFileType("QT_CUSTOM_FILE_ABS",files.filePath("custom alias Δ.dat").toStdString()); const auto alias=fm->GetAbsPathname("QT_CUSTOM_FILE_ABS"); const auto python=fm->GetAllPythonModulePaths();
      const auto currentFunctions=functions(); const auto currentOutput=output(); const auto currentIdentity=fm->GetFullStartupFilePath();
      auto *global=GmatGlobal::Instance(); const auto runMode=global->GetRunMode(); const bool batch=global->IsBatchMode(),testing=global->InTestingMode(),echo=global->EchoCommands();
      const auto malformed=files.filePath("broken.txt"); write(malformed,"RUN_MODE = EXIT_AFTER_RUN\nECHO_COMMANDS = TRUE\nOUTPUT_PATH = /does/not/exist\nBROKEN without equals\n");
      const auto invalidDirectories=files.filePath("invalid directories.txt"),wxStartup=files.filePath("wx startup.txt");
      const auto validStartup=capturePathSettings(startup).startupText.toUtf8();
      write(invalidDirectories,validStartup+"\nROOT_PATH = /gmat/nonexistent/root\n"); write(wxStartup,validStartup+"\nPLUGIN = ../plugins/libOpenFramesInterface\n");
      MessageInterface::ShowMessage("PATH_SENTINEL\n"); MessageInterface::SetLogPath(out2.toStdString(),true); const auto oldLog=read(out2+"/GmatLog.txt");
      modal(window,[&] { action->trigger(); },[&](QDialog *dialog) {
         choose(dialog,"pathReadStartup",malformed); require(dialog->findChild<QLabel *>("pathStatus")->text().contains("expecting '='"),"Malformed startup error missing");
         require(functions()==currentFunctions && output()==currentOutput && fm->GetAbsPathname("QT_CUSTOM_FILE_ABS")==alias && fm->GetAllPythonModulePaths()==python && fm->GetFullStartupFilePath()==currentIdentity && global->GetRunMode()==runMode && global->IsBatchMode()==batch && global->InTestingMode()==testing && global->EchoCommands()==echo && read(out2+"/GmatLog.txt")==oldLog,"Startup preview failed exact rollback or truncated log");
         choose(dialog,"pathReadStartup",invalidDirectories); require(dialog->findChild<QLabel *>("pathStatus")->text().contains("directories do not exist") && functions()==currentFunctions && output()==currentOutput && fm->GetAbsPathname("QT_CUSTOM_FILE_ABS")==alias,"Path-validation import failure did not restore session");
         choose(dialog,"pathReadStartup",wxStartup); require(dialog->findChild<QLabel *>("pathStatus")->text().contains("wxWidgets-only") && functions()==currentFunctions && fm->GetFullStartupFilePath()==currentIdentity,"wx startup compatibility rejection altered session");
         choose(dialog,"pathReadStartup",savedStartup,true); require(dialog->findChild<QLineEdit *>("pathStartupFile")->text()==startup,"Read chooser Cancel changed identity");
         choose(dialog,"pathSaveStartup",savedStartup); require(QFileInfo::exists(savedStartup) && read(savedStartup).contains("QT_CUSTOM_FILE_ABS"),"Startup save lost custom aliases");
         const auto savedText=read(savedStartup); choose(dialog,"pathSaveStartup",missionFile); require(read(missionFile)==source.toUtf8() && dialog->findChild<QLabel *>("pathStatus")->text().contains("different"),"Startup export overwrote mission");
         auto *paths=dynamic_cast<PathSettingsDialog *>(dialog); require(paths && !paths->exportStartup(files.filePath("missing/settings.txt")).isEmpty() && paths->exportStartup(savedStartup).isEmpty(),"Startup write failure/retry failed");
         const auto aliasFile=files.filePath("mission alias.txt"); require(QFile::link(missionFile,aliasFile) && !paths->exportStartup(aliasFile).isEmpty() && read(missionFile)==source.toUtf8(),"Startup save overwrote mission through alias");
         choose(dialog,"pathSaveStartup",savedStartup,true); require(read(savedStartup)==savedText,"Startup Save Cancel changed file");
         choose(dialog,"pathReadStartup",savedStartup); require(dialog->findChild<QLineEdit *>("pathStartupFile")->text()==savedStartup && functions()==currentFunctions && output()==currentOutput,"Read startup was not pending");
         dialog->findChild<QTabWidget *>("pathTabs")->setCurrentIndex(2); capture(dialog,screenshots.isEmpty() ? QString() : screenshots+".startup.png");
      }); require(functions()==currentFunctions && output()==currentOutput && fm->GetAbsPathname("QT_CUSTOM_FILE_ABS")==alias,"Imported startup Cancel changed engine");
      modal(window,[&] { action->trigger(); },[&](QDialog *dialog) { choose(dialog,"pathReadStartup",savedStartup); apply(dialog); require(dialog->findChild<QLabel *>("pathStatus")->text().startsWith("Applied"),"Saved startup Apply failed"); });
      require(fm->GetFullStartupFilePath()==savedStartup.toStdString() && window.runMission()==MainWindow::RunResult::Completed && read(out2+"/path-report.txt")==report,"Startup reload changed calculation or identity");
      QProcess child; auto environment=QProcessEnvironment::systemEnvironment(); environment.insert("QT_QPA_PLATFORM","offscreen"); child.setProcessEnvironment(environment); child.setWorkingDirectory(QFileInfo(startup).absolutePath());
      const auto executable=QFileInfo(startup).absolutePath()+"/GmatQt"; child.start(executable,{"--startup",savedStartup,"--settings-dir",files.filePath("child settings"),"--run","--screenshot",files.filePath("child.png"),missionFile}); const bool childDone=child.waitForFinished(30000); if (!childDone || child.exitCode()!=0) { QFile::remove("/tmp/gmat-path-child.png"); QFile::copy(files.filePath("child.png"),"/tmp/gmat-path-child.png"); } require(childDone && child.exitCode()==0 && read(out2+"/path-report.txt")==report,qPrintable("Saved startup child launch failed ("+QString::number(child.exitCode())+"): "+child.errorString()+"\n"+QString::fromUtf8(child.readAllStandardError())+"\n"+QString::fromUtf8(read(out2+"/GmatLog.txt"))));
      editor->setPlainText("Create Variable n;\nBeginMissionSequence;\nWhile n < 1e12;\nn = n + 1;\nEndWhile;\n"); bool disabled=false; QTimer::singleShot(20,&window,[&] { disabled=!action->isEnabled(); window.stopMission(); }); require(window.runMission()==MainWindow::RunResult::Stopped && disabled && action->isEnabled(),"Paths not disabled during run/re-enabled after Stop");
      editor->setPlainText(source); require(window.runMission()==MainWindow::RunResult::Completed && read(out2+"/path-report.txt")==report,"Stopped mission did not recover path execution");
      std::cout<<"Qt paths: ordered Add/Replace/Remove/Up/Down, directory choosers, Cancel/Apply, invalid recovery, default/explicit reports, log relocation, exact startup preview rollback, aliases/Python/modes/log preservation, Unicode export/read/apply/child launch, independent function-path reports and Stop guard passed\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
