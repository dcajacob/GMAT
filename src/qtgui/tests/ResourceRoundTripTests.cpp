#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "UserParameter.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "FileManager.hpp"
#include <QApplication>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTableWidget>
#include <QTabBar>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QRegularExpression>
#include <QElapsedTimer>
#include <QThread>
#include <iostream>
#include <stdexcept>
static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static QString bytes(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Round-trip report missing"); return QString::fromUtf8(file.readAll()); }
static GmatBase *object(const char *name) { return Moderator::Instance()->GetConfiguredObject(name); }
static ResourceEditor *open(MainWindow &window,const QString &name)
{
   auto *tree=window.findChild<QTreeWidget *>("Resources"); auto items=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Resource row missing"); tree->itemDoubleClicked(items.first(),0); QApplication::processEvents();
   auto *area=window.findChild<QMdiArea *>("workspace"); require(area->activeSubWindow(),"Resource MDI missing"); auto *panel=dynamic_cast<ResourceEditor *>(area->activeSubWindow()->widget()); require(panel,"Resource panel missing"); return panel;
}
static QTableWidgetItem *field(ResourceEditor &panel,const QString &name)
{
   auto *table=panel.findChild<QTableWidget *>(); require(table,"Property table missing"); for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==name) return table->item(row,1); throw std::runtime_error("Property missing");
}
static void apply(ResourceEditor &panel)
{
   panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); QStringList messages; for (auto *label:panel.findChildren<QLabel *>()) messages.append(label->text()); require(!panel.hasChanges(),qPrintable(messages.join('\n')));
}
static void capture(QWidget &widget,const QString &path)
{
   if (path.isEmpty()) return; QElapsedTimer elapsed; elapsed.start(); while (elapsed.elapsed()<250) { QApplication::processEvents(); QThread::msleep(5); } require(widget.grab().save(path),"Round-trip capture failed");
}
static void sourceChecks()
{
   const QString untouched="Create SolarPowerSystem Power; % implicit epoch\n% untouched config Ω\n";
   const QString mission="BeginMissionSequence; % boundary\nFrame.Primary = OtherSat; % mission assignment must stay\n";
   const QString source="Create CoordinateSystem Frame;\nFrame.Axes = ObjectReferenced; % axes note\nFrame.Primary = Sat; % primary note\nFrame.Secondary = Earth;\nFrame.XAxis = R;\nFrame.Origin = Earth; % retained origin\n"+untouched+mission;
   const QString before="Create CoordinateSystem Frame;\nFrame.Origin = Earth;\nFrame.Axes = ObjectReferenced;\nFrame.Primary = Sat;\nFrame.Secondary = Earth;\nFrame.XAxis = R;\n";
   const QString after="Create CoordinateSystem Frame;\nFrame.Origin = Earth;\nFrame.Axes = MJ2000Eq;\n";
   const auto patched=patchResourceConfiguration(source,"Frame",before,after);
   require(patched.contains(untouched) && patched.endsWith(mission) && patched.contains("% axes note") && patched.contains("% primary note") && patched.contains("Frame.Origin = Earth; % retained origin"),"Source patch lost comments/unrelated settings/mission");
   require(!patched.left(patched.indexOf("BeginMissionSequence")).contains("Frame.Primary =") && patched.contains("Frame.Axes = MJ2000Eq;"),"Owned-axis settings not replaced");
   require(patchResourceConfiguration(patched,"Frame",after,after)==patched,"No-op snapshot changed source");
   const QString arrays="Create Array Grid[1,2] Spare[3,4]; % grouped shapes\nGrid( 1, 1 ) = 2; % first cell\nGrid(1,2) = 4;\nSpare(1,1) = 9; % untouched cell\nBeginMissionSequence;\nGrid(1,1) = Spare(1,1) + 1;\n";
   const auto resized=patchResourceConfiguration(arrays,"Grid","Create Array Grid[1,2];\nGrid(1,1) = 2;\nGrid(1,2) = 4;\n","Create Array Grid[2,2];\nGrid(1,1) = 3;\nGrid(2,2) = 8;\n");
   require(resized.contains("Create Array Grid[2,2] Spare[3,4]; % grouped shapes") && resized.contains("% first cell") && resized.contains("Spare(1,1) = 9; % untouched cell") && resized.endsWith("BeginMissionSequence;\nGrid(1,1) = Spare(1,1) + 1;\n") && !resized.contains("Grid(1,2) = 4;"),"Grouped array patch lost declarations/cells/mission");
   const QString nested="Create ForceModel FM;\nFM.GravityField.Earth.Degree = ... % owned gravity\n4;\nBeginMissionSequence;\n";
   const auto gravity=patchResourceConfiguration(nested,"FM","Create ForceModel FM;\nFM.GravityField.Earth.Degree = 4;\n","Create ForceModel FM;\nFM.GravityField.Earth.Degree = 6;\n");
   require(gravity.contains("% owned gravity") && gravity.contains("FM.GravityField.Earth.Degree = 6;") && !gravity.contains("..."),"Continued dotted force setting not patched safely");
   bool refused=false; try { patchResourceConfiguration(source,"Frame",before,"Create Spacecraft Frame;\n"); } catch (const std::exception &) { refused=true; } require(refused,"Resource type change was accepted");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtRoundTrips");
   try {
      sourceChecks(); TestSettings settings; QTemporaryDir files; require(files.isValid() && argc>1,"Round-trip fixtures missing"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(),captures=argc>2 ? QString::fromLocal8Bit(argv[2]) : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Round-trip engine initialization failed"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); const auto report=files.filePath("values.txt"),saved=files.filePath("preserved Ω.script");
      const auto spk=QString::fromStdString(FileManager::Instance()->GetFullPathname("PLANETARY_EPHEM_SPK_PATH"))+"ceres_1900_2100.bsp";
      const QString prefix="% preserved configuration α\nCreate SolarPowerSystem Power; % implicit epoch remains implicit\nPower.ShadowModel = None; % power choice\nCreate Asteroid SpareBody;\nSpareBody.NAIFId = 2000001;\nSpareBody.OrbitSpiceKernelName = {'"+spk+"'}; % unused body has implicit physical defaults\nCreate Spacecraft Sat;\nSat.PowerSystem = Power;\nSat.DisplayStateType = Keplerian;\nSat.SMA = 7200;\nSat.ECC = 0.02;\nSat.INC = 28;\nSat.RAAN = 45;\nSat.AOP = 15;\nSat.TA = 20; % exact input elements\nCreate ForceModel Forces;\nForces.PrimaryBodies = {};\nForces.PointMasses = {Earth};\nCreate Propagator Prop;\nProp.FM = Forces;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 16;\nCreate Array Grid[1,2] Spare[3,4]; % grouped arrays\nGrid(1,1) = 2;\nGrid(1,2) = 4; % editable numeric cells\nSpare(1,1) = 9; % preserved numeric cell\n";
      const QString mission="BeginMissionSequence; % original boundary\nPropagate 'Preserved run' Prop(Sat) {Sat.ElapsedSecs = 120}; % original label\nReport Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.Power.TotalPowerAvailable Sat.Power.RequiredBusPower Sat.Power.ThrustPowerAvailable Grid(1,1) Grid(1,2) Spare(1,1);\n";
      const auto original=prefix+mission; editor->setPlainText(original); require(window.saveScriptTo(saved) && window.runMission()==MainWindow::RunResult::Completed,"Round-trip baseline mission failed"); const auto baseline=bytes(report);
      auto *panel=open(window,"Sat"); field(*panel,"Cd")->setText("2.4"); apply(*panel); const auto spacecraftEdited=editor->toPlainText(); require(spacecraftEdited.contains("Sat.TA = 20; % exact input elements") && spacecraftEdited.contains("% original label") && !spacecraftEdited.contains("Power.InitialEpoch") && !spacecraftEdited.contains("SpareBody.Mu") && spacecraftEdited.endsWith(mission),"Spacecraft edit rewrote unrelated settings/defaults/mission"); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==baseline,"Unrelated spacecraft edit changed power/propagation/array reports");
      editor->undo(); require(editor->toPlainText()==original,"Source-preserving resource Undo not exact"); editor->redo(); require(editor->toPlainText()==spacecraftEdited && window.buildScript(),"Source-preserving Redo failed");
      panel=open(window,"Power"); field(*panel,"InitialMaxPower")->setText("1.7"); field(*panel,"BusCoeff1")->setText("0.25"); field(*panel,"ShadowBodies")->setText("Luna");
      require(editor->toPlainText()==spacecraftEdited && object("Power")->GetRealParameter("InitialMaxPower")==1.2,"Mixed pending edits changed source/model"); capture(*panel->parentWidget(),captures.isEmpty() ? QString() : captures+".power.png"); apply(*panel); const auto mixed=editor->toPlainText(); require(!mixed.contains("Power.InitialEpoch") && !mixed.contains("SpareBody.Mu") && mixed.contains("% implicit epoch remains implicit") && mixed.contains("Sat.TA = 20; % exact input elements") && mixed.endsWith(mission),"Mixed power edit rewrote implicit defaults or unrelated source"); require(window.runMission()==MainWindow::RunResult::Completed,"Mixed power edit mission failed"); const auto powerReport=bytes(report); require(powerReport!=baseline,"Mixed power edits had no effect");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.runMission()==MainWindow::RunResult::Completed && bytes(report)==powerReport,"Mixed edit Unicode save/reopen changed reports");
      const auto reference=prefix+"Sat.Cd = 2.4;\nPower.InitialMaxPower = 1.7;\nPower.BusCoeff1 = 0.25;\nPower.ShadowBodies = {Luna};\n"+mission; editor->setPlainText(reference); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==powerReport,"Mixed GUI edits and independent script differ");
      panel=open(window,"Power"); field(*panel,"Margin")->setText("-1"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(panel->hasChanges() && editor->toPlainText()==reference,"Invalid mixed edit did not retain pending/source"); field(*panel,"Margin")->setText("8"); field(*panel,"ShadowBodies")->setText(""); apply(*panel); require(window.runMission()==MainWindow::RunResult::Completed && object("Power")->GetStringArrayParameter("ShadowBodies").empty(),"Mixed invalid-edit correction/empty shadows failed"); const auto corrected=bytes(report); auto correctedReference=reference; correctedReference.insert(correctedReference.indexOf("BeginMissionSequence"),"Power.Margin = 8;\nPower.ShadowBodies = {};\n"); editor->setPlainText(correctedReference); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==corrected,"Corrected mixed GUI/raw-script reports differ");
      panel=open(window,"Grid"); field(*panel,"RmatValue")->setText("3 4; 0 8"); apply(*panel); const auto resized=editor->toPlainText(); require(resized.contains("Create Array Grid[2,2] Spare[3,4]; % grouped arrays") && resized.contains("Spare(1,1) = 9; % preserved numeric cell") && resized.contains("% editable numeric cells") && resized.endsWith(mission),"Engine array resize lost grouped declaration/comments/mission"); require(window.runMission()==MainWindow::RunResult::Completed,"Source-preserving array resize failed"); const auto arrayReport=bytes(report); auto arrayReference=correctedReference; arrayReference.replace("Grid[1,2]","Grid[2,2]"); arrayReference.replace("Grid(1,1) = 2;","Grid(1,1) = 3;"); arrayReference.insert(arrayReference.indexOf("BeginMissionSequence"),"Grid(2,2) = 8;\n"); editor->setPlainText(arrayReference); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==arrayReport,"Grouped array GUI/raw-script reports differ");
      auto implicit=arrayReference; implicit.replace("BeginMissionSequence; % original boundary\n",""); editor->setPlainText(implicit); require(window.runMission()==MainWindow::RunResult::Completed,"Implicit mission fixture failed"); require(window.applyResourceChanges("Power",{{"BusCoeff1","0.26"},{"ShadowBodies","Luna"}},implicit).isEmpty() && !editor->toPlainText().contains("BeginMissionSequence") && editor->toPlainText().endsWith(implicit.mid(implicit.indexOf("Propagate 'Preserved run'"))),"Mixed edit did not preserve implicit mission boundary"); require(window.runMission()==MainWindow::RunResult::Completed,"Patched implicit mission failed"); const auto implicitReport=bytes(report); implicit.insert(implicit.indexOf("Propagate 'Preserved run'"),"Power.BusCoeff1 = 0.26;\nPower.ShadowBodies = {Luna};\n"); editor->setPlainText(implicit); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==implicitReport,"Implicit mixed GUI/raw-script reports differ");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.runMission()==MainWindow::RunResult::Completed && bytes(report)==implicitReport,"Implicit edited script save/reopen changed reports");
      std::cout<<"Qt resource round trips: changed-assignment snapshots, removed owned fields, dotted continuations, grouped Array resize, exact source/Undo/Redo, preserved implicit power epoch and unused body defaults, mixed power/empty shadows, invalid correction, implicit mission and Unicode save/reopen matched independent script reports\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
