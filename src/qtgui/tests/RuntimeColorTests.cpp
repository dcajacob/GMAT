#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "QtPlotReceiver.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include "RgbColor.hpp"
#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Runtime color report unavailable"); return file.readAll(); }
static void run(MainWindow &window) { if (window.runMission()!=MainWindow::RunResult::Completed) { QApplication::processEvents(); throw std::runtime_error(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()); } }
static const PlotCurve &curve(const PlotModel &model,const QString &name) { for (const auto &item:model.curves) if (item.name==name) return item; throw std::runtime_error("Runtime color spacecraft curve missing"); }
static QColor named(const char *name) { return QColor::fromRgb(RgbColor::ToIntColor(name)&0xffffff); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtRuntimeColors");
   try {
      TestSettings settings; QTemporaryDir files; require(argc==2 && files.isValid(),"Runtime color setup failed"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Runtime color initialization failed"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto report=files.filePath("runtime colors.txt"),saved=files.filePath("runtime colors Δ.script");
      const QString prefix="% retain runtime color source α\nCreate Spacecraft Sat Peer;\nSat.OrbitColor = Red;\nSat.TargetColor = Yellow;\nPeer.OrbitColor = Yellow;\nPeer.TargetColor = Magenta;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\nCreate OrbitView Orb;\nOrb.Add = {Sat, Peer, Earth};\nCreate GroundTrack Ground;\nGround.Add = {Sat, Peer};\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 16;\nValues.WriteHeaders = false;\nBeginMissionSequence;\n";
      const QString mission="Propagate P(Sat, Peer) {Sat.ElapsedSecs = 30};\nSat.OrbitColor = 'Red'; % first runtime color\nSat.TargetColor = 'Yellow'; % runtime solver color\nPropagate P(Sat, Peer) {Sat.ElapsedSecs = 60};\nPropagate P(Sat, Peer) {Sat.ElapsedSecs = 90, OrbitColor = Blue};\nSat.OrbitColor = 'Red'; % second runtime color\nPropagate P(Sat, Peer) {Sat.ElapsedSecs = 120};\n";
      const QString suffix="Report Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ Peer.EarthMJ2000Eq.X Peer.EarthMJ2000Eq.Y Peer.EarthMJ2000Eq.Z Peer.EarthMJ2000Eq.VX Peer.EarthMJ2000Eq.VY Peer.EarthMJ2000Eq.VZ;\n";
      const auto source=prefix+mission+suffix;
      auto plain=source; plain.remove("Sat.OrbitColor = 'Red'; % first runtime color\n"); plain.remove("Sat.OrbitColor = 'Red'; % second runtime color\n"); plain.remove("Sat.TargetColor = 'Yellow'; % runtime solver color\n"); plain.remove(", OrbitColor = Blue"); editor->setPlainText(plain); run(window); const auto expected=read(report); require(!expected.isEmpty(),"Independent uncolored report empty");
      auto reference=source; reference.replace("'Red'; % first runtime color","'Green'; % first runtime color"); reference.replace("'Red'; % second runtime color","'[20 180 90]'; % second runtime color"); reference.replace("'Yellow'; % runtime solver color","'Cyan'; % runtime solver color");
      editor->setPlainText(reference); run(window); require(read(report)==expected,"Runtime colors changed independent calculation report");
      for (const auto &name:QStringList{"Orb","Ground"}) {
         const auto model=window.plotReceiver()->model(name); require(bool(model),"Runtime color plot missing");
         int green=0,rgb=0,blue=0,yellow=0; for (const auto &point:curve(*model,"Sat").points) { green+=point.color==named("Green"); rgb+=point.color==QColor(20,180,90); blue+=point.color==named("Blue"); }
         for (const auto &point:curve(*model,"Peer").points) yellow+=point.color==named("Yellow");
         if (green<2 || rgb<2 || blue<2 || yellow<2) throw std::runtime_error((name+" runtime palette: green="+QString::number(green)+", rgb="+QString::number(rgb)+", blue="+QString::number(blue)+", peer yellow="+QString::number(yellow)).toStdString());
      }
      QMap<QString,std::deque<PlotPoint>> paths; for (const auto &plot:QStringList{"Orb","Ground"}) for (const auto &name:QStringList{"Sat","Peer"}) paths[plot+name]=curve(*window.plotReceiver()->model(plot),name).points;
      editor->setPlainText(source); require(window.buildScript(),"Runtime color edit source failed");
      const auto edit=[&](const QString &comment,const QString &value,bool rejectSyntax) {
         const auto snapshot=window.missionSnapshot(); int index=-1; for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.contains(comment)) index=i;
         require(index>=0,"Runtime color assignment missing from mission"); auto *tree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItemIterator item(tree);
         while (*item && (!(*item)->data(0,Qt::UserRole).isValid() || (*item)->data(0,Qt::UserRole).toInt()!=index)) ++item;
         require(*item,"Runtime color mission tree item missing"); tree->itemDoubleClicked(*item,0); auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow(); auto *panel=child ? dynamic_cast<CommandEditor *>(child->widget()) : nullptr; require(panel,"Runtime color MDI editor missing");
         auto *expression=panel->findChild<QLineEdit *>("commandField_Expression"); require(expression,"Assignment expression control missing"); const auto before=editor->toPlainText(); auto *apply=panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply);
         if (rejectSyntax) { expression->setText("'unterminated"); apply->click(); require(editor->toPlainText()==before && child->widget()==panel && panel->hasChanges(),"Rejected assignment lost original source/pending panel"); }
         expression->setText(value); require(panel->hasChanges() && editor->toPlainText()==before,"Runtime color edit did not stay pending"); apply->click();
         require(child->isVisible() && child->widget()!=panel && !dynamic_cast<EditablePanel *>(child->widget())->hasChanges(),"Runtime color Apply did not refresh retained panel"); const auto changed=editor->toPlainText(); editor->undo(); require(editor->toPlainText()==before,"Runtime color mission Undo was not exact"); editor->redo(); require(editor->toPlainText()==changed,"Runtime color mission Redo was not exact");
      };
      edit("first runtime color","'Green'",true); edit("runtime solver color","'Cyan'",false); edit("second runtime color","'[20 180 90]'",false);
      require(editor->toPlainText().startsWith(prefix) && editor->toPlainText().endsWith(suffix),"Runtime color commands changed other source"); require(window.saveScriptTo(saved) && window.loadScript(saved),"Runtime color Unicode save/reopen failed"); run(window); require(read(report)==expected,"GUI runtime color edits changed calculation report");
      for (const auto &plot:QStringList{"Orb","Ground"}) for (const auto &name:QStringList{"Sat","Peer"}) {
         const auto &points=curve(*window.plotReceiver()->model(plot),name).points; const auto &want=paths.value(plot+name); require(points.size()==want.size(),"Runtime color history count changed");
         for (size_t i=0;i<points.size();++i) require(points[i].color==want[i].color && std::abs(points[i].x-want[i].x)<1e-8 && std::abs(points[i].y-want[i].y)<1e-8 && std::abs(points[i].z-want[i].z)<1e-8,"GUI runtime trajectory/color differs from independent script");
      }
      auto *receiver=window.plotReceiver(); require(receiver->TakeGroundTrackAction("Ground","SolverData=On"),"Runtime target palette rejected"); require(curve(*receiver->model("Ground"),"Sat").color==named("Cyan") && curve(*receiver->model("Ground"),"Peer").color==named("Magenta"),"Runtime target change did not reach solver palette");
      require(receiver->TakeGroundTrackAction("Ground","SolverData=Off") && curve(*receiver->model("Ground"),"Sat").color==QColor(20,180,90) && curve(*receiver->model("Ground"),"Peer").color==named("Yellow"),"Runtime default palette did not restore");
      // Rerun from the saved configuration to prove old runtime palettes do not
      // leak into the initial red/yellow arc of a new mission.
      run(window); require(read(report)==expected,"Runtime-color rerun changed report"); for (const auto &plot:QStringList{"Orb","Ground"}) require(curve(*receiver->model(plot),"Sat").points.front().color==named("Red") && curve(*receiver->model(plot),"Peer").points.front().color==named("Yellow"),"Rerun retained previous runtime defaults");
      std::cout<<"PASS: actual MDI named/RGB runtime color and target assignments, pending failed-syntax correction, retained panels and exact Undo/Redo/Unicode save/reopen; unchanged independent twelve-state reports and two-spacecraft Orbit/Ground default/segment color histories; target/accepted palette transitions and rerun reset.\n";
   } catch (BaseException &failure) { std::cerr<<"FAIL: "<<failure.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &failure) { std::cerr<<"FAIL: "<<failure.what()<<'\n'; return 1; }
}
