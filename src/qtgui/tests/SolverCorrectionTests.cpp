#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "SolverBranchCommand.hpp"
#include "Solver.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QMdiSubWindow *open(MainWindow &window,const QString &label) {
   const auto snapshot=window.missionSnapshot(); int index=-1;
   for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].label.endsWith(" — "+label)) index=i;
   if (index>=0 && !snapshot.nodes[index].editable) std::cerr<<snapshot.canonicalScript.toStdString()<<'\n';
   require(index>=0 && snapshot.nodes[index].editable,qPrintable("Solver correction source mapping unavailable for "+label));
   auto *tree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItemIterator item(tree);
   while (*item && (!(*item)->data(0,Qt::UserRole).isValid() || (*item)->data(0,Qt::UserRole).toInt()!=index)) ++item;
   require(index>=0 && *item,"Solver correction command not found"); tree->itemDoubleClicked(*item,0);
   auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow(); require(child && dynamic_cast<CommandEditor *>(child->widget()),"Solver correction panel missing"); return child;
}
static void closePanels(MainWindow &window) {
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (auto *panel=dynamic_cast<CommandEditor *>(child->widget())) { panel->discardChanges(); child->close(); }
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
static QString click(QMdiSubWindow *child) {
   auto *button=child->widget()->findChild<QPushButton *>("applySolverCorrections"); require(button,"Apply Corrections button missing"); button->click();
   return child->widget()->findChild<QLabel *>("commandStatus")->text();
}
int main(int argc,char **argv) {
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtSolverCorrections");
   try {
      TestSettings settings; QTemporaryDir files; require((argc==2 || argc==3) && files.isValid(),"Solver correction setup failed");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Solver correction runtime unavailable");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); const auto saved=files.filePath("Solver corrections Δ.script"),report=files.filePath("results.txt");
      const QString source="% preserve corrections α and implicit settings\nCreate Variable Alpha Beta Gamma Delta Seed;\nSeed = 1;\nCreate DifferentialCorrector Outer Inner;\nCreate Yukon Opt;\nOpt.ShowProgress = false;\nOpt.OptimalityTolerance = 0.00000001;\nOpt.FunctionTolerance = 0.00000001;\nCreate ReportFile Results;\nResults.Filename = '"+report+"';\nResults.WriteHeaders = false;\nResults.Precision = 16;\nBeginMissionSequence;\n"
         "Target 'outer corrections' Outer {ShowProgressWindow = false}; % keep branch\n"
         "   Vary 'scaled guess' Outer(Alpha = +1.000e0, {Perturbation = 0.01, Lower = 0, Upper = 10, MaxStep = 1, AdditiveScaleFactor = 3, MultiplicativeScaleFactor = 2}); % keep literal\n"
         "   Vary 'reference guess' Outer(Beta = Seed, {Perturbation = 0.01, Lower = 0, Upper = 10, MaxStep = 1}); % keep reference\n"
         "   Target 'inner corrections' Inner {ShowProgressWindow = false};\n"
         "      Vary 'inner guess' Inner(Gamma = 2, {Perturbation = 0.01, Lower = 0, Upper = 10, MaxStep = 1});\n"
         "      Achieve Inner(Gamma = 7, {Tolerance = 0.000001});\n"
         "   EndTarget; % retain inner\n"
         "   Achieve Outer(Alpha = 4, {Tolerance = 0.000001});\n"
         "   Achieve Outer(Beta = 5, {Tolerance = 0.000001});\n"
         "EndTarget; % retain outer\n"
         "Optimize 'optimizer corrections' Opt {ShowProgressWindow = false};\n"
         "   Vary 'optimizer guess' Opt(Delta = 1, {Perturbation = 0.000001, Lower = 0, Upper = 3});\n"
         "   Seed = (Delta - 2)^2;\n"
         "   Minimize Opt(Seed);\nEndOptimize;\n"
         "Report Results Alpha Beta Gamma Delta; % retain final report\n";
      auto run=[&] {
         require(window.runMission()==MainWindow::RunResult::Completed,qPrintable(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()));
         for (const auto &entry:QList<QPair<QString,double>>{{"Alpha",4},{"Beta",5},{"Gamma",7},{"Delta",2}}) {
            auto *value=Moderator::Instance()->GetInternalObject(entry.first.toStdString()); require(value && std::abs(value->GetRealParameter("Value")-entry.second)<1e-5,qPrintable("Unexpected post-run "+entry.first+" = "+(value ? QString::number(value->GetRealParameter("Value"),'g',17) : "missing")));
         }
         QFile file(report); require(file.open(QIODevice::ReadOnly),"Correction report missing"); return file.readAll();
      };
      editor->setPlainText(source); require(window.buildScript(),"Correction source did not build");
      auto *outer=open(window,"outer corrections"); require(click(outer).contains("Run the current mission") && editor->toPlainText()==source,"Corrections accepted before a run");
      closePanels(window); const auto baseline=run(); require(!baseline.isEmpty(),"Correction baseline report empty");
      outer=open(window,"outer corrections"); auto *vary=open(window,"scaled guess"); auto *pending=vary->widget()->findChild<QPlainTextEdit *>("commandSource"); const auto applied=pending->toPlainText(); pending->setPlainText(applied+"\n% pending companion");
      require(click(outer).contains("pending panel") && editor->toPlainText()==source && pending->toPlainText().contains("pending companion"),"Corrections overwrote pending Vary edits");
      pending->setPlainText(applied); auto *outerText=outer->widget()->findChild<QPlainTextEdit *>("commandSource"); const auto outerApplied=outerText->toPlainText(); outerText->setPlainText(outerApplied+"\n% pending branch");
      require(click(outer).contains("this panel's pending") && editor->toPlainText()==source,"Corrections accepted pending branch edits"); outerText->setPlainText(outerApplied);
      click(outer); const auto corrected=editor->toPlainText();
      const auto literal=QRegularExpression("Alpha = ([^,]+),").match(corrected).captured(1); require(std::abs(literal.toDouble()-4)<1e-6,"Scaled corrections did not store the unscaled value");
      require(corrected==QString(source).replace("Alpha = +1.000e0,","Alpha = "+literal+","),"Outer corrections changed labels/comments/options/references/nested guesses");
      require(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().contains("retained reference guesses: Beta = Seed"),"Reference retention was not explained");
      require(!dynamic_cast<CommandEditor *>(outer->widget())->hasChanges() && vary->widget()->findChild<QPlainTextEdit *>("commandSource")->toPlainText().contains("Alpha = "+literal),"Correction did not refresh retained clean solver/Vary panels");
      editor->undo(); require(editor->toPlainText()==source,"Corrections Undo not exact"); editor->redo(); require(editor->toPlainText()==corrected,"Corrections Redo not exact");
      require(click(outer).contains("Run the current mission") && editor->toPlainText()==corrected,"Old result accepted after correction rebuild");
      closePanels(window); require(window.saveScriptTo(saved) && window.loadScript(saved),"Unicode correction save/reopen failed");
      const auto correctedReport=run();
      // A better initial guess changes trial counts, not the final physical values.
      const auto values=QString::fromUtf8(correctedReport).trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(values.size()==4 && std::abs(values[0].toDouble()-4)<1e-6 && std::abs(values[1].toDouble()-5)<1e-6 && std::abs(values[2].toDouble()-7)<1e-6 && std::abs(values[3].toDouble()-2)<1e-5,"Reopened corrected report differs from known goals");
      auto *inner=open(window,"inner corrections"); const auto beforeInner=editor->toPlainText(); click(inner); const auto afterInner=editor->toPlainText(); const auto innerLiteral=QRegularExpression("Gamma = ([^,]+),").match(afterInner).captured(1); require(std::abs(innerLiteral.toDouble()-7)<1e-6 && afterInner==QString(beforeInner).replace("Gamma = 2,","Gamma = "+innerLiteral+","),"Inner correction changed another solver block");
      closePanels(window); run(); auto *opt=open(window,"optimizer corrections"); const auto beforeOpt=editor->toPlainText(); click(opt); const auto afterOpt=editor->toPlainText(); const auto optLiteral=QRegularExpression("Delta = ([^,]+),").match(afterOpt).captured(1); require(std::abs(optLiteral.toDouble()-2)<1e-5 && afterOpt==QString(beforeOpt).replace("Delta = 1,","Delta = "+optLiteral+","),"Optimizer correction did not preserve exact unrelated source");
      editor->undo(); require(editor->toPlainText()==beforeOpt,"Optimizer correction Undo not exact"); editor->redo(); require(editor->toPlainText()==afterOpt,"Optimizer correction Redo not exact");
      if (argc==3) { QApplication::processEvents(); require(window.grab().save(QString::fromLocal8Bit(argv[2])+".png"),"Correction native capture failed"); }
      closePanels(window); require(window.saveScriptTo(saved) && window.loadScript(saved),"Combined corrections reopen failed"); run();
      const QString simple="Create Variable Value;\nCreate DifferentialCorrector DC;\nBeginMissionSequence;\n"
         "Target 'additional corrections' DC {ShowProgressWindow = false};\n"
         "Vary 'hidden guess' DC(Value = 1, {Perturbation = 0.01, MaxStep = 1}); % retain hidden guess\n"
         "Achieve DC(Value = 4, {Tolerance = 0.000001});\nEndTarget;\n";
      const auto hidden=QString(simple).replace("Vary 'hidden guess'","BeginScript; % retain script event\nVary 'hidden guess'").replace("Achieve DC", "EndScript; % retain event end\nAchieve DC");
      editor->setPlainText(hidden); require(window.runMission()==MainWindow::RunResult::Completed,"Script-event correction solve failed"); auto *event=open(window,"additional corrections"); click(event);
      const auto hiddenAfter=editor->toPlainText(); const auto hiddenLiteral=QRegularExpression("Value = ([^,]+),").match(hiddenAfter).captured(1);
      require(std::abs(hiddenLiteral.toDouble()-4)<1e-6 && hiddenAfter==QString(hidden).replace("Value = 1,","Value = "+hiddenLiteral+","),"Script-event Vary correction lost source ownership");
      editor->undo(); require(editor->toPlainText()==hidden,"Script-event correction Undo not exact"); editor->redo(); require(editor->toPlainText()==hiddenAfter,"Script-event correction Redo not exact"); closePanels(window);
      const auto unexecuted=QString(simple).replace("Target 'additional", "If 0 == 1;\nTarget 'additional").replace("EndTarget;", "EndTarget;\nEndIf;");
      editor->setPlainText(unexecuted); require(window.runMission()==MainWindow::RunResult::Completed,"Unexecuted solver fixture failed"); auto *skipped=open(window,"additional corrections"); require(click(skipped).contains("Run this solver block") && editor->toPlainText()==unexecuted,"Unexecuted solver accepted corrections"); closePanels(window);
      const auto stopping=QString(simple).replace("ShowProgressWindow = false", "ExitMode = Stop, ShowProgressWindow = false");
      editor->setPlainText(stopping); require(window.runMission()==MainWindow::RunResult::Failed && !window.isRunning(),"ExitMode Stop did not interrupt mission"); auto *stopped=open(window,"additional corrections"); click(stopped);
      const auto stoppedAfter=editor->toPlainText(); const auto stoppedLiteral=QRegularExpression("Value = ([^,]+),").match(stoppedAfter).captured(1); require(std::abs(stoppedLiteral.toDouble()-4)<1e-6 && stoppedAfter==QString(stopping).replace("Value = 1,","Value = "+stoppedLiteral+","),"Stopped solver correction traversed incomplete execution state or changed other source"); closePanels(window);
      editor->setPlainText(QString(stoppedAfter).replace("ExitMode = Stop", "ExitMode = SaveAndContinue")); require(window.runMission()==MainWindow::RunResult::Completed,"Correction recovery after ExitMode Stop failed");
      const auto nonconverged=QString(simple).replace("Create DifferentialCorrector DC;", "Create DifferentialCorrector DC;\nDC.MaximumIterations = 2;");
      editor->setPlainText(nonconverged); const auto result=window.runMission(); require(result!=MainWindow::RunResult::Busy && !window.isRunning(),"Nonconverged solver retained the mission lock");
      auto *command=Moderator::Instance()->GetFirstCommand(); while (command && !command->IsOfType("Target")) command=command->GetNext();
      auto *branch=dynamic_cast<SolverBranchCommand *>(command); auto *solver=branch ? dynamic_cast<Solver *>(branch->GetClone(0)) : nullptr;
      require(solver && solver->GetIntegerParameter(solver->GetParameterID("IntegerSolverStatus"))==Gmat::EXCEEDED_ITERATIONS,"Nonconverged fixture did not exceed its iteration limit");
      const auto lastValue=solver->GetSolverVariable(0); require(std::isfinite(lastValue) && std::abs(lastValue-1)>1e-6,"Nonconverged fixture produced no new guess");
      auto *last=open(window,"additional corrections"); click(last); const auto lastAfter=editor->toPlainText(); const auto lastLiteral=QRegularExpression("Value = ([^,]+),").match(lastAfter).captured(1);
      require(std::abs(lastLiteral.toDouble()-lastValue)<1e-10 && lastAfter==QString(nonconverged).replace("Value = 1,","Value = "+lastLiteral+","),"Nonconverged correction did not use the solver's last value");
      closePanels(window); editor->setPlainText(QString(lastAfter).replace("DC.MaximumIterations = 2;", "DC.MaximumIterations = 30;")); require(window.runMission()==MainWindow::RunResult::Completed && std::abs(Moderator::Instance()->GetInternalObject("Value")->GetRealParameter("Value")-4)<1e-6,"Nonconverged correction/recovery failed");
      std::cout<<"PASS: actual Target/Optimize Apply Corrections controls; independently known goals 4/5/7/2, scale factors, reference retention/explanation, nested and script-event ownership, pending/stale/unexecuted guards, corrections after ExitMode Stop and nonconvergence with recovery, retained clean companion panels, exact unrelated source/Undo/Redo and Unicode save/reopen execution. No prior viewer suite repeated.\n";
   } catch (BaseException &e) { std::cerr<<"FAIL: "<<e.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
