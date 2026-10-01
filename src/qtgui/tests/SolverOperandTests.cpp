#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "ReportParameterDialog.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QWindow>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Solver operand output missing"); return file.readAll(); }
static QMdiSubWindow *open(MainWindow &window,const QString &type) {
   auto *tree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItemIterator it(tree); while (*it && (*it)->data(0,Qt::UserRole+1)!=type) ++it;
   require(*it,qPrintable("Solver operand command unavailable: "+type)); tree->itemDoubleClicked(*it,0); auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow(); require(child && dynamic_cast<CommandEditor *>(child->widget()),"Solver operand panel unavailable"); return child;
}
static QLineEdit *field(QMdiSubWindow *child,const QString &name) { auto *input=child->widget()->findChild<QLineEdit *>("commandField_"+name); require(input,qPrintable("Array operand form field unavailable: "+name)); return input; }
static void apply(QMdiSubWindow *child) { child->widget()->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); }
static void closePanels(MainWindow &window) { for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (auto *panel=dynamic_cast<CommandEditor *>(child->widget())) { panel->discardChanges(); child->close(); } QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static void pick(QMdiSubWindow *child,const QString &name,const QString &array,int column,bool accept=true,bool numeric=false,const QString &capture={}) {
   bool visited=false; std::exception_ptr failure;
   QTimer::singleShot(0,child,[&] {
      auto *dialog=dynamic_cast<ReportParameterDialog *>(QApplication::activeModalWidget()); visited=true;
      try {
         require(dialog,"Solver operand picker missing"); auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry"); auto *buttons=dialog->findChild<QDialogButtonBox *>();
         require(entry->findText("Note")<0,"Solver operand picker offers String"); entry->setEditText("7"); require(buttons->button(QDialogButtonBox::Ok)->isEnabled()==numeric,"Solver operand picker literal restriction wrong");
         entry->setCurrentText(array); require(!buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Solver operand picker accepts whole array");
         dialog->findChild<QSpinBox *>("reportArrayRow")->setValue(1); dialog->findChild<QSpinBox *>("reportArrayColumn")->setValue(column); dialog->findChild<QPushButton *>("reportAddElement")->click();
         require(dialog->selection()==QStringList{array+QString("(1,%1)").arg(column)} && buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Solver operand picker did not select indexed element");
         if (!capture.isEmpty()) { QEventLoop loop; QTimer::singleShot(200,&loop,&QEventLoop::quit); loop.exec(); require(dialog->windowHandle() && dialog->windowHandle()->isExposed() && dialog->grab().save(capture),"Native solver operand picker not exposed"); }
         buttons->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
      } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
   });
   auto *button=child->widget()->findChild<QPushButton *>("commandChoose_"+name); require(button,qPrintable("Solver operand picker action unavailable: "+name)); button->click(); require(visited,"Solver operand picker not visited"); if (failure) std::rethrow_exception(failure);
}
int main(int argc,char **argv) {
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtSolverOperands");
   try {
      TestSettings settings; QTemporaryDir files; require((argc==2 || argc==3) && files.isValid(),"Solver operand setup failed");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath()); MainWindow window; window.show(); require(window.initialize(startup),"Solver operand runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      for (bool optimizer:{false,true}) {
         const QString kind=optimizer ? "Optimize" : "Target",solver=optimizer ? "Opt" : "DC"; const auto report=files.filePath(kind+".txt"),saved=files.filePath(kind+" operands Δ.script"),saveAs=files.filePath(kind+" copy ü.script");
         const QString base="% preserve array operands α and implicit settings\nCreate Array State[1,3] Params[1,8] Goals[1,3] CostGrid[1,2];\nParams(1,1) = 0;\nParams(1,2) = 0.000001;\nParams(1,3) = -10;\nParams(1,4) = 10;\nParams(1,5) = 1;\nParams(1,6) = 2;\nParams(1,7) = 3;\nParams(1,8) = 0.000001;\nGoals(1,1) = 3;\nGoals(1,2) = 4;\nGoals(1,3) = 0.0000001;\nCreate String Note;\nNote = 'retained';\nCreate "+(optimizer ? QString("Yukon Opt;\nOpt.ShowProgress = false;\nOpt.OptimalityTolerance = 0.00000001;\nOpt.FunctionTolerance = 0.00000001;\n") : QString("DifferentialCorrector DC;\nDC.ShowProgress = false;\n"))+solver+".ReportFile = '"+files.filePath(kind+"-solver.txt")+"';\nCreate ReportFile Results;\nResults.Filename = '"+report+"';\nResults.WriteHeaders = false;\nResults.Precision = 17;\nResults.SolverIterations = All;\nBeginMissionSequence;\n"+kind+" 'retain branch' "+solver+" {ShowProgressWindow = false}; % preserve header\n";
         const QString vary="Vary 'retain vary' "+solver+"( State(1,1) = Params(1,1), {Upper = Params(1,4), Perturbation = Params(1,2), MaxStep = Params(1,5), Lower = Params(1,3), AdditiveScaleFactor = Params(1,6), MultiplicativeScaleFactor = Params(1,7)} ); % retain all options\n";
         const QString objective=optimizer ? "CostGrid(1,1) = (State(1,2)-2)^2;\nCostGrid(1,2) = (State(1,2)-3)^2;\nMinimize 'retain objective' Opt( CostGrid(1,1) ); % retain objective\n" : "Achieve 'retain achieve' DC( State(1,1) = Goals(1,1), {Tolerance = Params(1,8)} ); % retain achieve\n";
         const QString tail="Report Results State(1,1) State(1,2) CostGrid(1,2) Goals(1,2); % retain trials\nEnd"+kind+"; % retain branch end\nReport Results State(1,1) State(1,2) CostGrid(1,2) Goals(1,2); % retain accepted\n";
         const auto source=base+vary+objective+tail;
         const auto varied=QString(vary).replace("State(1,1) = Params(1,1)","State(1,2) = Params(1,5)").replace("Upper = Params(1,4)",optimizer ? "Upper = 8" : "Upper = Params(1,4)").replace("Lower = Params(1,3)",optimizer ? "Lower = -5" : "Lower = Params(1,3)");
         const auto afterVary=base+varied+objective+tail;
         const auto desired=optimizer ? QString(objective).replace("Opt( CostGrid(1,1)","Opt( CostGrid(1,2)") : QString(objective).replace("State(1,1) = Goals(1,1)","State(1,2) = Goals(1,2)").replace("Tolerance = Params(1,8)","Tolerance = Goals(1,3)");
         const auto expected=base+varied+desired+tail;
         auto run=[&] {
            require(window.runMission()==MainWindow::RunResult::Completed,qPrintable(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText())); const auto data=read(report); const auto rows=QString::fromUtf8(data).trimmed().split('\n'); require(rows.size()>1,"Solver operand report lacks trials"); const auto final=rows.last().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);
            require(final.size()==4 && std::abs(final[0].toDouble())<1e-12 && std::abs(final[1].toDouble()-(optimizer ? 3 : 4))<2e-5 && std::abs(final[3].toDouble()-4)<1e-12 && (!optimizer || std::abs(final[2].toDouble())<1e-10),"Array solver result differs from independent goal/optimum"); return data;
         };
         editor->setPlainText(expected); const auto reference=run(); closePanels(window);
         editor->setPlainText(source); require(window.buildScript(),"Array solver source build failed"); auto *varyChild=open(window,"Vary");
         for (const auto &name:QStringList{"Variable","Initial value","Perturbation","Lower","Upper","MaxStep","AdditiveScaleFactor","MultiplicativeScaleFactor"}) field(varyChild,name);
         require(field(varyChild,"Upper")->text()=="Params(1,4)" && field(varyChild,"Lower")->text()=="Params(1,3)" && field(varyChild,"MultiplicativeScaleFactor")->text()=="Params(1,7)","Array option separator confused source spans");
         pick(varyChild,"Variable","State",2,false); require(!dynamic_cast<CommandEditor *>(varyChild->widget())->hasChanges() && editor->toPlainText()==source,"Vary picker Cancel changed state");
         pick(varyChild,"Variable","State",2); field(varyChild,"Initial value")->setText("Params(1,5)");
         if (optimizer) { field(varyChild,"Lower")->setText("-5"); field(varyChild,"Upper")->setText("8"); }
         require(editor->toPlainText()==source && dynamic_cast<CommandEditor *>(varyChild->widget())->hasChanges(),"Vary operands applied prematurely"); apply(varyChild);
         require(editor->toPlainText()==afterVary && !dynamic_cast<CommandEditor *>(varyChild->widget())->hasChanges(),"Vary Apply changed array operands/unrelated source or lost retained panel"); editor->undo(); require(editor->toPlainText()==source,"Vary array Undo not exact"); editor->redo(); require(editor->toPlainText()==afterVary,"Vary array Redo not exact");
         auto *objectiveChild=open(window,optimizer ? "Minimize" : "Achieve"); const QString primary=optimizer ? "Objective" : "Goal"; field(objectiveChild,primary);
         pick(objectiveChild,primary,optimizer ? "CostGrid" : "State",2,false); require(!dynamic_cast<CommandEditor *>(objectiveChild->widget())->hasChanges(),"Goal/objective picker Cancel changed state"); pick(objectiveChild,primary,optimizer ? "CostGrid" : "State",2);
         if (!optimizer) { pick(objectiveChild,"Value","Goals",2,true,true); pick(objectiveChild,"Tolerance","Goals",3,true,true,!capture.isEmpty() ? capture+".tolerance.png" : QString()); }
         require(editor->toPlainText()==afterVary && dynamic_cast<CommandEditor *>(objectiveChild->widget())->hasChanges(),"Goal/objective operands applied prematurely"); apply(objectiveChild); require(editor->toPlainText()==expected && !dynamic_cast<CommandEditor *>(objectiveChild->widget())->hasChanges(),"Goal/objective Apply changed unrelated source"); editor->undo(); require(editor->toPlainText()==afterVary,"Goal/objective Undo not exact"); editor->redo(); require(editor->toPlainText()==expected,"Goal/objective Redo not exact");
         closePanels(window); require(window.saveScriptTo(saved) && window.saveScriptTo(saveAs) && read(saved)==expected.toUtf8() && read(saveAs)==expected.toUtf8() && window.loadScript(saveAs),"Array solver Unicode Save/Save As/reopen changed source"); require(run()==reference,"GUI array solver trial/accepted report differs from independent script");
         varyChild=open(window,"Vary"); objectiveChild=open(window,optimizer ? "Minimize" : "Achieve");
         if (!capture.isEmpty()) { QEventLoop loop; QTimer::singleShot(200,&loop,&QEventLoop::quit); loop.exec(); require(window.grab().save(capture+"."+kind+".png"),"Native array solver capture failed"); QFile evidence(capture+"."+kind+".state.txt"); require(evidence.open(QIODevice::WriteOnly) && evidence.write(reference)==reference.size(),"Array solver evidence report failed"); }
         field(objectiveChild,primary)->setText(optimizer ? "CostGrid(2,1)" : "State(2,1)"); apply(objectiveChild); require(editor->toPlainText()==expected && dynamic_cast<CommandEditor *>(objectiveChild->widget())->hasChanges() && objectiveChild->widget()->findChild<QLabel *>("commandStatus")->text().contains("indices"),"Out-of-range array goal/objective accepted"); field(objectiveChild,primary)->setText(optimizer ? "CostGrid(1,2)" : "State(1,2)"); apply(objectiveChild); require(editor->toPlainText()==expected && !dynamic_cast<CommandEditor *>(objectiveChild->widget())->hasChanges(),"Array goal/objective correction failed");
         field(varyChild,"Perturbation")->setText("Params(2,1)"); apply(varyChild); require(editor->toPlainText()==expected && dynamic_cast<CommandEditor *>(varyChild->widget())->hasChanges() && varyChild->widget()->findChild<QLabel *>("commandStatus")->text().contains("indices"),"Out-of-range array Vary option accepted"); field(varyChild,"Perturbation")->setText("Params(1,2)");
         if (optimizer) {
            field(varyChild,"Lower")->setText("12"); apply(varyChild); require(editor->toPlainText()==expected && dynamic_cast<CommandEditor *>(varyChild->widget())->hasChanges() && varyChild->widget()->findChild<QLabel *>("commandStatus")->text().contains("bound"),"Inverted literal Vary bounds accepted"); field(varyChild,"Lower")->setText("-5");
            field(varyChild,"Initial value")->setText("9"); apply(varyChild); require(editor->toPlainText()==expected && dynamic_cast<CommandEditor *>(varyChild->widget())->hasChanges() && varyChild->widget()->findChild<QLabel *>("commandStatus")->text().contains("bound"),"Initial value outside literal Vary bounds accepted"); field(varyChild,"Initial value")->setText("Params(1,5)");
         }
         apply(varyChild); require(editor->toPlainText()==expected && !dynamic_cast<CommandEditor *>(varyChild->widget())->hasChanges(),"Array Vary correction failed"); closePanels(window); require(run()==reference,"Array solver failed-edit recovery altered complete report");
         std::cout<<"PASS: "<<kind.toStdString()<<" array Vary/initial/options and "<<(optimizer ? "array Minimize, optimum 3" : "array Achieve/value/tolerance, goal 4")<<"; picker filtering/Cancel/pending/retained Apply, exact source/Undo/Redo/Unicode Save/Save As/reopen, complete independent report and invalid-index/bound correction/rerun.\n";
      }
   } catch (BaseException &e) { std::cerr<<"FAIL: "<<e.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
