#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ReportParameterDialog.hpp"
#include "XYPlotDialog.hpp"
#include "QtPlotReceiver.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
static void later(QWidget *owner,std::exception_ptr &failure,std::function<void()> action)
{
   QTimer::singleShot(0,owner,[owner,&failure,action] {
      try { action(); } catch (...) { failure=std::current_exception(); for (auto *dialog:owner->findChildren<QDialog *>()) dialog->reject(); if (auto *dialog=qobject_cast<QDialog *>(owner)) dialog->reject(); }
   });
}
static void close(QDialog *dialog,bool accepted=true) { dialog->findChild<QDialogButtonBox *>()->button(accepted ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click(); }
static QTableWidgetItem *field(ResourceEditor &panel,const QString &name)
{
   auto *table=panel.findChild<QTableWidget *>();
   for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==name) return table->item(row,1);
   throw std::runtime_error("Plot field missing");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setAttribute(Qt::AA_DontUseNativeDialogs); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtPlotSetup");
   if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString();
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Plot fixture directory missing");
      MainWindow window; window.show(); require(window.initialize(startup),"Plot setup runtime unavailable");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto report=files.filePath("values.txt"),saved=files.filePath("XY ü.script");
      const QString mission="BeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 60}; % preserve plot mission\nReport Values Sat.ElapsedSecs V A(1,2) Sat.EarthMJ2000Eq.X;\n";
      const QString base="Create Spacecraft Sat;\nCreate ForceModel FM;\nCreate Propagator Prop;\nProp.FM = FM;\n"
         "Create Variable V;\nV = 3;\nCreate String Label;\nLabel = 'not numeric';\nCreate Array A[1,2];\nA(1,1) = 2;\nA(1,2) = 7;\n"
         "Create XYPlot Graph;\nGraph.XVariable = Sat.ElapsedSecs;\nGraph.YVariables = {Sat.EarthMJ2000Eq.Y};\n"
         "Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 16;\n"+mission;
      editor->setPlainText(base); require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Plot baseline failed");
      QFile data(report); require(data.open(QIODevice::ReadOnly),"Reference plot report missing");
      const auto lines=QString::fromUtf8(data.readAll()).trimmed().split('\n'); const auto values=lines.last().simplified().split(' ');
      std::cout<<"REFERENCE: "<<lines.last().toStdString()<<'\n';
      require(values.size()==4 && std::abs(values[0].toDouble()-60)<1e-6 && values[1].toDouble()==3 && values[2].toDouble()==7,"Plot reference report wrong");
      const double finalX=values[3].toDouble(); require(window.buildScript(),"Plot baseline reset failed"); const auto source=editor->toPlainText();
      auto *plot=Moderator::Instance()->GetConfiguredObject("Graph"); QString applyError="Not applied"; std::exception_ptr failure;
      {
         QWidget owner; ResourceEditor panel(*plot,[&](const auto &changes) { applyError=window.applyResourceChanges("Graph",changes,source); return applyError; },&owner,source);
         auto *setup=panel.findChild<QPushButton *>("editXYPlot"); require(setup,"XY setup button missing");
         later(&panel,failure,[&] { auto *dialog=dynamic_cast<XYPlotDialog *>(panel.findChild<QDialog *>("xyPlotDialog")); dialog->findChild<QLineEdit *>("xyXVariable")->setText("V"); close(dialog,false); }); setup->click(); if (failure) std::rethrow_exception(failure);
         require(!panel.hasChanges() && editor->toPlainText()==source,"Plot Cancel leaked edits");
         later(&panel,failure,[&] {
            auto *dialog=dynamic_cast<XYPlotDialog *>(panel.findChild<QDialog *>("xyPlotDialog")); auto *x=dialog->findChild<QLineEdit *>("xyXVariable");
            x->setText("Label"); close(dialog); require(dialog->isVisible() && !dialog->findChild<QLabel *>("xyPlotError")->text().isEmpty(),"String accepted for plot X");
            x->clear(); close(dialog); require(dialog->isVisible(),"Shown plot accepted empty X"); x->setText("Sat.ElapsedSecs");
            later(dialog,failure,[&] {
               auto *picker=dynamic_cast<ReportParameterDialog *>(dialog->findChild<QDialog *>("reportParameterDialog")); auto *entry=picker->findChild<QComboBox *>("reportParameterEntry");
               require(entry->findText("Label")<0 && entry->findText("V")>=0 && entry->findText("A")>=0,"Plottable picker filtering wrong");
               entry->setEditText("A"); require(!picker->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Bare array accepted for X");
               picker->findChild<QSpinBox *>("reportArrayColumn")->setValue(1); picker->findChild<QPushButton *>("reportAddElement")->click(); close(picker);
            }); dialog->findChild<QPushButton *>("xySelectX")->click(); if (failure) std::rethrow_exception(failure);
            require(x->text()=="A(1,1)","X array element selection failed");
            later(dialog,failure,[&] {
               auto *picker=dynamic_cast<ReportParameterDialog *>(dialog->findChild<QDialog *>("reportParameterDialog"));
               picker->findChild<QComboBox *>("reportParameterEntry")->setEditText("V"); close(picker,false);
            }); dialog->findChild<QPushButton *>("xySelectX")->click(); if (failure) std::rethrow_exception(failure);
            require(x->text()=="A(1,1)","X picker Cancel changed selection");
            later(dialog,failure,[&] {
               auto *picker=dynamic_cast<ReportParameterDialog *>(dialog->findChild<QDialog *>("reportParameterDialog")); auto *list=picker->findChild<QListWidget *>("reportSelectedParameters"); list->clear();
               auto *entry=picker->findChild<QComboBox *>("reportParameterEntry"); auto *add=picker->findChild<QPushButton *>("reportAddParameter");
               entry->setEditText("Label"); add->click(); require(list->count()==0,"Y picker accepted string");
               entry->setEditText("V"); add->click(); add->click(); require(list->count()==1,"Y duplicate parameter accepted");
               entry->setEditText("A"); picker->findChild<QSpinBox *>("reportArrayColumn")->setValue(2); picker->findChild<QPushButton *>("reportAddElement")->click();
               require(list->count()==2 && list->item(1)->text()=="A(1,2)","Y array element selection failed");
               picker->findChild<QComboBox *>("reportPropertyObject")->setCurrentText("Sat");
               auto *property=picker->findChild<QComboBox *>("reportPropertyType"); require(property->findText("X")>=0 && property->findText("UTCGregorian")<0,"Plot property browser filtering wrong"); property->setCurrentText("X");
               picker->findChild<QComboBox *>("reportPropertyDependency")->setCurrentText("EarthMJ2000Eq");
               picker->findChild<QPushButton *>("reportUseReference")->click(); add->click(); require(list->count()==3 && list->item(2)->text()=="Sat.EarthMJ2000Eq.X","Y property/frame browsing failed"); close(picker);
            }); dialog->findChild<QPushButton *>("xySelectY")->click(); if (failure) std::rethrow_exception(failure);
            auto *ys=dialog->findChild<QListWidget *>("xyYVariables"); ys->setCurrentRow(2); dialog->findChild<QPushButton *>("xyYUp")->click(); dialog->findChild<QPushButton *>("xyYDown")->click();
            require(ys->item(2)->text()=="Sat.EarthMJ2000Eq.X","Y reordering failed");
            dialog->findChild<QCheckBox *>("xyShowGrid")->setChecked(false); dialog->findChild<QComboBox *>("xySolverIterations")->setCurrentText("None");
            require(dialog->findChild<QLabel *>("xyPlotError")->text().isEmpty(),"Corrected plot input retained stale validation error");
            if (!capture.isEmpty()) { dialog->resize(620,500); QApplication::processEvents(); require(dialog->grab().save(capture),"XY setup capture failed"); }
            close(dialog);
         }); setup->click(); if (failure) std::rethrow_exception(failure);
         require(panel.hasChanges() && plot->GetStringParameter("XVariable")=="Sat.ElapsedSecs" && plot->GetStringArrayParameter("YVariables").size()==1,"XY setup modified configured resource before Apply");
         later(&panel,failure,[&] { auto *dialog=dynamic_cast<XYPlotDialog *>(panel.findChild<QDialog *>("xyPlotDialog")); require(dialog->settings().value("XVariable")=="A(1,1)" && dialog->findChild<QListWidget *>("xyYVariables")->count()==3,"Pending XY settings lost when reopening"); close(dialog,false); }); setup->click(); if (failure) std::rethrow_exception(failure);
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(applyError.isEmpty(),qPrintable(applyError));
      }
      const auto changed=editor->toPlainText(); require(changed.endsWith(mission),"XY Apply altered mission comment/report"); editor->undo(); require(editor->toPlainText()==source,"XY Undo failed"); editor->redo(); require(editor->toPlainText()==changed,"XY Redo failed");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"GUI XY round trip/execution failed");
      auto model=window.plotReceiver()->model("Graph"); require(model && !model->grid && model->curves.size()==3,"XY grid/curve configuration did not reach viewer");
      int index=0;
      for (const auto &curve:model->curves) {
         require(!curve.points.empty(),"Configured XY curve has no samples");
         for (const auto &point:curve.points) {
            require(point.x==2,"XY X array reference changed value");
            if (index<2) require(point.y==(index==0 ? 3 : 7),"XY variable/array Y changed value");
         }
         if (index==2) require(std::abs(curve.points.back().y-finalX)<1e-8,"XY spacecraft Y differs from report"); ++index;
      }
      require(ReportParameterDialog::isPlottableReference("A(1,2)") && !ReportParameterDialog::isPlottableReference("A(1,3)") && !ReportParameterDialog::isPlottableReference("Label"),"Plottable reference bounds/type validation failed");
      const auto current=editor->toPlainText(); require(!window.applyResourceChanges("Graph",{{"YVariables","Label"}},current).isEmpty() && editor->toPlainText()==current,"Invalid plot Y did not roll back");
      require(!window.applyResourceChanges("Graph",{{"XVariable","Label"}},current).isEmpty() && editor->toPlainText()==current,"Invalid plot X did not roll back");
      require(!window.applyResourceChanges("Graph",{{"YVariables",""}},current).isEmpty() && editor->toPlainText()==current,"Shown plot accepted no Y parameters");
      require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Invalid plot reference recovery failed");
      require(window.applyResourceChanges("Graph",{{"ShowPlot","false"},{"YVariables",""}},editor->toPlainText()).isEmpty(),"Plot disable/clear failed");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed && !window.plotReceiver()->model("Graph"),"Disabled XY plot still opens after save/reopen");
      const auto disabled=editor->toPlainText(); require(!window.applyResourceChanges("Graph",{{"ShowPlot","true"}},disabled).isEmpty() && editor->toPlainText()==disabled,"Empty XY plot enabled without Y parameters");
      std::cout<<"PASS: XY grouped controls, plottable parameter/array browser, Cancel/pending Apply, validation/recovery, exact Undo/Redo/save/reopen, grid/visibility and numerical curve/report agreement\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
