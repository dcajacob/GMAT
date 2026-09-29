#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "Moderator.hpp"
#include <QApplication>
#include <QAction>
#include <QClipboard>
#include <QMenu>
#include <QKeyEvent>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTreeWidget>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
static int find(const MissionSnapshot &snapshot,const QString &type,const QString &parent={})
{
   for (int i=0;i<snapshot.nodes.size();++i) {
      const auto &node=snapshot.nodes[i];
      if (node.type==type && (parent.isEmpty() || (node.parent>=0 && snapshot.nodes[node.parent].type==parent))) return i;
   }
   throw std::runtime_error("Expected mission node missing");
}
static int forAssignment(const MissionSnapshot &snapshot)
{
   for (int i=0;i<snapshot.nodes.size();++i) {
      const auto &node=snapshot.nodes[i];
      if (node.parent>=0 && snapshot.nodes[node.parent].type=="For" && node.statement.contains("total = total + 1")) return i;
   }
   throw std::runtime_error("For-body assignment missing");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtMission");
   if (argc<3 || argc>4) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(), script=QFileInfo(argv[2]).absoluteFilePath();
   const auto screenshot=argc==4 ? QFileInfo(argv[3]).absoluteFilePath() : QString();
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      MainWindow window; window.show(); require(window.initialize(startup),"Runtime failed");
      require(window.loadScript(script) && window.buildScript(),"Mission failed to build");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString original=editor->toPlainText();
      auto snapshot=window.missionSnapshot();
      for (const auto &node:snapshot.nodes) {
         if (node.start<0) std::cerr<<"Unmapped "<<node.type.toStdString()<<": "<<node.statement.toStdString()<<'\n';
         require(node.start>=0 && node.end>node.start,"Command source range not mapped");
      }
      require(snapshot.nodes[find(snapshot,"If","If")].parent>=0,"Nested branch not represented");
      require(find(snapshot,"Else","If")>=0 && find(snapshot,"EndFor","For")>=0,"Branch structure missing");
      auto total=[&] {
         require(window.runMission()==MainWindow::RunResult::Completed,"Mission did not run");
         auto *value=Moderator::Instance()->GetInternalObject("total"); require(value!=nullptr,"Result variable missing");
         return value->GetRealParameter("Value");
      };
      require(total()==6,"Initial mission result incorrect");
      snapshot=window.missionSnapshot();
      const int assignment=forAssignment(snapshot);
      const auto error=window.applyMissionChange(snapshot,assignment,MissionEdit::Replace,"total = total + 2;");
      if (!error.isEmpty()) std::cerr<<error.toStdString()<<'\n';
      require(error.isEmpty() && total()==9,"Repeated-command edit changed wrong branch");
      require(!window.applyMissionChange(snapshot,assignment,MissionEdit::Remove,{}).isEmpty(),"Stale mission editor was accepted");
      editor->undo(); require(editor->toPlainText()==original && window.buildScript() && total()==6,"Mission edit lost undo history");
      snapshot=window.missionSnapshot();
      require(!window.applyMissionChange(snapshot,forAssignment(snapshot),MissionEdit::Replace,"Propagate Missing(Spacecraft);").isEmpty(),"Invalid mission command accepted");
      require(editor->toPlainText()==snapshot.sourceScript && total()==6,"Invalid edit did not restore previous mission");
      require(window.applyMissionChange(snapshot,forAssignment(snapshot),MissionEdit::InsertBefore,"total = total + 10;").isEmpty(),"Corrected insertion after failure rejected");
      require(total()==36,"Inserted command did not execute inside selected loop");
      require(window.loadScript(script) && window.buildScript(),"Could not restore test mission");
      snapshot=window.missionSnapshot();
      require(window.applyMissionChange(snapshot,find(snapshot,"If"),MissionEdit::Remove,{}).isEmpty() && total()==4,"Deleting a branch did not remove its whole block");
      require(window.loadScript(script) && window.buildScript(),"Could not restore test mission");
      snapshot=window.missionSnapshot();
      require(window.applyMissionChange(snapshot,find(snapshot,"For"),MissionEdit::InsertAfter,"total = total + 10;").isEmpty() && total()==16,"Insert-after remained inside branch");
      require(window.loadScript(script) && window.buildScript(),"Could not restore test mission");
      snapshot=window.missionSnapshot();
      require(window.applyMissionChange(snapshot,-1,MissionEdit::Append,"total = total + 10;").isEmpty() && total()==16,"Append mission command failed");
      require(window.loadScript(script) && window.buildScript(),"Could not restore test mission");
      snapshot=window.missionSnapshot();
      require(window.applyMissionChange(snapshot,find(snapshot,"BeginScript"),MissionEdit::Replace,
         "BeginScript;\n   total = total + 5;\nEndScript;").isEmpty() && total()==10,
         "Script event replacement did not replace the complete block");
      require(window.loadScript(script) && window.buildScript(),"Could not restore test mission");
      snapshot=window.missionSnapshot();
      require(!window.applyMissionChange(snapshot,find(snapshot,"EndIf"),MissionEdit::Remove,{}).isEmpty(),"Structural end command could be deleted alone");
      auto *tree=window.findChild<QTreeWidget *>("Mission"); require(tree!=nullptr,"Mission tree missing");
      QTreeWidgetItem *target=nullptr;
      const int selected=forAssignment(snapshot);
      for (QTreeWidgetItemIterator it(tree);*it;++it)
         if ((*it)->data(0,Qt::UserRole).isValid() && (*it)->data(0,Qt::UserRole).toInt()==selected) target=*it;
      require(target && target->parent()->data(0,Qt::UserRole+1).toString()=="For","Tree hierarchy wrong");
      window.findChild<QTabWidget *>()->setCurrentIndex(1);
      tree->expandAll(); tree->setCurrentItem(target); tree->itemDoubleClicked(target,0);
      CommandEditor *panel=nullptr;
      for (auto *widget:window.findChildren<QWidget *>()) if (auto *candidate=dynamic_cast<CommandEditor *>(widget)) panel=candidate;
      require(panel!=nullptr,"Mission double click did not open editor");
      auto *commandSource=panel->findChild<QPlainTextEdit *>("commandSource");
      commandSource->setFocus(); QApplication::processEvents();
      const auto scriptBeforeTextEdit=editor->toPlainText();
      const auto commandBeforeTextEdit=commandSource->toPlainText();
      commandSource->selectAll(); QApplication::clipboard()->setText("total = total + 4;");
      window.findChild<QAction *>("edit_paste")->trigger();
      require(commandSource->toPlainText()=="total = total + 4;" && editor->toPlainText()==scriptBeforeTextEdit,"Paste targeted the wrong document");
      QMenu *editMenu=nullptr;
      for (auto *menu:window.findChildren<QMenu *>()) if (menu->title()=="&Edit") editMenu=menu;
      require(editMenu!=nullptr,"Edit menu missing");
      editMenu->popup(window.mapToGlobal(QPoint(100,30))); QApplication::processEvents();
      editMenu->setActiveAction(window.findChild<QAction *>("edit_undo"));
      QKeyEvent activate(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);
      QApplication::sendEvent(editMenu,&activate); QApplication::processEvents();
      require(commandSource->toPlainText()==commandBeforeTextEdit && editor->toPlainText()==scriptBeforeTextEdit,"Undo targeted the script instead of the command panel");
      window.findChild<QAction *>("edit_redo")->trigger();
      require(commandSource->toPlainText()=="total = total + 4;","Redo did not target command editor");
      tree->setFocus(); QApplication::processEvents();
      window.findChild<QAction *>("edit_undo")->trigger();
      require(editor->toPlainText()==scriptBeforeTextEdit && commandSource->toPlainText()=="total = total + 4;","Undo with tree focus changed an editor");
      require(panel->hasChanges() && !window.buildScript(),"Unapplied mission changes did not protect Build");
      if (!screenshot.isEmpty()) { QApplication::processEvents(); require(window.grab().save(screenshot),"Mission screenshot failed"); }
      panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
      require(total()==15,"Mission panel Apply did not update the loop");
      std::cout<<"PASS: nested branches, duplicate-command identity, replace/insert/delete/append, invalid-edit rollback, stale panel, undo, Mission tree and Apply\n";
   } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
