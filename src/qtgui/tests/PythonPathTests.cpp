#include "MainWindow.hpp"
#include "PathSettings.hpp"
#include "PathSettingsDialog.hpp"
#include "TestSettings.hpp"
#include "FileManager.hpp"
#include "GmatGlobal.hpp"
#include "MessageInterface.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QRegularExpression>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),qPrintable("Missing fixture: "+path)); return file.readAll(); }
static void write(const QString &path,const QByteArray &text) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(text)==text.size(),"Fixture write failed"); }
static QStringList selection(QListWidget *list) { QStringList result; for (int i=0;i<list->count();++i) result.append(list->item(i)->text()); return result; }
static QString unrelated(const QString &text)
{
   QStringList lines; const QRegularExpression changed("^\\s*(OUTPUT_PATH|GMAT_FUNCTION_PATH|PYTHON_MODULE_PATH)\\s*=.*$");
   for (const auto &line:text.split('\n')) if (!changed.match(line).hasMatch()) lines.append(line);
   return lines.join('\n');
}
template<class Open,class Inspect> static void modal(QWidget &owner,Open open,Inspect inspect)
{
   std::exception_ptr error; bool inspected=false;
   QTimer::singleShot(0,&owner,[&] {
      inspected=true; auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
      try { require(dialog,"Expected modal missing"); inspect(dialog); } catch (...) { error=std::current_exception(); }
      if (dialog && dialog->isVisible()) dialog->reject();
   });
   open(); require(inspected,"Modal inspection did not execute"); if (error) std::rethrow_exception(error);
}
static void choose(QDialog *owner,const char *button,const QString &path,bool cancel=false)
{
   modal(*owner,[&] { owner->findChild<QPushButton *>(button)->click(); },[&](QDialog *dialog) {
      auto *chooser=qobject_cast<QFileDialog *>(dialog); require(chooser,"File/directory chooser missing");
      chooser->setOption(QFileDialog::DontConfirmOverwrite,true);
      if (!cancel) { chooser->setDirectory(QFileInfo(path).absolutePath()); chooser->selectFile(path); QMetaObject::invokeMethod(chooser,"accept",Qt::DirectConnection); }
   });
}
static void freshImport(const QString &app,const QString &startup,const QString &script,const QString &report,const QString &artifacts,int ordinal,double expected)
{
   QFile::remove(report);
   QProcess child; auto environment=QProcessEnvironment::systemEnvironment(); environment.insert("QT_QPA_PLATFORM","offscreen"); child.setProcessEnvironment(environment);
   child.setWorkingDirectory(QFileInfo(app).absolutePath());
   child.start(app,{"--startup",startup,"--settings-dir",artifacts+"/settings"+QString::number(ordinal),"--run","--screenshot",artifacts+"/child"+QString::number(ordinal)+".png",script});
   const bool finished=child.waitForFinished(15000);
   if (!finished) { child.kill(); child.waitForFinished(2000); }
   require(finished && child.exitStatus()==QProcess::NormalExit && child.exitCode()==0,qPrintable("Fresh startup import failed: "+child.errorString()+"\n"+QString::fromUtf8(child.readAllStandardError())));
   const auto values=QString::fromUtf8(read(report)).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);
   bool valid=false; require(values.size()==1,"Fresh Python result row count wrong"); const auto actual=values.first().toDouble(&valid);
   require(valid && std::isfinite(actual) && actual==expected,"Fresh startup did not use the declared first Python module path");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtPythonPaths");
   try {
      TestSettings isolated; QTemporaryDir files; require(argc==3 && files.isValid(),"Python path fixture setup failed");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(),executable=QFileInfo(QString::fromLocal8Bit(argv[2])).absoluteFilePath();
      QDir::setCurrent(QFileInfo(executable).absolutePath());
      const auto first=files.filePath("first modules ü"),second=files.filePath("second.modules"),spare=files.filePath("replacement modules"),out=files.filePath("output"),laterOut=files.filePath("output later");
      for (const auto &directory:QStringList{first,second,spare,out,laterOut}) require(QDir().mkpath(directory),"Python path directory fixture failed");
      write(first+"/QtPathPriorityTest.py","def result():\n    return 13.25\n"); write(second+"/QtPathPriorityTest.py","def result():\n    return 23.5\n");
      MainWindow window; window.show(); require(window.initialize(startup),"Python path runtime initialization failed"); auto *fm=FileManager::Instance();
      const auto state=fm->CaptureState(); struct Restore { std::shared_ptr<const FileManager::State> state; ~Restore() { FileManager::Instance()->RestoreState(*state); } } restore{state};
      fm->SetAbsPathname("OUTPUT_PATH",out.toStdString()); MessageInterface::SetLogPath(out.toStdString(),true);
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); const QString source="% preserve the active mission α\nCreate Variable Marker;\nBeginMissionSequence;\nMarker = 1;\n";
      const auto activeMission=files.filePath("unchanged mission.script"); editor->setPlainText(source); require(window.saveScriptTo(activeMission) && window.buildScript(),"Parent fixture mission failed");
      auto *action=window.findChild<QAction *>("setPaths"); require(action && action->isEnabled(),"Set paths action missing");
      const auto livePython=fm->GetAllPythonModulePaths(),liveFunctions=fm->GetAllGmatFunctionPaths();
      const auto identity=fm->GetFullStartupFilePath(); const auto echo=GmatGlobal::Instance()->EchoCommands();
      auto baseline=capturePathSettings(startup);
      const auto imported=files.filePath("imported startup ü.txt"),saved=files.filePath("exported startup ü.txt"),reordered=files.filePath("reordered startup.txt"),report=files.filePath("priority result.txt"),call=files.filePath("priority.script");
      const auto alias=files.filePath("unknown alias Δ.dat"); write(alias,"preserve alias input\n");
      // Ordinary _FILE aliases split the absolute dirname/basename. A raw
      // _FILE_ABS getter has independent legacy path-spelling behavior.
      baseline.startupText+="\n## preserve Python path comment α\nQT_PYTHON_SENTINEL_FILE = "+alias+"\nQT_PYTHON_FIRST_PATH = "+first+"/\n";
      baseline.pythonModules={first,second};
      auto importedText=pathSettingsScript(baseline); importedText.replace("PYTHON_MODULE_PATH = "+first+"/","PYTHON_MODULE_PATH = QT_PYTHON_FIRST_PATH"); write(imported,importedText.toUtf8());
      const auto originalImported=read(imported);
      const auto malformed=files.filePath("malformed startup.txt"); write(malformed,"ECHO_COMMANDS = TRUE\nPYTHON_MODULE_PATH = "+first.toUtf8()+"/\nBROKEN without equals\n");
      auto unchanged=[&] { require(fm->GetAllPythonModulePaths()==livePython && fm->GetAllGmatFunctionPaths()==liveFunctions && fm->GetFullStartupFilePath()==identity && GmatGlobal::Instance()->EchoCommands()==echo && editor->toPlainText()==source,"Pending/Cancel changed live paths, modes, identity or mission"); };
      modal(window,[&] { action->trigger(); },[&](QDialog *dialog) {
         auto *list=dialog->findChild<QListWidget *>("pathPythonModules"); require(list,"Python module path editor missing");
         auto *note=dialog->findChild<QLabel *>("pathPythonRestartNote"); require(note && note->text().contains("restart") && note->text().contains("Apply does not"),"Python path restart boundary missing");
         auto *entry=dialog->findChild<QLineEdit *>("pathPythonEntry"); list->clear(); entry->setText(first); dialog->findChild<QPushButton *>("pathPythonAdd")->click();
         choose(dialog,"pathPythonBrowse",second,true); require(entry->text()==first,"Python directory chooser Cancel changed pending text");
         unchanged();
      }); unchanged();
      modal(window,[&] { action->trigger(); },[&](QDialog *dialog) {
         choose(dialog,"pathReadStartup",malformed); require(dialog->findChild<QLabel *>("pathStatus")->text().contains("expecting '='"),"Malformed Python startup error missing"); unchanged();
         choose(dialog,"pathReadStartup",imported); auto *list=dialog->findChild<QListWidget *>("pathPythonModules"); auto *entry=dialog->findChild<QLineEdit *>("pathPythonEntry");
         require(selection(list)==QStringList{first,second},"Imported Python rows leaked earlier live paths or lost alias/order"); unchanged();
         list->clear(); entry->setText(files.filePath("missing")); dialog->findChild<QPushButton *>("pathPythonAdd")->click(); require(list->count()==0,"Missing Python directory added");
         choose(dialog,"pathPythonBrowse",second); dialog->findChild<QPushButton *>("pathPythonAdd")->click(); entry->setText(first); dialog->findChild<QPushButton *>("pathPythonAdd")->click(); require(selection(list)==QStringList{first,second},"Python Add/Browse order wrong");
         entry->setText(first+"/"); dialog->findChild<QPushButton *>("pathPythonAdd")->click(); require(list->count()==2,"Equivalent Python directory duplicated");
         list->setCurrentRow(0); dialog->findChild<QPushButton *>("pathPythonDown")->click(); require(selection(list)==QStringList{second,first},"Python Down wrong"); dialog->findChild<QPushButton *>("pathPythonUp")->click();
         entry->setText(second); dialog->findChild<QPushButton *>("pathPythonReplace")->click(); require(selection(list)==QStringList{first,second},"Duplicate Python Replace changed row");
         entry->setText(spare); dialog->findChild<QPushButton *>("pathPythonReplace")->click(); require(selection(list)==QStringList{spare,second},"Python Replace wrong"); dialog->findChild<QPushButton *>("pathPythonRemove")->click(); entry->setText(first); dialog->findChild<QPushButton *>("pathPythonAdd")->click();
         list->addItem(files.filePath("missing")); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(dialog->findChild<QLabel *>("pathStatus")->text().contains("unavailable"),"Invalid Python Apply accepted"); unchanged(); delete list->takeItem(list->count()-1);
         choose(dialog,"pathSaveStartup",saved,true); require(!QFileInfo::exists(saved),"Startup export Cancel wrote a file"); choose(dialog,"pathSaveStartup",saved);
         const auto text=QString::fromUtf8(read(saved)); require(unrelated(text)==unrelated(importedText) && text.count(QRegularExpression("(?m)^PYTHON_MODULE_PATH ="))==2 && text.indexOf("PYTHON_MODULE_PATH = "+first)<text.indexOf("PYTHON_MODULE_PATH = "+second),"Export lost unrelated startup source or Python order");
         require(dialog->findChild<QLabel *>("pathStatus")->text().contains("Restart GMAT with this file"),"Export claimed live Python Apply");
         const auto priorExport=read(saved); auto *paths=dynamic_cast<PathSettingsDialog *>(dialog); require(paths && !paths->exportStartup(activeMission).isEmpty() && read(activeMission)==source.toUtf8(),"Python export overwrote active mission");
         require(!paths->exportStartup(files.filePath("missing/settings.txt")).isEmpty() && read(saved)==priorExport,"Failed export damaged previous startup");
         dialog->findChild<QLineEdit *>("pathOutput")->setText(laterOut); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         require(dialog->findChild<QLabel *>("pathStatus")->text().startsWith("Applied GMAT") && fm->GetAllPythonModulePaths()==livePython && QDir::cleanPath(QString::fromStdString(fm->GetAbsPathname("OUTPUT_PATH")))==laterOut && editor->toPlainText()==source,"Other path Apply changed live Python ordering or source");
         const auto actualAlias=QString::fromStdString(fm->GetAbsPathname("QT_PYTHON_SENTINEL_FILE"));
         const auto actualCanonical=QFileInfo(actualAlias).canonicalFilePath(),expectedCanonical=QFileInfo(alias).canonicalFilePath();
         require(!expectedCanonical.isEmpty() && actualCanonical==expectedCanonical,qPrintable("Other path Apply lost an unknown startup alias: actual=["+actualAlias+"] expected=["+alias+"] actual canonical=["+actualCanonical+"] expected canonical=["+expectedCanonical+"]"));
      });
      require(read(imported)==originalImported && read(activeMission)==source.toUtf8(),"Path editing changed original input files");
      const auto afterPython=fm->GetAllPythonModulePaths(); const auto afterIdentity=fm->GetFullStartupFilePath();
      modal(window,[&] { action->trigger(); },[&](QDialog *dialog) {
         choose(dialog,"pathReadStartup",saved); auto *list=dialog->findChild<QListWidget *>("pathPythonModules"); require(selection(list)==QStringList{first,second},"Export/import did not retain Python order independently of live state");
         list->setCurrentRow(0); dialog->findChild<QPushButton *>("pathPythonDown")->click(); choose(dialog,"pathSaveStartup",reordered);
         list->clear(); auto *paths=dynamic_cast<PathSettingsDialog *>(dialog); const auto empty=files.filePath("empty python startup.txt"); require(paths && paths->exportStartup(empty).isEmpty() && !QString::fromUtf8(read(empty)).contains(QRegularExpression("(?m)^PYTHON_MODULE_PATH\\s*=")),"Remove all did not export an empty configured Python list");
         choose(dialog,"pathReadStartup",reordered); require(selection(list)==QStringList{second,first},"Reordered startup rows did not reopen");
      });
      require(fm->GetAllPythonModulePaths()==afterPython && fm->GetFullStartupFilePath()==afterIdentity && editor->toPlainText()==source,"Reopened startup Cancel changed current session");
      const QString runtime="Create Variable Result;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 16;\nBeginMissionSequence;\n[Result] = Python.QtPathPriorityTest.result();\nReport Values Result;\n"; write(call,runtime.toUtf8());
      freshImport(executable,saved,call,report,files.path(),1,13.25); freshImport(executable,reordered,call,report,files.path(),2,23.5);
      require(read(imported)==originalImported && read(activeMission)==source.toUtf8() && read(alias)=="preserve alias input\n","Fresh children changed source/input files");
      std::cout<<"PASS: Python paths actual Set paths menu/tab controls, Browse/Cancel/Add/Replace/Remove/Up/Down, alias-resolved ordered pending import independent of stale engine paths, invalid Apply rollback, startup export/Cancel/write failure/mission protection, unrelated source retention, other-path Apply preserves live Python, exported empty/reordered lists, and two fresh GmatQt imports select known 13.25/23.5 module results. No live Python reload/cache reset, old path matrix or host desktop check.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
