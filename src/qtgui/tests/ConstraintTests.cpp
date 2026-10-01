#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "ReportParameterDialog.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
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
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Constraint output missing"); return file.readAll(); }
static QMdiSubWindow *open(MainWindow &window) {
   auto *tree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItemIterator it(tree);
   while (*it && (*it)->data(0,Qt::UserRole+1)!="NonlinearConstraint") ++it;
   require(*it,"Constraint command unavailable"); tree->itemDoubleClicked(*it,0);
   auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow(); require(child && dynamic_cast<CommandEditor *>(child->widget()),"Constraint MDI panel unavailable"); return child;
}
static void closePanels(MainWindow &window) {
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (auto *panel=dynamic_cast<CommandEditor *>(child->widget())) { panel->discardChanges(); child->close(); }
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
static QLineEdit *field(QMdiSubWindow *child,const QString &name) {
   auto *input=child->widget()->findChild<QLineEdit *>("commandField_"+name); require(input,qPrintable("Constraint field unavailable: "+name)); return input;
}
static void pick(QMdiSubWindow *child,const QString &side,const QString &value,bool accept=true,const QString &capture={}) {
   std::exception_ptr failure; bool visited=false;
   QTimer::singleShot(0,child,[&] {
      auto *dialog=dynamic_cast<ReportParameterDialog *>(QApplication::activeModalWidget()); visited=true;
      try {
         require(dialog,"Constraint operand picker unavailable");
         auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry"); auto *buttons=dialog->findChild<QDialogButtonBox *>();
         require(entry->findText("Label")<0,"Constraint picker offers String objects");
         dialog->findChild<QComboBox *>("reportPropertyObject")->setCurrentText("Sat");
         auto *properties=dialog->findChild<QComboBox *>("reportPropertyType"); require(properties->findText("UTCGregorian")<0 && properties->findText("X")>=0,"Constraint property picker includes nonnumeric values");
         for (const auto &invalid:{QString("Label"),QString("Grid"),QString("Grid(2,1)"),QString("nan")}) { entry->setEditText(invalid); require(!buttons->button(QDialogButtonBox::Ok)->isEnabled(),qPrintable("Constraint picker accepts "+invalid)); }
         if (value.startsWith("Grid(")) {
            entry->setCurrentText("Grid"); dialog->findChild<QSpinBox *>("reportArrayRow")->setValue(1); dialog->findChild<QSpinBox *>("reportArrayColumn")->setValue(value.contains("1,2") ? 2 : 1); dialog->findChild<QPushButton *>("reportAddElement")->click();
         } else if (value=="Sat.EarthMJ2000Eq.X") {
            properties->setCurrentText("X"); dialog->findChild<QComboBox *>("reportPropertyDependency")->setCurrentText("EarthMJ2000Eq"); dialog->findChild<QPushButton *>("reportUseReference")->click();
         } else entry->setEditText(value);
         require(dialog->selection()==QStringList{value} && buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Constraint scalar selection mismatch");
         if (!capture.isEmpty()) { QEventLoop loop; QTimer::singleShot(200,&loop,&QEventLoop::quit); loop.exec(); require(dialog->windowHandle() && dialog->windowHandle()->isExposed() && dialog->grab().save(capture),"Native constraint picker not exposed"); }
         buttons->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
      } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
   });
   auto *button=child->widget()->findChild<QPushButton *>("commandChoose_"+side); require(button,"Constraint operand action unavailable"); button->click(); require(visited,"Constraint picker unvisited"); if (failure) std::rethrow_exception(failure);
}
int main(int argc,char **argv) {
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtConstraints");
   try {
      TestSettings settings; QTemporaryDir files; require((argc==2 || argc==3) && files.isValid(),"Constraint setup failed");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 && QString::fromLocal8Bit(argv[2])!="--dynamic-bound" ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Constraint runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto report=files.filePath("values.txt"),saved=files.filePath("constraint Δ.script"),saveAs=files.filePath("constraint copy ü.script");
      const QString base="% constraint qualification α, preserve implicit settings\nCreate Variable Value Cost Limit;\nLimit = 1;\nCreate Array Grid[1,2];\nGrid(1,2) = 3;\nCreate String Label;\nLabel = 'text';\nCreate Spacecraft Sat;\nCreate Yukon Opt;\nOpt.ShowProgress = false;\nOpt.ReportFile = '"+files.filePath("solver.txt")+"';\nOpt.FeasibilityTolerance = 0.00000001;\nOpt.OptimalityTolerance = 0.00000001;\nOpt.FunctionTolerance = 0.00000001;\nCreate DifferentialCorrector DC;\nCreate ReportFile Results;\nResults.Filename = '"+report+"';\nResults.WriteHeaders = false;\nResults.Precision = 17;\nResults.SolverIterations = All;\nBeginMissionSequence;\nOptimize 'retain optimizer' Opt {ShowProgressWindow = false}; % retain header\nVary Opt(Value = 0, {Perturbation = 0.000001, Lower = -10, Upper = 10});\nCost = (Value - 2)^2;\nGrid(1,1) = Value;\nSat.X = Value;\nMinimize Opt(Cost);\n";
      const QString tail="Report Results Value Cost Grid(1,1) Sat.EarthMJ2000Eq.X; % retain trials\nEndOptimize; % retain branch\nReport Results Value Cost Grid(1,1) Sat.EarthMJ2000Eq.X; % retain accepted\n";
      struct Case { QString initial,left,relation,right; double optimum; };
      QList<Case> cases={{"Grid(1,1) <= 4","Grid(1,1)",">=","Grid(1,2)",3},
         {"Value <= 4","Value","<=","Limit",1},
         {"Value <= 4","Sat.EarthMJ2000Eq.X","=","+1.5",1.5},
         {"Value <= 4","2","<=","Grid(1,2)",2},
         {"Value <= 4","Value",">=","-1e0",2}};
      if (argc==3 && QString::fromLocal8Bit(argv[2])=="--dynamic-bound") cases={{"Value <= 4","3","<=","Value",3}};
      auto run=[&](double optimum) {
         require(window.runMission()==MainWindow::RunResult::Completed,qPrintable(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()));
         auto *value=Moderator::Instance()->GetInternalObject("Value"); require(value && std::abs(value->GetRealParameter("Value")-optimum)<2e-5,qPrintable("Constrained optimum differs from analytic solution: expected="+QString::number(optimum,'g',17)+" actual="+(value ? QString::number(value->GetRealParameter("Value"),'g',17) : "missing")));
         const auto data=read(report); const auto rows=QString::fromUtf8(data).trimmed().split('\n'); require(rows.size()>1,"Constraint trial/accepted report missing");
         const auto final=rows.last().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(final.size()==4,"Constraint report columns missing");
         require(std::abs(final[0].toDouble()-optimum)<2e-5 && std::abs(final[1].toDouble()-std::pow(optimum-2,2))<2e-5 && std::abs(final[2].toDouble()-final[0].toDouble())<1e-10 && std::abs(final[3].toDouble()-final[0].toDouble())<1e-10,"Constraint report does not match known scalar/array/property solution"); return data;
      };
      int index=0;
      for (const auto &test:cases) {
         const QString command="NonlinearConstraint 'retain constraint' Opt( "+test.initial+" ); % keep constraint\n";
         const auto source=base+command+tail,expected=QString(source).replace(test.initial,test.left+" "+test.relation+" "+test.right);
         editor->setPlainText(expected); const auto reference=run(test.optimum); closePanels(window);
         editor->setPlainText(source); require(window.buildScript(),"Constraint initial source failed"); auto *child=open(window);
         field(child,"Left side"); auto *relation=child->widget()->findChild<QComboBox *>("commandChoice_Relation"); require(relation && relation->count()==3 && relation->findText("<=")>=0 && relation->findText(">=")>=0 && relation->findText("=")>=0,"Constraint relation dropdown missing");
         pick(child,"Right side",test.right,false); require(editor->toPlainText()==source && !dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Constraint picker Cancel changed source or pending panel");
         pick(child,"Left side",test.left); relation->setCurrentText(test.relation); pick(child,"Right side",test.right,true,index==0 && !capture.isEmpty() ? capture+".picker.png" : QString());
         require(editor->toPlainText()==source && dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Constraint picker applied before Apply");
         child->widget()->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         require(editor->toPlainText()==expected && !dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Constraint Apply changed unrelated source or did not retain clean panel");
         editor->undo(); require(editor->toPlainText()==source,"Constraint Undo not exact"); editor->redo(); require(editor->toPlainText()==expected,"Constraint Redo not exact");
         closePanels(window); require(window.saveScriptTo(saved) && window.saveScriptTo(saveAs) && read(saved)==expected.toUtf8() && read(saveAs)==expected.toUtf8() && window.loadScript(saveAs),"Constraint Unicode Save/Save As/reopen not exact"); require(run(test.optimum)==reference,"GUI constraint full iteration report differs from independent script");
         child=open(window);
         if (index==0 && !capture.isEmpty()) { QEventLoop loop; QTimer::singleShot(200,&loop,&QEventLoop::quit); loop.exec(); require(window.grab().save(capture+".png"),"Native constraint capture failed"); QFile state(capture+".state.txt"); require(state.open(QIODevice::WriteOnly) && state.write(reference)==reference.size(),"Constraint evidence report failed"); }
         // Free text is preserved, but the transactional Apply rejects invalid
         // types/references without replacing the applied mission/source.
         if (index==0) for (const auto &bad:QStringList{"Label","Grid","Grid(2,1)","MissingValue"}) {
            field(child,"Right side")->setText(bad); child->widget()->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
            require(editor->toPlainText()==expected && dynamic_cast<CommandEditor *>(child->widget())->hasChanges() && !child->widget()->findChild<QLabel *>("commandStatus")->text().isEmpty(),qPrintable("Invalid constraint accepted or pending input lost: "+bad+"; status="+child->widget()->findChild<QLabel *>("commandStatus")->text()+"; source="+editor->toPlainText()));
         }
         if (index==0) { field(child,"Right side")->setText(test.right); child->widget()->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(editor->toPlainText()==expected && !dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Constraint failed-edit correction did not recover"); } closePanels(window); if (index==0) require(run(test.optimum)==reference,"Constraint corrected rollback model did not execute the reference result");
         std::cout<<"PASS: "<<test.left.toStdString()<<' '<<test.relation.toStdString()<<' '<<test.right.toStdString()<<"; analytic Value="<<test.optimum<<"; complete GUI/script report identical; exact source/Undo/Redo/Unicode reopen .\n"; ++index;
      }
      std::cout<<"PASS: actual constraint MDI relation controls, numeric filtered single picker, array element indices and frame-dependent property browser; pending/Cancel/retained Apply; all three relations, active/inactive inequality, literal on either side with fixed bounds, variable/array/property execution; invalid operand rollback/correction and reference-result rerun.\n";
   } catch (BaseException &e) { std::cerr<<"FAIL: "<<e.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
