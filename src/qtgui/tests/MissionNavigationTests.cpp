#include "MainWindow.hpp"
#include "MissionNavigation.hpp"
#include "CommandEditor.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QEventLoop>
#include <QWindow>
#include <QSettings>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Navigation report missing"); return file.readAll(); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtMissionNavigation");
   if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Navigation fixtures unavailable"); MainWindow window; window.show(); require(window.initialize(startup),"Navigation runtime unavailable");
      const auto report=files.filePath("values.txt");
      const QString source="% preserve tree view source α\nCreate Variable V i;\nV = 1;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nBeginMissionSequence;\nIf 'keep branch' V > 0;\n   For i = 1:1:2;\n      V = V + 1; % keep nested assignment\n      Report Values V;\n   EndFor;\nElse;\n   V = V + 10;\nEndIf;\nBeginScript 'keep script event';\n   V = V + 2;\nEndScript;\nReport 'keep report' Values V;\n";
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); editor->setPlainText(source); require(window.runMission()==MainWindow::RunResult::Completed,"Independent navigation reference failed"); const auto expected=read(report);
      auto *tree=window.findChild<QTreeWidget *>("Mission"); auto *nav=dynamic_cast<MissionNavigation *>(window.findChild<QWidget *>("missionNavigation")); require(tree && nav,"Mission navigation missing");
      const auto snapshot=window.missionSnapshot(); editor->appendPlainText("% pending view test"); const auto pending=editor->toPlainText();
      auto visible=[&](const QString &type) { int count=0; for (QTreeWidgetItemIterator it(tree);*it;++it) if (!(*it)->isHidden() && (*it)->data(0,Qt::UserRole+1)==type) ++count; return count; };
      auto filter=[&](const QString &type,bool exclude,bool apply=true,bool bulk=false) {
         std::exception_ptr failure; QTimer::singleShot(0,&window,[&] {
            try {
               auto *dialog=nav->findChild<QDialog *>("missionFilterDialog"); require(dialog,"Filter dialog missing"); auto *list=dialog->findChild<QListWidget *>("missionFilterTypes");
               dialog->findChild<QPushButton *>(bulk ? "missionFilterCheckAll" : "missionFilterUncheckAll")->click();
               if (!type.isEmpty()) { const auto entries=list->findItems(type,Qt::MatchExactly); require(entries.size()==1,"Command filter alias/type unavailable"); entries[0]->setCheckState(Qt::Checked); }
               dialog->findChild<QComboBox *>("missionFilterMode")->setCurrentIndex(exclude ? 1 : 0);
               if (apply) dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
               if (!capture.isEmpty() && apply && type=="Report" && !exclude) { QEventLoop loop; QTimer::singleShot(200,&loop,&QEventLoop::quit); loop.exec(); require(dialog->windowHandle() && dialog->windowHandle()->isExposed() && dialog->grab().save(capture+".filter.png"),"Native command filter did not expose"); }
               dialog->reject();
            } catch (...) { failure=std::current_exception(); if (auto *dialog=nav->findChild<QDialog *>("missionFilterDialog")) dialog->reject(); }
         }); nav->findChild<QAction *>("missionFilter")->trigger(); if (failure) std::rethrow_exception(failure);
         require(editor->toPlainText()==pending && window.missionSnapshot().sourceScript==snapshot.sourceScript,"Tree filter modified the pending document or mission snapshot");
      };
      filter("Report",false); require(visible("Report")==2 && visible("If")==1 && visible("For")==1 && visible("GMAT")==0 && visible("EndFor")==1 && visible("EndIf")==1,"Report filter lost nested branch context or showed unrelated assignments");
      filter({},false,false,true); require(visible("Report")==2 && visible("GMAT")==0,"Closing unapplied filter changed the view");
      filter("Report",true); require(visible("Report")==0 && visible("GMAT")>=2,"Exclude filter did not retain other command types");
      filter("Equation",false); require(visible("GMAT")>=2 && visible("Report")==0,"Equation alias filter failed");
      filter("ScriptEvent",false); require(visible("BeginScript")==1 && visible("Report")==0,"ScriptEvent alias filter failed");
      bool completeEvent=false; for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole+1)=="BeginScript") completeEvent=(*it)->toolTip(0).contains("EndScript"); require(completeEvent,"ScriptEvent filter lost its retained boundary/source");
      filter({},false); require(visible("Report")==0 && visible("If")==0,"Empty include filter was treated as Show all");
      nav->findChild<QAction *>("missionShowAll")->trigger(); require(visible("Report")==2 && visible("GMAT")>=2,"Show all did not recover the complete mission tree");
      auto *depth=nav->findChild<QComboBox *>("missionDepth"); depth->setCurrentIndex(0); require(!tree->topLevelItem(0)->isExpanded(),"Collapsed view did not collapse"); depth->setCurrentIndex(depth->count()-1);
      QTreeWidgetItem *assignment=nullptr; for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->toolTip(0).contains("V = V + 2")) assignment=*it;
      require(assignment && assignment->parent()->isExpanded(),"Expand all did not expose ScriptEvent contents"); tree->setCurrentItem(assignment);
      auto *tabs=window.findChild<QDockWidget *>("navigation")->findChild<QTabWidget *>(); auto *dock=window.findChild<QDockWidget *>("missionDock");
      tabs->setCurrentWidget(nav); QApplication::processEvents(); require(tree->isVisible(),"Mission page hides its tree after adding controls");
      for (int repeat=0;repeat<3;++repeat) {
         nav->detachAction()->trigger(); QApplication::processEvents(); require(dock->isFloating() && dock->isVisible() && tabs->count()==2 && tree->currentItem()==assignment && tree->window()==dock,"Mission-only undock lost selection or moved other navigation tabs");
         if (!capture.isEmpty() && repeat==0) { QEventLoop loop; QTimer::singleShot(250,&loop,&QEventLoop::quit); loop.exec(); require(dock->windowHandle() && dock->windowHandle()->isExposed() && dock->grab().save(capture+".undocked.png"),"Native mission-only window is not exposed"); }
         dock->close(); QApplication::processEvents(); require(!dock->isVisible() && tabs->count()==3 && tabs->widget(1)==nav && tree->currentItem()==assignment && !nav->detachAction()->isChecked(),"Closing the mission window did not restore its tab and selection");
      }
      QSettings().setValue("MissionTree/detached",true); nav->restorePlacement(); require(dock->isFloating() && tabs->count()==2,"Saved mission-only placement did not restore"); dock->setFloating(false); QApplication::processEvents(); require(tabs->count()==3 && !dock->isVisible() && !QSettings().value("MissionTree/detached").toBool(),"Docking the floating tree did not restore its tab and saved placement");
      editor->undo(); require(editor->toPlainText()==source,"Navigation damaged source Undo"); editor->redo(); require(editor->toPlainText()==pending,"Navigation damaged source Redo"); editor->undo();
      nav->detachAction()->trigger(); tree->itemDoubleClicked(assignment,0);
      auto *workspace=window.findChild<QMdiArea *>("workspace"); QMdiSubWindow *command=nullptr; for (auto *child:workspace->subWindowList()) if (dynamic_cast<CommandEditor *>(child->widget())) command=child;
      require(command,"Undocked tree cannot open the real command editor"); auto *text=command->widget()->findChild<QPlainTextEdit *>("commandSource"); text->setPlainText(QString(text->toPlainText()).replace("V + 2","V + 4"));
      command->widget()->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(editor->toPlainText().contains("V + 4") && editor->toPlainText().contains("% keep nested assignment"),"Undocked editor Apply failed or changed unrelated source"); command->close();
      nav->setDetached(false); require(window.saveScriptTo(files.filePath("mission tree ü.script")) && window.loadScript(files.filePath("mission tree ü.script")) && window.runMission()==MainWindow::RunResult::Completed,"Edited navigation mission cannot save/reopen/run"); const auto edited=read(report);
      editor->setPlainText(QString(source).replace("V + 2","V + 4")); require(window.runMission()==MainWindow::RunResult::Completed && read(report)==edited && edited!=expected,"Navigation edit differs from independent calculation or did not take effect");
      if (!capture.isEmpty()) require(window.grab().save(capture+".docked.png"),"Docked navigation capture failed");
      std::cout<<"PASS: command include/exclude filters, Equation/ScriptEvent aliases, Check/Uncheck all, unapplied Close, nested branch boundaries, expansion levels/Show all, source/snapshot/Undo/Redo retention; repeated mission-only undock/close/restore with selection and other tabs retained, command editing from the floating tree, Unicode save/reopen and independent reports\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
