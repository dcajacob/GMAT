#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "CommandForm.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Cannot read fixture/report"); return QString::fromUtf8(file.readAll()); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtCompatibility");
   if (argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(); const QDir samples(QFileInfo(argv[2]).absoluteFilePath());
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir output; require(output.isValid(),"Temporary output directory failed");
      MainWindow window; window.show(); require(window.initialize(startup),"Runtime failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      auto roundTrip=[&](const QString &label) {
         const auto source=editor->toPlainText();
         const auto first=output.filePath(label+" original.script");
         const auto second=output.filePath(label+QString::fromUtf8(" saved Δ.script"));
         require(window.saveScriptTo(first),"Plugin script save failed");
         require(window.saveScriptTo(second),"Plugin script Save As failed");
         require(read(first)==source && read(second)==source,"Plugin save changed script calculations or properties");
         editor->setPlainText("Create MissingPluginType Broken;\nBeginMissionSequence;\n");
         require(!window.buildScript(),"Invalid plugin type unexpectedly built");
         require(window.loadScript(second) && editor->toPlainText()==source && window.buildScript(),
            "Plugin script did not recover unchanged after failed interpretation and reopen");
      };
      const auto types=window.availableEngineTypes();
      require(types.contains("GmatFunction") && types.contains("Yukon") && types.contains("EclipseLocator"),"Expected native plugins were not registered");
      auto function=read(samples.filePath("Ex_GMATFunction_Math.script"));
      function.replace("BeginMissionSequence;","rf.Filename = '"+output.filePath("cross.txt")+"';\nBeginMissionSequence;");
      editor->setPlainText(function); require(window.buildScript(),"Function sample did not build");
      const auto snapshot=window.missionSnapshot(); int call=-1;
      for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.contains("cross(")) call=i;
      require(call>=0,"Function call not in mission tree");
      QString replacement; CommandForm form([&](const QString &value) { replacement=value; });
      form.setStatement(snapshot.nodes[call].statement);
      auto *inputs=form.findChild<QLineEdit *>("commandField_Inputs"); require(inputs,"Function-call form missing"); inputs->setText("vec2, vec1");
      require(window.applyMissionChange(snapshot,call,MissionEdit::Replace,replacement).isEmpty(),"Function form change rejected");
      roundTrip("function");
      require(window.runMission()==MainWindow::RunResult::Completed,"Function plugin execution failed");
      auto *product=Moderator::Instance()->GetInternalObject("crossProd");
      require(product && std::abs(product->GetRealParameter("SingleValue",0,0)+.25)<1e-12 &&
         std::abs(product->GetRealParameter("SingleValue",1,0)-2.75)<1e-12 &&
         std::abs(product->GetRealParameter("SingleValue",2,0)-2)<1e-12,"Function edited argument order produced wrong cross product");
      auto optimize=read(samples.filePath("Ex_Yukon_AlgebraicOptimization.script"));
      optimize.replace("'MinNLPadYukon1.data'","'"+output.filePath("optimizer.data")+"'");
      optimize.replace("'Ex_AlgebraicOptimization.report'","'"+output.filePath("optimize.txt")+"'");
      editor->setPlainText(optimize);
      require(window.buildScript(),"Yukon sample did not build");
      roundTrip("optimizer");
      require(window.runMission()==MainWindow::RunResult::Completed,"Yukon sample failed");
      for (const auto *name:{"X1","X2"}) {
         auto *variable=Moderator::Instance()->GetInternalObject(name);
         require(variable && std::abs(variable->GetRealParameter("Value")-2)<.01,"Yukon numerical result differs from analytic optimum");
      }
      require(!read(output.filePath("optimize.txt")).trimmed().isEmpty(),"Optimizer report missing");
      const auto eventPath=output.filePath("eclipses.txt");
      editor->setPlainText("Create Spacecraft EventSat;\nCreate ForceModel Forces;\nCreate Propagator Prop;\nProp.FM = Forces;\n"
         "Create EclipseLocator Eclipse;\nEclipse.Spacecraft = EventSat;\nEclipse.OccultingBodies = {Earth};\n"
         "Eclipse.Filename = '"+eventPath+"';\nEclipse.RunMode = Automatic;\nEclipse.WriteReport = true;\n"
         "BeginMissionSequence;\nPropagate Prop(EventSat) {EventSat.ElapsedSecs = 12000};\n");
      require(window.buildScript(),"Eclipse fixture failed to build");
      require(window.applyResourceChanges("Eclipse",{{"EclipseTypes","Umbra, Penumbra"},{"OccultingBodies","Earth, Luna"}},editor->toPlainText()).isEmpty(),
         "Event locator list settings failed");
      roundTrip("event");
      require(window.runMission()==MainWindow::RunResult::Completed,"Eclipse locator execution failed");
      const auto events=read(eventPath);
      require(events.contains("Umbra") || events.contains("Penumbra"),"Eclipse locator found no expected shadow intervals");
      auto *resources=window.findChild<QTreeWidget *>("Resources");
      require(resources->findItems("Event Locators",Qt::MatchExactly|Qt::MatchRecursive).size()==1,"Event locator resource category missing");
      auto *outputs=window.findChild<QTreeWidget *>("Output");
      const auto reports=outputs->findItems("Eclipse",Qt::MatchExactly|Qt::MatchRecursive);
      require(reports.size()==1 && reports.first()->data(0,Qt::UserRole).toString()==eventPath,"Event report absent from Output");
      auto python=read(samples.filePath("Ex_PythonInterface.script"));
      const auto pythonReport=output.filePath("python-cross.txt");
      python.replace("BeginMissionSequence;","Create ReportFile PythonReport;\nPythonReport.Filename = '"+pythonReport+"';\nBeginMissionSequence;");
      python+="\nReport PythonReport crossProd(1,1) crossProd(1,2) crossProd(1,3);\n";
      editor->setPlainText(python); require(window.buildScript(),"Python sample did not build");
      roundTrip("python");
      require(window.runMission()==MainWindow::RunResult::Completed,"Python interface execution failed");
      auto *position=Moderator::Instance()->GetInternalObject("state");
      auto *velocity=Moderator::Instance()->GetInternalObject("vel");
      auto *cross=Moderator::Instance()->GetInternalObject("crossProd");
      require(position && velocity && cross,"Python result arrays missing");
      for (int i=0;i<3;++i) {
         const int j=(i+1)%3,k=(i+2)%3;
         const auto expected=position->GetRealParameter("SingleValue",0,j)*velocity->GetRealParameter("SingleValue",0,k)-
            position->GetRealParameter("SingleValue",0,k)*velocity->GetRealParameter("SingleValue",0,j);
         require(std::abs(cross->GetRealParameter("SingleValue",0,i)-expected)<1e-9,"Python cross product differs from independent calculation");
      }
      require(!read(pythonReport).trimmed().isEmpty(),"Python report missing");
      editor->setPlainText("Create Spacecraft MemberA MemberB;\nCreate Formation Fleet;\nFleet.Add = {MemberA, MemberB};\n"
         "Create ForceModel Forces;\nCreate Propagator Prop;\nProp.FM = Forces;\n"
         "BeginMissionSequence;\nPropagate Prop(Fleet) {MemberA.ElapsedSecs = 60};\n");
      require(window.buildScript(),"Formation fixture did not build");
      const auto formationSource=editor->toPlainText();
      require(!window.applyResourceChanges("Fleet",{{"Add","Prop"}},formationSource).isEmpty() && editor->toPlainText()==formationSource,
         "Formation accepted a non-spacecraft member or changed source after rejection");
      require(window.applyResourceChanges("Fleet",{{"Add","MemberB, MemberA"}},editor->toPlainText()).isEmpty(),"Formation member configuration failed");
      roundTrip("formation");
      const auto epoch=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("MemberA"))->GetEpoch();
      require(window.runMission()==MainWindow::RunResult::Completed,"Formation propagation failed");
      for (const auto *name:{"MemberA","MemberB"}) {
         auto *member=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject(name));
         require(member && std::abs((member->GetEpoch()-epoch)*86400-60)<.01,"Formation did not propagate both members for 60 seconds");
      }
      std::cout<<"PASS: Python cross product and report, configured formation members and simultaneous propagation\n";
      std::cout<<"PASS: plugin save, Save As, exact source round trip and failed-build recovery before numerical execution\n";
      std::cout<<"PASS: registered native plugins, edited GMAT function arguments and cross product, Yukon analytic optimum, automatic eclipse events and report access\n";
   } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
