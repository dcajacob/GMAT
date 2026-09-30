#include "MainWindow.hpp"
#include "WelcomeDialog.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QTimer>
#include <QEventLoop>
#include <QWindow>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void write(const QString &path,const QByteArray &text) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(text)==text.size(),"Welcome fixture write failed"); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Welcome report missing"); return file.readAll(); }
static void settle() { QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); QApplication::processEvents(); }
template<class Open,class Inspect> static void modal(QWidget &owner,Open open,Inspect inspect)
{
   std::exception_ptr failure; bool visited=false; QTimer::singleShot(0,&owner,[&] { visited=true; auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); try { require(dialog,"Expected Welcome modal missing"); inspect(dialog); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } }); open(); require(visited,"Welcome modal was not visited"); if (failure) std::rethrow_exception(failure);
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtWelcome");
   if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Welcome fixtures unavailable"); MainWindow window; window.show(); require(window.initialize(startup),"Welcome runtime unavailable");
      const auto a=files.filePath("First ü.script"),b=files.filePath("Second.script"),saved=files.filePath("Saved ü.script"),report=files.filePath("report.txt");
      const auto source=[&](int value) { return ("% Welcome comments α\nCreate Variable V;\nV = "+QString::number(value)+";\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nBeginMissionSequence;\nReport Values V;\n").toUtf8(); };
      write(a,source(1)); write(b,source(2)); require(window.loadScript(a) && window.runMission()==MainWindow::RunResult::Completed,"First Welcome reference failed"); const auto reference=read(report);
      require(window.loadScript(b) && window.buildScript() && window.saveScriptTo(saved),"Recent file load/Save As failed"); require(recentMissionFiles()==QStringList{saved,b,a},"Successful load/Save As history order or identity incorrect");
      require(window.loadScript(files.path()+"/./First ü.script") && recentMissionFiles()==QStringList{a,saved,b},"Equivalent recent path duplicated a mission");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(window.buildScript(),"Welcome source build failed"); editor->appendPlainText("% retain pending Welcome text"); const auto pending=editor->toPlainText(); const auto title=window.windowTitle();
      auto welcome=[&] { return dynamic_cast<WelcomeDialog *>(window.findChild<QDialog *>("welcomeDialog")); };
      QSettings().setValue("Welcome/showOnStartup",false); window.showWelcome(true); require(!welcome(),"Disabled startup preference still opened Welcome");
      window.findChild<QAction *>("showWelcome")->trigger(); auto *dialog=welcome(); require(dialog && dialog->isVisible(),"Welcome menu missing"); window.showWelcome(); require(welcome()==dialog && window.findChildren<QDialog *>("welcomeDialog").size()==1,"Repeated Welcome made duplicate windows");
      auto *recent=dialog->findChild<QListWidget *>("welcomeRecent"); require(recent->count()==3 && recent->item(0)->data(Qt::UserRole)==a && recent->item(0)->toolTip()==a,"Welcome recent path/tooltip missing");
      dialog->findChild<QCheckBox *>("welcomeOnStartup")->setChecked(true); require(QSettings().value("Welcome/showOnStartup").toBool(),"Startup preference did not persist");
      for (const auto &topic:QStringList{"UsingGmat","RefGuide","Tutorials"}) {
         dialog->findChild<QPushButton *>("welcomeHelp"+topic)->click(); settle(); auto *help=window.findChild<QDialog *>("helpDialog"); require(help && help->findChild<QTextBrowser *>("helpBrowser")->source().fileName()==topic+".html" && help->findChild<QLabel *>("helpStatus")->text().isEmpty(),"Welcome offline guide topic failed"); help->close(); settle();
      }
      require(editor->toPlainText()==pending && window.windowTitle()==title && editor->document()->isUndoAvailable(),"Welcome/Help changed source, identity or Undo");
      if (!capture.isEmpty()) { QEventLoop loop; QTimer::singleShot(250,&loop,&QEventLoop::quit); loop.exec(); require(dialog->windowHandle() && dialog->windowHandle()->isExposed() && dialog->grab().save(capture+".welcome.png"),"Native Welcome capture failed"); }
      dialog->resize(480,350); QApplication::processEvents(); auto *close=dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Close); require(dialog->rect().contains(close->mapTo(dialog,close->rect().bottomRight())) && dialog->rect().contains(dialog->findChild<QCheckBox *>("welcomeOnStartup")->mapTo(dialog,QPoint(0,0))),"Compact Welcome hides Close or its startup preference"); dialog->resize(680,540);
      modal(window,[&] { dialog->findChild<QPushButton *>("welcomeOpenRecent")->click(); },[](QDialog *modal) { auto *prompt=qobject_cast<QMessageBox *>(modal); require(prompt,"Unsaved Welcome prompt missing"); prompt->button(QMessageBox::Cancel)->click(); });
      require(dialog->isVisible() && editor->toPlainText()==pending && window.windowTitle()==title && recentMissionFiles().first()==a,"Canceled recent open lost the pending document");
      modal(window,[&] { dialog->findChild<QPushButton *>("welcomeOpenRecent")->click(); },[](QDialog *modal) { qobject_cast<QMessageBox *>(modal)->button(QMessageBox::Discard)->click(); }); settle(); require(!welcome() && editor->toPlainText()==QString::fromUtf8(source(1)) && window.runMission()==MainWindow::RunResult::Completed && read(report)==reference,"Accepted recent open did not build/run the reference mission");
      const auto missing=files.filePath("missing.script"); rememberMissionFile(missing); window.showWelcome(); dialog=welcome(); dialog->findChild<QPushButton *>("welcomeOpenRecent")->click(); require(dialog->findChild<QLabel *>("welcomeStatus")->text().contains(missing) && editor->toPlainText()==QString::fromUtf8(source(1)),"Unavailable recent mission changed the current source or failed silently");
      // Exercise sample Browse with the widget chooser. Portal interaction is
      // qualified separately on the desktop.
      auto browse=[&](bool cancel) { const auto before=editor->toPlainText(); modal(window,[&] { dialog->findChild<QPushButton *>("welcomeBrowseSamples")->click(); },[&](QDialog *modal) { auto *chooser=qobject_cast<QFileDialog *>(modal); require(chooser && chooser->directory().exists(),"Samples chooser or initial directory missing"); if (cancel) chooser->reject(); else { chooser->setDirectory(files.path()); chooser->selectFile(b); QMetaObject::invokeMethod(chooser,"accept",Qt::DirectConnection); } }); if (cancel) require(editor->toPlainText()==before && dialog->isVisible(),"Canceled sample chooser changed the mission"); };
      browse(true); browse(false); settle(); require(!welcome() && editor->toPlainText()==QString::fromUtf8(source(2)) && recentMissionFiles().first()==b && window.runMission()==MainWindow::RunResult::Completed && read(report).trimmed().toDouble()==2,"Sample chooser acceptance did not open/build the selected mission");
      editor->appendPlainText("% preserve New Cancel"); const auto newPending=editor->toPlainText(); window.showWelcome(); dialog=welcome();
      modal(window,[&] { dialog->findChild<QPushButton *>("welcomeNew")->click(); },[](QDialog *modal) { qobject_cast<QMessageBox *>(modal)->button(QMessageBox::Cancel)->click(); }); require(editor->toPlainText()==newPending && dialog->isVisible(),"New mission Cancel lost pending text");
      modal(window,[&] { dialog->findChild<QPushButton *>("welcomeNew")->click(); },[](QDialog *modal) { qobject_cast<QMessageBox *>(modal)->button(QMessageBox::Discard)->click(); }); settle(); require(!welcome() && window.windowTitle().startsWith("Untitled") && editor->toPlainText().contains("DefaultSC"),"New Welcome mission did not restore the engine default");
      auto *recentMenu=window.findChild<QMenu *>("recentMissionsMenu"); QMetaObject::invokeMethod(recentMenu,"aboutToShow",Qt::DirectConnection); QAction *second=nullptr; for (auto *action:recentMenu->actions()) if (action->data()==b) second=action; require(second,"Recent menu lost its full mission identity"); second->trigger(); require(editor->toPlainText()==QString::fromUtf8(source(2)) && window.runMission()==MainWindow::RunResult::Completed && read(report).trimmed().toDouble()==2,"Recent menu did not build/run the selected mission");
      window.showWelcome(); dialog=welcome(); dialog->findChild<QPushButton *>("welcomeClearRecent")->click(); require(recentMissionFiles().isEmpty() && dialog->findChild<QListWidget *>("welcomeRecent")->count()==0 && QFileInfo::exists(a) && QFileInfo::exists(b) && QFileInfo::exists(saved),"Clearing recent history affected mission files");
      dialog->findChild<QCheckBox *>("welcomeOnStartup")->setChecked(false); dialog->close(); settle(); window.showWelcome(true); require(!welcome(),"Disabled preference was lost on reopening"); window.showWelcome(); require(welcome(),"Manual Welcome was disabled by startup preference"); welcome()->close(); settle();
      auto *menu=window.findChild<QMenu *>("recentMissionsMenu"); QMetaObject::invokeMethod(menu,"aboutToShow",Qt::DirectConnection); require(menu->actions().size()==1 && !menu->actions()[0]->isEnabled(),"Empty recent menu has a stale open action");
      std::cout<<"PASS: Welcome/menu/single window, startup preference persistence, recent load/Save As/equivalent-path ordering, full-path tooltips, offline guides, pending source/identity/Undo retention, dirty Cancel/Discard, missing-file recovery, sample widget Browse Cancel/accept, New mission Cancel/accept, clear history preserving files and independent numeric reports\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
