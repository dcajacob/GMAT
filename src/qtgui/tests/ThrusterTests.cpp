#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ThrusterDialog.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QEventLoop>
#include <QRegularExpression>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void later(QWidget *owner,std::exception_ptr &failure,std::function<void()> action)
{
   QTimer::singleShot(0,owner,[owner,&failure,action] { try { action(); } catch (...) { failure=std::current_exception(); for (auto *dialog:owner->findChildren<QDialog *>()) dialog->reject(); } });
}
static void close(QDialog *dialog,bool accepted=true) { dialog->findChild<QDialogButtonBox *>()->button(accepted ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click(); }
static QTableWidgetItem *field(ResourceEditor &panel,const QString &name)
{
   auto *table=panel.findChild<QTableWidget *>(); for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==name) return table->item(row,1) ? table->item(row,1) : table->item(row,0); throw std::runtime_error("Thruster field missing");
}
static void value(ResourceEditor &panel,const QString &name,const QString &text)
{
   auto *item=field(panel,name); auto *table=item->tableWidget(); if (auto *combo=qobject_cast<QComboBox *>(table->cellWidget(item->row(),1))) combo->setCurrentText(text); else item->setText(text);
}
static QVector<QVector<double>> reportRows(const QString &path)
{
   QFile file(path); require(file.open(QIODevice::ReadOnly),"Thruster report missing"); QVector<QVector<double>> result;
   for (const auto &line:QString::fromUtf8(file.readAll()).trimmed().split('\n')) { QVector<double> row; for (const auto &part:line.trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)) { bool ok=false; auto number=part.toDouble(&ok); if (!ok) { row.clear(); break; } row.append(number); } if (!row.isEmpty()) result.append(row); } return result;
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setAttribute(Qt::AA_DontUseNativeDialogs); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtThrusters");
   if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Thruster fixtures unavailable"); MainWindow window; window.show(); require(window.initialize(startup),"Thruster runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      for (const auto &mode:QStringList{"Chemical","ConstantThrustAndIsp","FixedEfficiency","ThrustMassPolynomial","BelowMinimum"}) {
         const bool chemical=mode=="Chemical",polynomial=mode=="ThrustMassPolynomial"; const auto type=chemical ? "ChemicalThruster" : "ElectricThruster",tank=chemical ? "ChemicalTank" : "ElectricTank";
         const auto report=files.filePath(mode+".txt"),saved=files.filePath(mode+" ü.script");
         const QString mission="BeginMissionSequence;\nBeginFiniteBurn Burn(Sat);\nPropagate Prop(Sat) {Sat.ElapsedSecs = 10}; % keep thruster burn\nReport Values Sat.FuelA.FuelMass Sat.FuelB.FuelMass Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.Engine.ThrustMagnitude Sat.Engine.Isp Sat.Engine.MassFlowRate;\nEndFiniteBurn Burn(Sat);\nPropagate Prop(Sat) {Sat.ElapsedSecs = 20};\nReport Values Sat.FuelA.FuelMass Sat.FuelB.FuelMass;\n";
         const QString base="Create Spacecraft Sat;\nSat.DryMass = 1000;\nCreate "+QString(tank)+" FuelA FuelB;\nCreate "+(chemical ? QString("ElectricTank") : QString("ChemicalTank"))+" WrongFuel;\nFuelA.FuelMass = 100;\nFuelB.FuelMass = 100;\nCreate "+type+" Engine;\nEngine.CoordinateSystem = EarthMJ2000Eq;\nEngine.DecrementMass = false;\nSat.Tanks = {FuelA, FuelB};\nSat.Thrusters = {Engine};\n"
            "Create NuclearPowerSystem Power;\nPower.InitialMaxPower = "+(mode=="BelowMinimum" ? QString("0.5") : QString("10"))+";\nPower.AnnualDecayRate = 0;\nPower.Margin = 0;\nPower.BusCoeff1 = 0;\nPower.BusCoeff2 = 0;\nPower.BusCoeff3 = 0;\n"
            "Create FiniteBurn Burn;\nBurn.Thrusters = {Engine};\nCreate ForceModel FM;\nCreate Propagator Prop;\nProp.FM = FM;\nProp.InitialStepSize = 1;\nProp.MaxStep = 1;\n"
            "Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 16;\n";
         QMap<QString,QString> desired={{"CoordinateSystem","Local"},{"Origin","Mars"},{"Axes","MJ2000Eq"},{"ThrustDirection1","0"},{"ThrustDirection2","1"},{"ThrustDirection3","0"},{"DutyCycle","0.5"},{"ThrustScaleFactor","1.25"},{"GravitationalAccel","10"}};
         QMap<QString,QString> coefficients;
         if (chemical) coefficients={{"C1","10"},{"K1","2000"}};
         else {
            desired.insert("ThrustModel",mode=="BelowMinimum" ? "ConstantThrustAndIsp" : mode); desired.insert("MinimumUsablePower","1"); desired.insert("MaximumUsablePower","4"); desired.insert("FixedEfficiency","0.4"); desired.insert("Isp","2500"); desired.insert("ConstantThrust","0.5");
            if (polynomial) for (int i=1;i<=5;++i) { coefficients.insert("ThrustCoeff"+QString::number(i),i==1 ? "300" : i==2 ? "200" : "0"); coefficients.insert("MassFlowCoeff"+QString::number(i),i==1 ? "2" : i==2 ? "1" : "0"); }
         }
         QString reference=base+"Sat.PowerSystem = Power;\nEngine.Tank = {FuelB, FuelA};\nEngine.MixRatio = [3 1];\nEngine.DecrementMass = true;\n";
         for (auto it=desired.cbegin();it!=desired.cend();++it) reference+="Engine."+it.key()+" = "+it.value()+";\n";
         for (auto it=coefficients.cbegin();it!=coefficients.cend();++it) reference+="Engine."+it.key()+" = "+it.value()+";\n";
         editor->setPlainText(reference+mission); require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Script-configured thrust reference failed"); auto expected=reportRows(report); require(expected.size()==2 && expected[0].size()==8,"Reference thrust report incomplete");
         editor->setPlainText(base+mission); require(window.buildScript(),"Tankless thruster edit fixture failed"); const auto source=editor->toPlainText(); QString error="Not applied"; std::exception_ptr failure;
         {
            QWidget owner; auto *engine=Moderator::Instance()->GetConfiguredObject("Engine"); ResourceEditor panel(*engine,[&](const auto &changes) { error=window.applyResourceChanges("Engine",changes,source); return error; },&owner,source);
            require(field(panel,"MixRatio") && field(panel,"Axes") && field(panel,"Origin"),"Conditionally readonly thruster controls missing");
            auto *setup=panel.findChild<QPushButton *>("editThruster"); require(setup,"Thruster setup missing");
            later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("thrusterDialog"); dialog->findChild<QLineEdit *>("thruster_DutyCycle")->setText("0.2"); close(dialog,false); }); setup->click(); if (failure) std::rethrow_exception(failure); require(!panel.hasChanges(),"Thruster Cancel changed pending settings");
            later(&panel,failure,[&] {
               auto *dialog=panel.findChild<QDialog *>("thrusterDialog"); auto *frame=dialog->findChild<QComboBox *>("thruster_CoordinateSystem"),*axes=dialog->findChild<QComboBox *>("thruster_Axes"),*origin=dialog->findChild<QComboBox *>("thruster_Origin");
               require(frame->findText("Local")>=0 && !axes->isEnabled() && !origin->isEnabled(),"Nonlocal direction controls wrong"); frame->setCurrentText("Local"); require(axes->isEnabled() && origin->isEnabled(),"Local VNB origin/axes not enabled"); origin->setCurrentText("Mars"); axes->setCurrentText("SpacecraftBody"); require(!origin->isEnabled(),"Body axes origin should be inactive"); axes->setCurrentText("MJ2000Eq"); require(!origin->isEnabled(),"MJ2000Eq origin should be inactive"); frame->setCurrentText("EarthFixed"); frame->setCurrentText("Local"); require(axes->currentText()=="MJ2000Eq" && origin->currentText()=="Mars","Local selector choices lost while changing frames");
               for (auto it=desired.cbegin();it!=desired.cend();++it) {
                  if (auto *combo=dialog->findChild<QComboBox *>("thruster_"+it.key())) combo->setCurrentText(it.value()); else dialog->findChild<QLineEdit *>("thruster_"+it.key())->setText(it.value());
               }
               auto *duty=dialog->findChild<QLineEdit *>("thruster_DutyCycle"); duty->setText("2"); close(dialog); require(dialog->isVisible(),"Out-of-range duty accepted"); duty->setText("0.5");
               if (!chemical) {
                  auto *model=dialog->findChild<QComboBox *>("thruster_ThrustModel");
                  model->setCurrentText("FixedEfficiency"); require(dialog->findChild<QLineEdit *>("thruster_FixedEfficiency")->isEnabled() && !dialog->findChild<QLineEdit *>("thruster_ConstantThrust")->isEnabled(),"Fixed-efficiency dependencies wrong");
                  model->setCurrentText("ThrustMassPolynomial"); require(!dialog->findChild<QLineEdit *>("thruster_Isp")->isEnabled(),"Polynomial Isp should be computed"); model->setCurrentText(desired.value("ThrustModel"));
                  auto *max=dialog->findChild<QLineEdit *>("thruster_MaximumUsablePower"); max->setText("0.5"); close(dialog); require(dialog->isVisible(),"Reversed power limits accepted"); max->setText("4"); require(dialog->findChild<QLineEdit *>("thruster_Isp")->text()=="2500","Inactive electric setting lost while switching model");
               }
               dialog->resize(620,420); QEventLoop loop; QTimer::singleShot(80,&loop,&QEventLoop::quit); loop.exec(); require(dialog->findChild<QScrollArea *>("thrusterScroll")->verticalScrollBar()->maximum()>0,"Compact thruster setup cannot scroll"); auto *ok=dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok); require(dialog->rect().contains(ok->mapTo(dialog,ok->rect().bottomRight())),"Thruster OK hidden in compact dialog");
               if (!capture.isEmpty() && mode=="ConstantThrustAndIsp") { dialog->resize(620,720); app.processEvents(); require(dialog->grab().save(capture),"Thruster capture failed"); }
               close(dialog); require(!dialog->isVisible(),qPrintable(dialog->findChild<QLabel *>("thrusterError")->text()));
            }); setup->click(); if (failure) std::rethrow_exception(failure);
            require(panel.hasChanges() && engine->GetStringParameter("CoordinateSystem")=="EarthMJ2000Eq" && engine->GetRealParameter("DutyCycle")==1,"Thruster dialog altered engine before Apply");
            later(&panel,failure,[&] { auto *dialog=dynamic_cast<ThrusterDialog *>(panel.findChild<QDialog *>("thrusterDialog")); require(dialog->settings().value("Axes")=="MJ2000Eq" && dialog->settings().value("DutyCycle")=="0.5","Pending thruster settings lost on reopen"); close(dialog,false); }); setup->click(); if (failure) std::rethrow_exception(failure);
            auto *tanks=panel.findChild<QPushButton *>("thrusterTankMixtures");
            later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("tankMixtureDialog"); require(dialog,"Tankless thruster cannot open mixture editor"); auto *available=dialog->findChild<QComboBox *>("tankMixtureAvailable"); require(available->findText("WrongFuel")<0,"Wrong tank type offered by thruster picker"); available->setCurrentText("FuelA"); dialog->findChild<QPushButton *>("tankMixtureAdd")->click(); available->setCurrentText("FuelB"); dialog->findChild<QPushButton *>("tankMixtureAdd")->click(); auto *grid=dialog->findChild<QTableWidget *>("tankMixtureTable"); grid->item(1,1)->setText("3"); grid->selectRow(1); dialog->findChild<QPushButton *>("tankMixtureUp")->click(); require(grid->item(0,0)->text()=="FuelB" && grid->item(0,1)->text()=="3","Tank reordering lost mixture ratio"); close(dialog); }); tanks->click(); if (failure) std::rethrow_exception(failure);
            value(panel,"DecrementMass","true");
            if (!coefficients.isEmpty()) {
               auto *button=panel.findChild<QPushButton *>("thrusterCoefficients"); require(button->isEnabled(),"Polynomial coefficient editor disabled");
               later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("thrusterCoefficientDialog"); for (auto *grid:dialog->findChildren<QTableWidget *>()) for (int row=0;row<grid->rowCount();++row) if (coefficients.contains(grid->item(row,0)->text())) grid->item(row,1)->setText(coefficients.value(grid->item(row,0)->text())); close(dialog); }); button->click(); if (failure) std::rethrow_exception(failure);
            } else require(!panel.findChild<QPushButton *>("thrusterCoefficients")->isEnabled(),"Nonpolynomial model enables polynomial editor");
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         }
         const auto changed=editor->toPlainText(); require(changed.endsWith(mission),"Thruster Apply altered mission syntax/comments"); editor->undo(); require(editor->toPlainText()==source,"Thruster Undo not exact"); editor->redo(); require(editor->toPlainText()==changed,"Thruster Redo not exact");
         if (!chemical) {
            require(window.runMission()==MainWindow::RunResult::Failed,"Electric burn without attached power unexpectedly succeeded");
            const auto noPower=editor->toPlainText(); QWidget owner; auto *spacecraft=Moderator::Instance()->GetConfiguredObject("Sat");
            ResourceEditor panel(*spacecraft,[&](const auto &changes) { error=window.applyResourceChanges("Sat",changes,noPower); return error; },&owner,noPower);
            auto *power=panel.findChild<QComboBox *>("spacecraftPowerSystem"); require(power && power->findText("Power")>=0 && spacecraft->GetStringParameter("PowerSystem").empty(),"Power attachment fixture/picker incorrect"); power->setCurrentText("Power"); require(spacecraft->GetStringParameter("PowerSystem").empty(),"Power selection mutated spacecraft before Apply");
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         }
         require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"GUI-configured thruster round trip/execution failed"); const auto actual=reportRows(report); require(actual.size()==expected.size(),"GUI thrust report missing rows");
         for (int row=0;row<actual.size();++row) { require(actual[row].size()==expected[row].size(),"GUI thrust report missing fields"); for (int col=0;col<actual[row].size();++col) require(std::abs(actual[row][col]-expected[row][col])<1e-8,"GUI thruster output differs from script reference"); }
         double loss=chemical ? .0025 : mode=="FixedEfficiency" ? .0000256 : polynomial ? .00003 : mode=="BelowMinimum" ? 0 : .0001;
         require(std::abs(actual[0][0]-(100-loss*.25))<1e-8 && std::abs(actual[0][1]-(100-loss*.75))<1e-8,"Fuel use disagrees with constant-rate analytic check"); require(actual[1][0]==actual[0][0] && actual[1][1]==actual[0][1],"Fuel continued changing after EndFiniteBurn");
         const auto current=editor->toPlainText();
         QList<QMap<QString,QString>> bad={{{"CoordinateSystem","Sat"}},{{"CoordinateSystem","Local"},{"Origin","Sat"}},{{"ThrustDirection1","0"},{"ThrustDirection2","0"},{"ThrustDirection3","0"}},{{"DutyCycle","-1"}},{{"ThrustScaleFactor","-1"}},{{"GravitationalAccel","0"}},{{"Tank","Sat"}},{{"Tank","WrongFuel"}}};
         if (!chemical) bad.append({{"MinimumUsablePower","5"},{"MaximumUsablePower","4"}});
         for (const auto &changes:bad) require(!window.applyResourceChanges("Engine",changes,current).isEmpty() && editor->toPlainText()==current,"Invalid thruster edit did not roll back");
         require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Thruster invalid-edit recovery failed");
         {
            const auto before=editor->toPlainText(); QWidget owner;
            ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("Engine"),[&](const auto &changes) { error=window.applyResourceChanges("Engine",changes,before); return error; },&owner,before);
            value(panel,"DecrementMass","false"); panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         }
         require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Burn with mass decrement off failed");
         const auto noMass=reportRows(report); require(noMass.size()==2 && noMass[0].size()==8 && noMass[0][0]==100 && noMass[0][1]==100 && noMass[0][7]==0 && noMass[1][0]==100 && noMass[1][1]==100,"Turning off mass decrement still consumed fuel");
         {
            const auto before=editor->toPlainText(); QWidget owner;
            ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("Engine"),[&](const auto &changes) { error=window.applyResourceChanges("Engine",changes,before); return error; },&owner,before);
            later(&panel,failure,[&] {
               auto *dialog=panel.findChild<QDialog *>("tankMixtureDialog"); auto *grid=dialog->findChild<QTableWidget *>("tankMixtureTable");
               while (grid->rowCount()) { grid->selectRow(0); dialog->findChild<QPushButton *>("tankMixtureRemove")->click(); }
               close(dialog);
            }); panel.findChild<QPushButton *>("thrusterTankMixtures")->click(); if (failure) std::rethrow_exception(failure);
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         }
         require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"Cleared tanks failed save/reopen/build");
         require(Moderator::Instance()->GetConfiguredObject("Engine")->GetStringArrayParameter("Tank").empty(),"Clearing tanks left a hidden tank attachment");
         require(window.runMission()==MainWindow::RunResult::Failed,"Tankless active burn unexpectedly succeeded");
         error=window.applyResourceChanges("Engine",{{"Tank","FuelB, FuelA"},{"MixRatio","3 1"},{"DecrementMass","true"}},editor->toPlainText()); require(error.isEmpty(),qPrintable(error));
         require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Restoring tanks after failed burn did not recover");
         const auto recovered=reportRows(report); require(recovered.size()==expected.size(),"Restored thruster report missing rows");
         for (int row=0;row<recovered.size();++row) { require(recovered[row].size()==expected[row].size(),"Restored thruster report missing fields"); for (int col=0;col<recovered[row].size();++col) require(std::abs(recovered[row][col]-expected[row][col])<1e-8,"Restored thruster changed numerical results"); }
         std::cout<<"PASS: "<<mode.toStdString()<<" direction/model dependencies, first-tank paired mixtures, coefficients, pending/Cancel, Undo/Redo/Unicode save/reopen, power limits, analytic fuel consumption, report/state invariance, coasting, mass-decrement off, tank clear/restore and invalid-edit recovery\n";
      }
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
