#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "CommandForm.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include "Moderator.hpp"
#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Call syntax report unavailable"); return file.readAll(); }
static void write(const QString &path,const QByteArray &body) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(body)==body.size(),"Function fixture write failed"); }
static void run(MainWindow &window) { if (window.runMission()!=MainWindow::RunResult::Completed) throw std::runtime_error(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtGmatCallSyntax");
   try {
      TestSettings settings; QTemporaryDir files; require(argc==2 && files.isValid(),"Call syntax setup failed"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Call syntax runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto report=files.filePath("values.txt"),saved=files.filePath("calls Δ.script");
      for (const auto &name:QStringList{"First","Second"}) write(files.filePath(name+".gmf"),("function [output] = "+name+"()\nCreate Variable output;\nBeginMissionSequence;\noutput = "+(name=="First" ? "7" : "13")+";\n").toUtf8());
      write(files.filePath("Touch.gmf"),"function Touch()\nGlobal Keep;\nBeginMissionSequence;\nKeep = Keep + 1;\n");
      const QString prefix="% retain call syntax α\nCreate GmatFunction First Second Touch;\nFirst.FunctionPath = '"+files.filePath("First.gmf")+"';\nSecond.FunctionPath = '"+files.filePath("Second.gmf")+"';\nTouch.FunctionPath = '"+files.filePath("Touch.gmf")+"';\nCreate Variable Result Keep;\nKeep = 0;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nBeginMissionSequence;\nGlobal Keep;\n";
      for (const auto &shape:QStringList{"[Result] = First()","[Result] = First","Result = First()","'Zero result' [Result] = First()","'Bare result' [Result] = First"}) {
         const QString source=prefix+shape+"; % retain edited call\n'Touch once' Touch(); % zero inputs and outputs\n'Touch twice' Touch; % bare no outputs\nReport Values Result Keep;\n";
         auto reference=source; reference.replace(shape,QString(shape).replace("First","Second")); editor->setPlainText(reference); run(window); const auto expected=read(report); const auto values=QString::fromUtf8(expected).simplified().split(' '); require(values.size()==2 && values[0].toDouble()==13 && values[1].toDouble()==2,"Independent GMAT zero-input/no-output execution incorrect");
         editor->setPlainText(source); require(window.buildScript(),"GMAT accepted call syntax did not build"); const auto snapshot=window.missionSnapshot(); int index=-1; for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type=="CallGmatFunction" && snapshot.nodes[i].statement.contains("First")) index=i;
         if (index<0 || !snapshot.nodes[index].editable) std::cerr<<snapshot.canonicalScript.toStdString(); require(index>=0 && snapshot.nodes[index].editable,"GMAT call source mapping unavailable");
         for (const auto &unsafe:QStringList{QString(source).replace(shape,QString(shape).replace("First","Second")),QString(source).replace(shape,QString(shape).replace("Result","Keep")),QString(source).replace("'Touch once'","'Changed label'"),QString(source).replace(shape,(shape.endsWith("()") ? QString(shape).replace("First()","First(Keep)") : shape+"(Keep)"))}) {
            if (unsafe==source) continue; const auto mismatched=snapshotMission(Moderator::Instance()->GetFirstCommand(),snapshot.canonicalScript,unsafe); for (const auto &node:mismatched.nodes) if (node.type=="CallGmatFunction") require(!node.editable,"Different GMAT function/output/arguments mapped as equivalent");
         }
         for (const auto &node:snapshot.nodes) if (node.type=="CallGmatFunction" && node.statement.contains("Touch")) { require(node.editable,"No-output GMAT call not mapped"); CommandForm form([](const QString &) {}); form.setStatement(node.statement); require(form.title()=="Function call" && form.findChild<QLineEdit *>("commandField_Inputs") && form.findChild<QLineEdit *>("commandField_Inputs")->text().isEmpty(),"No-output GMAT call controls missing"); }
         auto *tree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItemIterator item(tree); while (*item && (!(*item)->data(0,Qt::UserRole).isValid() || (*item)->data(0,Qt::UserRole).toInt()!=index)) ++item; require(*item,"Call tree item missing"); tree->itemDoubleClicked(*item,0); auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow(); auto *panel=child ? dynamic_cast<CommandEditor *>(child->widget()) : nullptr; require(panel && panel->property("helpTopic")=="CallGmatFunction","GMAT call editor/Help missing"); auto *input=panel->findChild<QLineEdit *>("commandField_Inputs"); auto *function=panel->findChild<QLineEdit *>("commandField_Function"); require(input && input->text().isEmpty() && function,"Empty GMAT input field missing"); function->setText("Second"); require(panel->hasChanges() && editor->toPlainText()==source,"GMAT call edit did not remain pending"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(editor->toPlainText()==reference && child->isVisible() && !dynamic_cast<EditablePanel *>(child->widget())->hasChanges(),"GMAT retained Apply changed source spelling/comments"); editor->undo(); require(editor->toPlainText()==source,"GMAT exact Undo failed"); editor->redo(); require(editor->toPlainText()==reference && window.saveScriptTo(saved) && window.loadScript(saved),"GMAT Redo/Unicode reopen failed"); run(window); require(read(report)==expected,"GUI-edited GMAT result differs from independent source"); std::cout<<"Covered: "<<shape.toStdString()<<'\n';
      }
      // A human can start with the output-bearing call template, give it a
      // caption, then clear outputs to call a no-output function. The caption
      // must survive Apply and reopening instead of becoming a quoted LHS.
      const QString withOutputs=prefix+"[Result] = First(); % retain call tail\nReport Values Result Keep;\n";
      editor->setPlainText(withOutputs); require(window.buildScript(),"Output removal baseline failed");
      auto removeSnapshot=window.missionSnapshot(); int removeIndex=-1;
      for (int i=0;i<removeSnapshot.nodes.size();++i) if (removeSnapshot.nodes[i].type=="CallGmatFunction") removeIndex=i;
      require(removeIndex>=0,"Output removal call missing");
      auto *removeTree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItemIterator removeItem(removeTree);
      while (*removeItem && (!(*removeItem)->data(0,Qt::UserRole).isValid() || (*removeItem)->data(0,Qt::UserRole).toInt()!=removeIndex)) ++removeItem;
      require(*removeItem,"Output removal tree item missing"); removeTree->itemDoubleClicked(*removeItem,0);
      auto *removeChild=window.findChild<QMdiArea *>("workspace")->activeSubWindow();
      auto *removePanel=removeChild ? dynamic_cast<CommandEditor *>(removeChild->widget()) : nullptr;
      require(removePanel,"Output removal editor missing");
      removePanel->findChild<QLineEdit *>("commandName")->setText("Touch without outputs");
      removePanel->findChild<QLineEdit *>("commandField_Function")->setText("Touch");
      removePanel->findChild<QLineEdit *>("commandField_Outputs")->clear();
      const auto noOutputs=QString(withOutputs).replace("[Result] = First();","'Touch without outputs' Touch();");
      require(editor->toPlainText()==withOutputs && removePanel->findChild<QLineEdit *>("commandName")->text()=="Touch without outputs", "Clearing outputs lost pending name or changed mission before Apply");
      removePanel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
      require(editor->toPlainText()==noOutputs && !dynamic_cast<EditablePanel *>(removeChild->widget())->hasChanges(),"Clearing outputs left assignment separator or lost call caption");
      editor->undo(); require(editor->toPlainText()==withOutputs,"Output removal Undo failed");
      editor->redo(); require(editor->toPlainText()==noOutputs && window.saveScriptTo(saved) && window.loadScript(saved),"Output removal save/reopen failed");
      run(window); const auto noOutputValues=QString::fromUtf8(read(report)).simplified().split(' ');
      require(noOutputValues.size()==2 && noOutputValues[0].toDouble()==0 && noOutputValues[1].toDouble()==1,"No-output GUI call did not execute its independent global increment");
      QString changed; CommandForm form([&](const QString &text) { changed=text; }); const QString bare="  [Result] = First; % retain bare tail\n"; form.setStatement(bare); form.findChild<QLineEdit *>("commandField_Inputs")->setText("Keep"); require(changed==QString(bare).replace("First;","First(Keep);"),"Adding bare GMAT inputs lost parentheses/source"); form.findChild<QLineEdit *>("commandField_Inputs")->clear(); require(changed==bare,"Clearing bare GMAT inputs lost spelling"); form.setStatement("Stop;"); require(!form.findChild<QLineEdit *>("commandField_Function"),"Non-function Stop mistaken for a bare call"); form.setStatement("Result = Keep;"); require(form.title()=="Assignment","Variable assignment mistaken for a bare call");
      // The shared bare-input wrapper changed; check that retained Python
      // field spans still insert and remove parentheses without rerunning Python.
      const QString python="Python.builtins.print; % retained\n"; form.setStatement(python); form.findChild<QLineEdit *>("commandField_Inputs")->setText("Keep"); require(changed==QString(python).replace("print;","print(Keep);"),"Shared wrapper broke bare Python input insertion"); form.findChild<QLineEdit *>("commandField_Inputs")->clear(); require(changed==python,"Shared wrapper broke Python source restoration");
      form.setStatement("  'Python caption' [Result] = Python.builtins.print(); % retained\n");
      form.findChild<QLineEdit *>("commandField_Outputs")->clear();
      require(changed=="  'Python caption' Python.builtins.print(); % retained\n","Shared output removal lost Python label/callee/comments");
      std::cout<<"PASS: actual MDI GMAT empty/bare/scalar call editing, configured-function-only mapping/Help, pending retained Apply, exact Undo/Redo/Unicode save/reopen, independent 13/2 execution and negative mapping; shared Python span smoke only.\n";
   } catch (BaseException &failure) { std::cerr<<"FAIL: "<<failure.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &failure) { std::cerr<<"FAIL: "<<failure.what()<<'\n'; return 1; }
}
