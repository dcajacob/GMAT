#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "BurnDialog.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QPlainTextEdit>
#include <QDockWidget>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QTableWidget>
#include <QListWidget>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QDir>
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
static void close(QDialog *dialog,bool accept=true) { dialog->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click(); }
static QVector<QVector<double>> rows(const QString &path)
{
   QFile file(path); require(file.open(QIODevice::ReadOnly),"Burn report missing"); QVector<QVector<double>> result;
   for (const auto &line:QString::fromUtf8(file.readAll()).trimmed().split('\n')) {
      QVector<double> row; for (const auto &part:line.trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)) { bool ok=false; const double value=part.toDouble(&ok); if (!ok) { row.clear(); break; } row.append(value); }
      if (!row.isEmpty()) result.append(row);
   } return result;
}
static void equivalent(const QVector<QVector<double>> &actual,const QVector<QVector<double>> &expected)
{
   require(actual.size()==expected.size(),"Burn report row count changed");
   for (int r=0;r<actual.size();++r) { require(actual[r].size()==expected[r].size(),"Burn report fields changed"); for (int c=0;c<actual[r].size();++c) require(std::abs(actual[r][c]-expected[r][c])<1e-8,"GUI burn results differ from script reference"); }
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setAttribute(Qt::AA_DontUseNativeDialogs); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtBurns");
   if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Burn fixtures unavailable"); MainWindow window; window.show(); require(window.initialize(startup),"Burn runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString state="Create Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.5;\nSat.VZ = 1;\nSat.DryMass = 1000;\n";
      for (const auto &mode:QStringList{"Inertial","MJ2000Eq","VNB","LVLH","SpacecraftBody","EarthFixed","Zero"}) {
         const bool local=mode!="Inertial" && mode!="EarthFixed",zero=mode=="Zero";
         const auto report=files.filePath(mode+".txt"),saved=files.filePath(mode+" ü.script");
         const QString reportCommand="Report Values Sat.Fuel.FuelMass Sat.DecoyFuel.FuelMass Sat.TotalMass Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ;\n";
         const QString mission="BeginMissionSequence;\n"+reportCommand+"Maneuver 'Forward kick' Kick(Sat); % keep forward\n"+reportCommand+"Maneuver 'Reverse kick' BackProp Kick(Sat); % keep reverse\n"+reportCommand;
         const QString base=state+"Create ChemicalTank Fuel DecoyFuel Detached;\nFuel.FuelMass = 100;\nDecoyFuel.FuelMass = 100;\nSat.Tanks = {Fuel, DecoyFuel};\nCreate ImpulsiveBurn Kick;\nKick.CoordinateSystem = EarthMJ2000Eq;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 16;\n";
         const QMap<QString,QString> desired={{"CoordinateSystem",local ? "Local" : mode=="EarthFixed" ? "EarthFixed" : "EarthMJ2000Eq"},{"Axes",zero ? "MJ2000Eq" : mode},{"Origin","Earth"},{"Element1",zero ? "0" : "0.01"},{"Element2",zero ? "0" : "0.02"},{"Element3",zero ? "0" : "-0.03"},{"DecrementMass","true"},{"Tank","Fuel"},{"Isp","400"},{"GravitationalAccel","10"}};
         QString reference=base+"Kick.CoordinateSystem = "+desired.value("CoordinateSystem")+";\n";
         for (auto it=desired.cbegin();it!=desired.cend();++it) if (it.key()!="CoordinateSystem" && (local || (it.key()!="Axes" && it.key()!="Origin"))) reference+="Kick."+it.key()+" = "+(it.key()=="Tank" ? "{Fuel}" : it.value())+";\n";
         editor->setPlainText(reference+mission); require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,qPrintable(window.findChild<QDockWidget *>("messages")->findChild<QPlainTextEdit *>()->toPlainText())); const auto expected=rows(report); require(expected.size()==3 && expected[0].size()==9,"Reference impulsive report incomplete");
         editor->setPlainText(base+mission); require(window.buildScript(),"Tankless impulsive edit fixture failed"); const auto source=editor->toPlainText(); QString error="Not applied"; std::exception_ptr failure;
         {
            QWidget owner; auto *burn=Moderator::Instance()->GetConfiguredObject("Kick"); ResourceEditor panel(*burn,[&](const auto &changes) { error=window.applyResourceChanges("Kick",changes,source); return error; },&owner,source);
            auto *setup=panel.findChild<QPushButton *>("editBurn"); require(setup,"Impulsive burn setup missing");
            later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("burnDialog"); dialog->findChild<QLineEdit *>("burn_Element1")->setText("0.5"); close(dialog,false); }); setup->click(); if (failure) std::rethrow_exception(failure); require(!panel.hasChanges(),"Impulsive Cancel altered pending values");
            later(&panel,failure,[&] {
               auto *dialog=panel.findChild<QDialog *>("burnDialog"); auto *frame=dialog->findChild<QComboBox *>("burn_CoordinateSystem"),*axes=dialog->findChild<QComboBox *>("burn_Axes"),*origin=dialog->findChild<QComboBox *>("burn_Origin");
               require(!axes->isEnabled() && !origin->isEnabled() && frame->findText("Local")>=0,"Foreign-frame impulsive dependencies wrong"); frame->setCurrentText("Local"); axes->setCurrentText("LVLH"); require(axes->isEnabled() && origin->isEnabled(),"Local burn origin/axes unavailable"); axes->setCurrentText("SpacecraftBody"); require(!origin->isEnabled(),"Body-axis burn origin should be inactive"); frame->setCurrentText("EarthFixed"); frame->setCurrentText("Local"); require(axes->currentText()=="SpacecraftBody","Pending local axes lost switching frames");
               auto *mass=dialog->findChild<QCheckBox *>("burn_DecrementMass"); require(!mass->isChecked() && !dialog->findChild<QLineEdit *>("burn_Isp")->isEnabled(),"Mass-off burn Isp should be inactive"); mass->setChecked(true); close(dialog); require(dialog->isVisible() && !dialog->findChild<QLabel *>("burnError")->text().isEmpty(),"Mass depletion without tank accepted");
               for (auto it=desired.cbegin();it!=desired.cend();++it) {
                  if (it.key()=="DecrementMass") continue;
                  if (auto *combo=dialog->findChild<QComboBox *>("burn_"+it.key())) { if (it.key()=="Tank") combo->setCurrentIndex(combo->findData(it.value())); else if (local || (it.key()!="Axes" && it.key()!="Origin")) combo->setCurrentText(it.value()); }
                  else dialog->findChild<QLineEdit *>("burn_"+it.key())->setText(it.value());
               }
               auto *tank=dialog->findChild<QComboBox *>("burn_Tank"); require(tank->findData("Fuel")>=0 && tank->findData("Sat")<0,"Impulsive tank selector wrong type");
               auto *isp=dialog->findChild<QLineEdit *>("burn_Isp"); isp->setText("0"); close(dialog); require(dialog->isVisible(),"Zero Isp accepted"); isp->setText("400");
               mass->setChecked(false); mass->setChecked(true); require(isp->text()=="400" && tank->currentData().toString()=="Fuel","Mass toggle lost pending values");
               dialog->resize(600,400); QEventLoop loop; QTimer::singleShot(80,&loop,&QEventLoop::quit); loop.exec(); require(dialog->findChild<QScrollArea *>("burnScroll")->verticalScrollBar()->maximum()>0,"Compact burn setup cannot scroll"); auto *ok=dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok); require(dialog->rect().contains(ok->mapTo(dialog,ok->rect().bottomRight())),"Burn action buttons hidden");
               if (!capture.isEmpty() && mode=="VNB") { dialog->resize(600,650); app.processEvents(); require(dialog->grab().save(capture),"Burn dialog capture failed"); }
               close(dialog); require(!dialog->isVisible(),qPrintable(dialog->findChild<QLabel *>("burnError")->text()));
            }); setup->click(); if (failure) std::rethrow_exception(failure);
            require(panel.hasChanges() && burn->GetStringArrayParameter("Tank").empty() && !burn->GetBooleanParameter("DecrementMass"),"Impulsive dialog mutated engine before Apply");
            later(&panel,failure,[&] { auto *dialog=dynamic_cast<BurnDialog *>(panel.findChild<QDialog *>("burnDialog")); require(dialog->settings().value("Tank")=="Fuel" && dialog->settings().value("Isp")=="400","Impulsive pending settings lost on reopen"); close(dialog,false); }); setup->click(); if (failure) std::rethrow_exception(failure);
            panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         }
         const auto changed=editor->toPlainText(); require(changed.endsWith(mission),"Burn Apply changed labeled mission/comments"); editor->undo(); require(editor->toPlainText()==source,"Burn Undo not exact"); editor->redo(); require(editor->toPlainText()==changed,"Burn Redo not exact");
         require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"GUI impulsive save/reopen execution failed"); const auto actual=rows(report); equivalent(actual,expected);
         const double norm=zero ? 0 : std::sqrt(.01*.01+.02*.02+.03*.03),fuel=100+1200*std::expm1(-norm*1000/(400*10));
         require(std::abs(actual[1][0]-fuel)<1e-9 && actual[1][1]==100,"Impulsive fuel loss disagrees with rocket equation");
         for (int c=0;c<9;++c) require(std::abs(actual[2][c]-actual[0][c])<1e-9,"Backward burn did not restore state/fuel");
         if (mode=="Inertial" || mode=="MJ2000Eq") require(std::abs(actual[1][6]-.01)<1e-10 && std::abs(actual[1][7]-7.52)<1e-10 && std::abs(actual[1][8]-.97)<1e-10,"Inertial delta-V components wrong");
         if (mode=="VNB") { const auto speed=std::hypot(7.5,1.0); require(std::abs(actual[1][6]+.03)<1e-10 && std::abs(actual[1][7]-7.5-(7.5*.01-.02)/speed)<1e-10 && std::abs(actual[1][8]-1-(.01+7.5*.02)/speed)<1e-10,"VNB delta-V transform disagrees with analytic basis"); }
         if (mode=="LVLH") { const auto speed=std::hypot(7.5,1.0); require(std::abs(actual[1][6]-.01)<1e-10 && std::abs(actual[1][7]-7.5-(7.5*.02+.03)/speed)<1e-10 && std::abs(actual[1][8]-1-(.02-7.5*.03)/speed)<1e-10,"LVLH delta-V transform disagrees with analytic basis"); }
         const auto current=editor->toPlainText();
         for (const auto &bad:QList<QMap<QString,QString>>{{{"Tank",""},{"DecrementMass","true"}},{{"Tank","Fuel, DecoyFuel"},{"DecrementMass","true"}},{{"Tank","Sat"}},{{"Isp","0"}},{{"GravitationalAccel","0"}},{{"Element1","NaN"}},{{"CoordinateSystem","Sat"}},{{"CoordinateSystem","Local"},{"Origin","Sat"}}}) require(!window.applyResourceChanges("Kick",bad,current).isEmpty() && editor->toPlainText()==current,"Invalid impulsive edit did not roll back");
         error=window.applyResourceChanges("Kick",{{"Tank","Detached"}},current); require(error.isEmpty(),qPrintable(error)); require(window.runMission()==MainWindow::RunResult::Failed,"Unattached burn tank unexpectedly succeeded");
         error=window.applyResourceChanges("Kick",{{"Tank","Fuel"}},editor->toPlainText()); require(error.isEmpty(),qPrintable(error)); require(window.runMission()==MainWindow::RunResult::Completed,"Restoring attached burn tank failed to recover"); equivalent(rows(report),expected);
         {
            const auto before=editor->toPlainText(); QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("Kick"),[&](const auto &changes) { error=window.applyResourceChanges("Kick",changes,before); return error; },&owner,before);
            later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("burnDialog"); dialog->findChild<QCheckBox *>("burn_DecrementMass")->setChecked(false); dialog->findChild<QComboBox *>("burn_Tank")->setCurrentIndex(0); close(dialog); }); panel.findChild<QPushButton *>("editBurn")->click(); if (failure) std::rethrow_exception(failure); panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         }
         require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Mass-off tank clear failed round trip/execution"); const auto noMass=rows(report); require(noMass.size()==3 && noMass[1][0]==100 && noMass[1][1]==100 && noMass[1][2]==1200 && Moderator::Instance()->GetConfiguredObject("Kick")->GetStringArrayParameter("Tank").empty(),"Mass-off tank clear lost its intended behavior");
         std::cout<<"PASS: impulsive "<<mode.toStdString()<<" grouped dependencies, first tank, pending/Cancel, Undo/Redo/Unicode save/reopen, script-reference state, analytic fuel and frame checks, backward restoration, invalid edits, unattached-tank failure/recovery, mass-off/clear\n";
      }
      {
         const auto saved=files.filePath("preserved tanks ü.script");
         editor->setPlainText(state+"Create ChemicalTank Fuel DecoyFuel;\nSat.Tanks = {Fuel, DecoyFuel};\nCreate ImpulsiveBurn Kick;\nKick.Tank = {DecoyFuel, Fuel};\nKick.DecrementMass = false;\nBeginMissionSequence;\nManeuver Kick(Sat); % retained multiple tanks\n");
         require(window.buildScript(),"Imported mass-off multi-tank burn failed to build"); const auto source=editor->toPlainText(); QString error="Not applied"; std::exception_ptr failure;
         {
            QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("Kick"),[&](const auto &changes) { error=window.applyResourceChanges("Kick",changes,source); return error; },&owner,source);
            later(&panel,failure,[&] {
               auto *dialog=panel.findChild<QDialog *>("burnDialog"); auto *tank=dialog->findChild<QComboBox *>("burn_Tank"); require(tank->currentData().toString()=="DecoyFuel, Fuel","Imported multi-tank list collapsed on opening burn setup");
               dialog->findChild<QLineEdit *>("burn_Element1")->setText("0.02"); close(dialog);
            }); panel.findChild<QPushButton *>("editBurn")->click(); if (failure) std::rethrow_exception(failure); panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         }
         require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Mass-off imported tank list failed Apply/save/reopen/execution");
         require(Moderator::Instance()->GetConfiguredObject("Kick")->GetStringArrayParameter("Tank")==StringArray{"DecoyFuel","Fuel"},"Vector edit reordered or truncated imported tanks");
         std::cout<<"PASS: imported mass-off tank list preserved through vector edit, Apply, Unicode save/reopen and execution\n";
      }
      {
         const auto report=files.filePath("finite.txt"),saved=files.filePath("finite ü.script");
         const QString reportCommand="Report Values Sat.FuelA.FuelMass Sat.FuelB.FuelMass Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z;\n";
         const QString mission="BeginMissionSequence;\nBeginFiniteBurn 'Begin two engines' Continuous(Sat); % keep start\nPropagate Prop(Sat) {Sat.ElapsedSecs = 10};\n"+reportCommand+"EndFiniteBurn 'End two engines' Continuous(Sat); % keep stop\nPropagate Prop(Sat) {Sat.ElapsedSecs = 20};\n"+reportCommand;
         QString base=state+"Create ChemicalTank FuelA FuelB;\nFuelA.FuelMass = 100;\nFuelB.FuelMass = 100;\nSat.Tanks = {FuelA, FuelB};\nCreate ChemicalThruster EngineA EngineB Unattached;\n";
         for (const auto &engine:QStringList{"EngineA","EngineB"}) base+=engine+".CoordinateSystem = EarthMJ2000Eq;\n"+engine+".Tank = {"+(engine=="EngineA" ? "FuelA" : "FuelB")+"};\n"+engine+".DecrementMass = true;\n"+engine+".DutyCycle = 0.5;\n"+engine+".GravitationalAccel = 10;\n"+engine+".C1 = "+(engine=="EngineA" ? "4" : "6")+";\n"+engine+".K1 = 1000;\n";
         base+="Sat.Thrusters = {EngineA, EngineB};\nCreate FiniteBurn Continuous;\nCreate ForceModel FM;\nCreate Propagator Prop;\nProp.FM = FM;\nProp.InitialStepSize = 1;\nProp.MaxStep = 1;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 16;\n";
         editor->setPlainText(base+"Continuous.Thrusters = {EngineB, EngineA};\n"+mission); require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Finite burn reference failed"); const auto expected=rows(report);
         editor->setPlainText(base+"Continuous.Thrusters = {EngineA};\n"+mission); require(window.buildScript(),"Finite burn edit fixture failed"); const auto source=editor->toPlainText(); QString error="Not applied"; std::exception_ptr failure;
         auto select=[&](ResourceEditor &panel,const QStringList &names,bool accept=true,bool exercise=false) {
            later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("resourceSelectionDialog"); auto *list=dialog->findChild<QListWidget *>("resourceSelectionList"); require(list && list->findItems("Sat",Qt::MatchExactly).empty() && list->findItems("Unattached",Qt::MatchExactly).size()==1,"Finite burn picker wrong resource types");
               if (exercise) { dialog->findChild<QPushButton *>("resourceSelectAll")->click(); for (int i=0;i<list->count();++i) require(list->item(i)->checkState()==Qt::Checked,"Finite select-all missed thruster"); }
               dialog->findChild<QPushButton *>("resourceClearSelection")->click(); for (int i=0;i<list->count();++i) require(list->item(i)->checkState()==Qt::Unchecked,"Finite clear-all retained thruster");
               for (int i=0;i<names.size();++i) { const auto found=list->findItems(names[i],Qt::MatchExactly); require(found.size()==1,"Finite chosen thruster missing"); auto *item=list->takeItem(list->row(found.first())); list->insertItem(i,item); item->setCheckState(Qt::Checked); }
               close(dialog,accept);
            }); panel.findChild<QPushButton *>("chooseProperty_Thrusters")->click(); if (failure) std::rethrow_exception(failure);
         };
         {
            QWidget owner; auto *burn=Moderator::Instance()->GetConfiguredObject("Continuous"); ResourceEditor panel(*burn,[&](const auto &changes) { error=window.applyResourceChanges("Continuous",changes,source); return error; },&owner,source);
            select(panel,{"EngineB"},false); require(!panel.hasChanges(),"Finite selection Cancel altered pending values"); select(panel,{"EngineB","EngineA"},true,true); require(panel.hasChanges() && burn->GetStringArrayParameter("Thrusters").size()==1,"Finite selection altered engine before Apply"); select(panel,{"EngineB","EngineA"}); panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         }
         const auto changed=editor->toPlainText(); require(changed.endsWith(mission),"Finite Apply changed labeled mission/comments"); editor->undo(); require(editor->toPlainText()==source,"Finite Undo not exact"); editor->redo(); require(editor->toPlainText()==changed,"Finite Redo not exact");
         require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Finite selection round trip failed"); const auto actual=rows(report); equivalent(actual,expected); require(actual.size()==2 && std::abs(actual[0][0]-99.998)<1e-9 && std::abs(actual[0][1]-99.997)<1e-9 && actual[1][0]==actual[0][0] && actual[1][1]==actual[0][1],"Two-thruster analytic fuel/coast check failed");
         require(Moderator::Instance()->GetConfiguredObject("Continuous")->GetStringArrayParameter("Thrusters")==StringArray{"EngineB","EngineA"},"Finite selection order changed during save/reopen");
         const auto current=editor->toPlainText(); for (const auto &name:QStringList{"Sat","Missing","EngineA, EngineA"}) require(!window.applyResourceChanges("Continuous",{{"Thrusters",name}},current).isEmpty() && editor->toPlainText()==current,"Invalid finite thruster edit failed rollback");
         error=window.applyResourceChanges("Continuous",{{"Thrusters","Unattached"}},current); require(error.isEmpty(),qPrintable(error)); require(window.runMission()==MainWindow::RunResult::Failed,"Unattached finite thruster unexpectedly ran"); error=window.applyResourceChanges("Continuous",{{"Thrusters","EngineB, EngineA"}},editor->toPlainText()); require(error.isEmpty(),qPrintable(error)); require(window.runMission()==MainWindow::RunResult::Completed,"Finite thruster restore failed recovery"); equivalent(rows(report),expected);
         {
            const auto before=editor->toPlainText(); QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("Continuous"),[&](const auto &changes) { error=window.applyResourceChanges("Continuous",changes,before); return error; },&owner,before); select(panel,{}); panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         }
         require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"Empty finite burn failed save/reopen/build");
         require(Moderator::Instance()->GetConfiguredObject("Continuous")->GetStringArrayParameter("Thrusters").empty(),"Finite clear-all left hidden thrusters");
         require(window.runMission()==MainWindow::RunResult::Failed,"Engine unexpectedly accepted an empty active finite burn"); app.processEvents();
         require(window.findChild<QDockWidget *>("messages")->findChild<QPlainTextEdit *>()->toPlainText().contains("does not identify any Thrusters"),"Empty finite burn failed for an unrelated reason");
         editor->setPlainText(base+mission); require(window.buildScript() && window.runMission()==MainWindow::RunResult::Failed,"Script-configured empty finite burn differs from GUI clear-all"); app.processEvents();
         require(window.findChild<QDockWidget *>("messages")->findChild<QPlainTextEdit *>()->toPlainText().contains("does not identify any Thrusters"),"Script empty burn diagnosis missing");
         require(window.loadScript(saved) && window.buildScript(),"Cleared finite burn could not reopen after reference failure");
         {
            const auto before=editor->toPlainText(); QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("Continuous"),[&](const auto &changes) { error=window.applyResourceChanges("Continuous",changes,before); return error; },&owner,before); select(panel,{"EngineB","EngineA"}); panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         }
         require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"GUI restore after empty finite burn did not recover"); equivalent(rows(report),expected);
         std::cout<<"PASS: finite typed individual/bulk selection, ordered pending/Cancel, Undo/Redo/Unicode save/reopen, paired engines, analytic fuel/coast, script-reference state, invalid edits, unattached-thruster recovery and clear-all\n";
      }
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
