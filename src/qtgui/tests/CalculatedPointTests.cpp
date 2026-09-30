#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "Moderator.hpp"
#include "CelestialBody.hpp"
#include "Barycenter.hpp"
#include "Spacecraft.hpp"
#include "SolarSystem.hpp"
#include "RgbColor.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QInputDialog>
#include <QListWidget>
#include <QLabel>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QWindow>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void run(MainWindow &window)
{
   if (window.runMission()==MainWindow::RunResult::Completed) return;
   QApplication::processEvents();
   throw std::runtime_error(window.findChild<QDockWidget *>("messages")->findChild<QPlainTextEdit *>()->toPlainText().right(3000).toStdString());
}
static QByteArray bytes(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Report unavailable"); return file.readAll(); }
static void later(QWidget *owner,std::exception_ptr &failure,std::function<void()> action)
{
   QTimer::singleShot(0,owner,[owner,&failure,action] { try { action(); } catch (...) { failure=std::current_exception(); for (auto *dialog:owner->findChildren<QDialog *>()) dialog->reject(); } });
}
static void close(QDialog *dialog,bool accept=true) { dialog->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click(); }
static int row(ResourceEditor &panel,const QString &name)
{
   auto *table=panel.findChild<QTableWidget *>();
   for (int i=0;i<table->rowCount();++i) if (table->item(i,0)->text()==name) return i;
   throw std::runtime_error(("Missing field "+name).toStdString());
}
static QTableWidgetItem *field(ResourceEditor &panel,const QString &name) { return panel.findChild<QTableWidget *>()->item(row(panel,name),1); }
static const PlotCurve &curve(const PlotModel &model,const QString &name) { for (const auto &entry:model.curves) if (entry.name==name) return entry; throw std::runtime_error("Calculated-point curve unavailable"); }
static bool waitUntil(const std::function<bool()> &ready)
{
   if (ready()) return true;
   QEventLoop loop; QElapsedTimer elapsed; elapsed.start(); QTimer timer; timer.setInterval(10);
   QObject::connect(&timer,&QTimer::timeout,&loop,[&] { if (ready() || elapsed.elapsed()>3000) loop.quit(); }); timer.start(); loop.exec(); return ready();
}
static void checkWeightedCenter(MainWindow &window,const std::vector<std::string> &names)
{
   const auto model=window.plotReceiver()->model("Orb"); require(model!=nullptr,"Orbit model unavailable");
   const auto &point=curve(*model,"Center").points.back(); const A1Mjd epoch(point.epoch);
   auto *solar=Moderator::Instance()->GetSolarSystemInUse();
   const auto earth=solar->GetBody("Earth")->GetMJ2000State(epoch);
   Rvector6 expected; double mass=0;
   for (const auto &name:names) { auto *body=solar->GetBody(name); mass+=body->GetMass(); expected+=body->GetMJ2000State(epoch)*body->GetMass(); }
   expected=expected/mass-earth;
   require(std::abs(point.x-expected[0])<1e-7 && std::abs(point.y-expected[1])<1e-7 && std::abs(point.z-expected[2])<1e-7,"GUI barycenter disagrees with mass-weighted body positions");
}
static void checkLibration(MainWindow &window,const QString &choice)
{
   const auto model=window.plotReceiver()->model("Orb"); require(model!=nullptr,"Libration plot missing");
   const auto &point=curve(*model,"LP").points.back(); const A1Mjd epoch(point.epoch);
   auto *solar=Moderator::Instance()->GetSolarSystemInUse();
   const auto state=solar->GetBody("Luna")->GetMJ2000State(epoch)-solar->GetBody("Earth")->GetMJ2000State(epoch);
   const auto r=state.GetR(),v=state.GetV(),xAxis=r/r.GetMagnitude(),yAxis=Cross(Cross(r,v).GetUnitVector(),xAxis);
   const Rvector3 displayed(point.x,point.y,point.z); const double x=(displayed*xAxis)/r.GetMagnitude(),y=(displayed*yAxis)/r.GetMagnitude();
   if (choice=="L4" || choice=="L5") require(std::abs(x-.5)<1e-10 && std::abs(y-(choice=="L4" ? 1 : -1)*std::sqrt(3.)/2)<1e-10,"Triangular libration geometry disagrees with selected point");
   else {
      require(std::abs(y)<1e-10 && (choice=="L1" ? x>0 && x<1 : choice=="L2" ? x>1 : x<0),"Collinear libration point on wrong side of selected bodies");
      const double mu=solar->GetBody("Luna")->GetMass()/(solar->GetBody("Earth")->GetMass()+solar->GetBody("Luna")->GetMass());
      const double residual=x-mu-(1-mu)*x/std::pow(std::abs(x),3)-mu*(x-1)/std::pow(std::abs(x-1),3);
      require(std::abs(residual)<1e-7,"Selected collinear point fails equilibrium residual");
   }
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setAttribute(Qt::AA_DontUseNativeDialogs); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtCalculatedPoints"); if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Calculated-point fixtures unavailable");
      MainWindow window; window.show(); require(window.initialize(startup),"Calculated-point runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto report=files.filePath("points.txt"),saved=files.filePath("points ü.script");
      const QString mission="BeginMissionSequence;\nPropagate 'Point mission' Prop(Sat) {Sat.ElapsedSecs = 60}; % retain source\nReport Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.CenterFrame.X Sat.CenterFrame.Y Sat.CenterFrame.Z Sat.LibFrame.X Sat.LibFrame.Y Sat.LibFrame.Z;\n";
      const QString resources="Create Spacecraft Sat;\nCreate ForceModel FM;\nCreate Propagator Prop;\nProp.FM = FM;\nProp.InitialStepSize = 30;\nProp.MinStep = 30;\nProp.MaxStep = 30;\n"
         "Create Barycenter Center;\nCenter.BodyNames = {Earth, Luna};\nCenter.OrbitColor = [238 170 20];\nCenter.TargetColor = [10 40 80];\n"
         "Create LibrationPoint LP;\nLP.Primary = Sun;\nLP.Secondary = Center;\nLP.Point = L4;\nLP.OrbitColor = [30 180 240];\nLP.TargetColor = [80 40 10];\n"
         "Create CoordinateSystem CenterFrame;\nCenterFrame.Origin = Center;\nCenterFrame.Axes = MJ2000Eq;\nCreate CoordinateSystem LibFrame;\nLibFrame.Origin = LP;\nLibFrame.Axes = MJ2000Eq;\n"
         "Create OrbitView Orb;\nOrb.Add = {Sat, Earth, Luna, Center, LP};\nOrb.ViewPointReference = Earth;\nOrb.ViewPointVector = [0 0 1500000];\nOrb.ViewDirection = Earth;\nOrb.EnableStars = Off;\nOrb.EnableConstellations = Off;\nOrb.Grid = Off;\nOrb.Axes = Off;\n"
         "Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 16;\nValues.WriteHeaders = false;\n";
      const auto reference=resources+mission; editor->setPlainText(reference); run(window); const auto expected=bytes(report); checkWeightedCenter(window,{"Earth","Luna"});
      auto initial=reference; initial.replace("Center.BodyNames = {Earth, Luna};","Center.BodyNames = {Earth};"); initial.replace("LP.Secondary = Center;","LP.Secondary = Earth;"); initial.replace("LP.Point = L4;","LP.Point = L1;"); initial.replace("Center.OrbitColor = [238 170 20];","Center.OrbitColor = Red;"); initial.replace("Center.TargetColor = [10 40 80];","Center.TargetColor = Red;"); initial.replace("LP.OrbitColor = [30 180 240];","LP.OrbitColor = Red;"); initial.replace("LP.TargetColor = [80 40 10];","LP.TargetColor = Red;");
      editor->setPlainText(initial); require(window.buildScript(),"Editable point mission failed"); std::exception_ptr failure; QString error;
      auto chooseColor=[&](ResourceEditor &panel,const QString &name,const QColor &color) { later(&panel,failure,[&panel,color] { auto *dialog=panel.findChild<QColorDialog *>("resourceColorDialog"); require(dialog,"Color picker missing"); dialog->setCurrentColor(color); dialog->accept(); }); panel.findChild<QPushButton *>("chooseProperty_"+name)->click(); if (failure) std::rethrow_exception(failure); };
      {
         const auto source=editor->toPlainText(); QWidget owner; auto *center=Moderator::Instance()->GetConfiguredObject("Center"); ResourceEditor panel(*center,[&](const auto &changes) { error=window.applyResourceChanges("Center",changes,source); return error; },&owner,source);
         require(field(panel,"BodyNames")->text()=="Earth","Barycenter list not exposed"); auto *choose=panel.findChild<QPushButton *>("chooseProperty_BodyNames"); require(choose,"Barycenter body selection missing");
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("resourceSelectionDialog"); dialog->findChild<QPushButton *>("resourceClearSelection")->click(); close(dialog,false); }); choose->click(); if (failure) std::rethrow_exception(failure); require(!panel.hasChanges(),"Body picker Cancel changed resource");
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("resourceSelectionDialog"); dialog->findChild<QPushButton *>("resourceClearSelection")->click(); close(dialog); }); choose->click(); if (failure) std::rethrow_exception(failure);
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(!error.isEmpty() && editor->toPlainText()==source,"Empty GUI barycenter selection did not reject Apply");
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("resourceSelectionDialog"); auto *list=dialog->findChild<QListWidget *>("resourceSelectionList"); require(list->findItems("Luna",Qt::MatchExactly).size()==1 && list->findItems("Sat",Qt::MatchExactly).isEmpty() && list->findItems("Center",Qt::MatchExactly).isEmpty() && list->findItems("SolarSystemBarycenter",Qt::MatchExactly).isEmpty(),"Barycenter choices include non-body references"); list->findItems("Earth",Qt::MatchExactly).first()->setCheckState(Qt::Checked); list->findItems("Luna",Qt::MatchExactly).first()->setCheckState(Qt::Checked); close(dialog); }); choose->click(); if (failure) std::rethrow_exception(failure);
         chooseColor(panel,"OrbitColor",QColor(238,170,20)); chooseColor(panel,"TargetColor",QColor(10,40,80)); require(center->GetStringArrayParameter("BodyNames").size()==1,"Pending list mutated configured bodies"); panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         const auto applied=editor->toPlainText(); editor->undo(); require(editor->toPlainText()==source,"Barycenter Undo changed source"); editor->redo(); require(editor->toPlainText()==applied,"Barycenter Redo changed source");
      }
      {
         const auto source=editor->toPlainText(); QWidget owner; auto *lp=Moderator::Instance()->GetConfiguredObject("LP"); ResourceEditor panel(*lp,[&](const auto &changes) { error=window.applyResourceChanges("LP",changes,source); return error; },&owner,source); auto *table=panel.findChild<QTableWidget *>(); auto *point=qobject_cast<QComboBox *>(table->cellWidget(row(panel,"Point"),1)); require(point && point->count()==5 && !point->isEditable(),"Libration point lacks five typed choices");
         auto *choose=panel.findChild<QPushButton *>("chooseProperty_Secondary"); require(choose,"Libration body picker missing"); later(&panel,failure,[&] { auto *dialog=panel.findChild<QInputDialog *>(); require(dialog && dialog->comboBoxItems().contains("Center") && !dialog->comboBoxItems().contains("LP") && !dialog->comboBoxItems().contains("Sat") && !dialog->comboBoxItems().contains("SolarSystemBarycenter"),"Libration choices include disallowed references"); dialog->setTextValue("Center"); dialog->accept(); }); choose->click(); if (failure) std::rethrow_exception(failure); point->setCurrentText("L4"); chooseColor(panel,"OrbitColor",QColor(30,180,240)); chooseColor(panel,"TargetColor",QColor(80,40,10)); require(lp->GetStringParameter("Secondary")=="Earth" && lp->GetStringParameter("Point")=="L1","Pending libration fields mutated model");
         if (!capture.isEmpty()) { panel.setWindowFlag(Qt::Window); panel.resize(760,560); panel.show(); require(waitUntil([&] { return panel.windowHandle() && panel.windowHandle()->isExposed(); }),"Point editor not exposed"); require(panel.grab().save(capture+".setup.png"),"Point setup capture failed"); }
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error)); const auto applied=editor->toPlainText(); require(applied.endsWith(mission),"Point Apply changed labels/comments"); editor->undo(); require(editor->toPlainText()==source,"Libration Undo not exact"); editor->redo(); require(editor->toPlainText()==applied,"Libration Redo not exact");
      }
      require(window.saveScriptTo(saved) && window.loadScript(saved),"Unicode calculated-point save/reopen failed"); run(window); require(bytes(report)==expected,"GUI calculated-point settings changed reference coordinate reports"); checkWeightedCenter(window,{"Earth","Luna"}); auto model=window.plotReceiver()->model("Orb"); require(curve(*model,"Center").color==QColor(238,170,20) && curve(*model,"LP").color==QColor(30,180,240),"Point colors did not reach viewer");
      const auto current=editor->toPlainText(); for (const auto &bad:QStringList{"","Earth, Earth","Sat","Center","SolarSystemBarycenter","Missing"}) require(!window.applyResourceChanges("Center",{{"BodyNames",bad}},current).isEmpty() && editor->toPlainText()==current,"Invalid barycenter bodies changed source");
      for (const auto &bad:QList<QMap<QString,QString>>{{{"Primary","Center"}},{{"Secondary","Sat"}},{{"Secondary","LP"}},{{"Secondary","SolarSystemBarycenter"}},{{"Secondary","Missing"}},{{"Point","L6"}}}) require(!window.applyResourceChanges("LP",bad,current).isEmpty() && editor->toPlainText()==current,"Invalid libration edit changed source"); run(window); require(bytes(report)==expected,"Point invalid-edit recovery changed reports");
      auto *ssb=Moderator::Instance()->GetConfiguredObject("SolarSystemBarycenter"); require(ssb && !isResourceList(*ssb,"BodyNames"),"Built-in barycenter exposes editable bodies"); for (const auto &property:resourceProperties(*ssb)) require(property.name!="BodyNames","Built-in body list not protected in editor"); require(!window.applyResourceChanges("SolarSystemBarycenter",{{"BodyNames","Earth"}},current).isEmpty() && editor->toPlainText()==current,"Built-in barycenter edit accepted");
      {
         const auto source=editor->toPlainText(); QWidget owner; ResourceEditor panel(*ssb,[&](const auto &changes) { error=window.applyResourceChanges("SolarSystemBarycenter",changes,source); return error; },&owner,source);
         require(panel.findChild<QLabel *>("builtInPointInfo") && !panel.findChild<QPushButton *>("chooseProperty_BodyNames"),"Built-in point protection not visible");
         chooseColor(panel,"OrbitColor",QColor(120,80,210)); chooseColor(panel,"TargetColor",QColor(50,20,100)); panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         const auto appearance=editor->toPlainText(); require(!appearance.contains("Create Barycenter SolarSystemBarycenter"),"Built-in color edit created a new body definition"); editor->undo(); require(editor->toPlainText()==source,"Built-in color Undo not exact"); editor->redo(); require(editor->toPlainText()==appearance,"Built-in color Redo not exact");
         require(window.saveScriptTo(files.filePath("built-in.script")) && window.loadScript(files.filePath("built-in.script")),"Built-in colors save/reopen failed");
         require(Moderator::Instance()->GetSpacePoint("SolarSystemBarycenter")->GetCurrentOrbitColor()==RgbColor(120,80,210).GetIntColor(),"Built-in color not retained"); run(window); require(bytes(report)==expected,"Built-in color edit changed numeric reports");
         require(window.applyResourceChanges("SolarSystemBarycenter",{{"OrbitColor","[20 60 120]"}},editor->toPlainText()).isEmpty(),"Existing built-in appearance update rejected"); run(window);
         require(Moderator::Instance()->GetSpacePoint("SolarSystemBarycenter")->GetCurrentTargetColor()==RgbColor(50,20,100).GetIntColor(),"Single color edit changed other built-in color");
      }
      for (const auto &choice:QStringList{"L1","L2","L3","L4","L5"}) {
         {
            const auto source=editor->toPlainText(); QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("LP"),[&](const auto &changes) { error=window.applyResourceChanges("LP",changes,source); return error; },&owner,source);
            field(panel,"Primary")->setText("Earth"); field(panel,"Secondary")->setText("Luna"); auto *table=panel.findChild<QTableWidget *>(); auto *selector=qobject_cast<QComboBox *>(table->cellWidget(row(panel,"Point"),1)); require(selector,"Libration selector unavailable after rebuild"); selector->setCurrentText(choice); panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         }
         require(window.saveScriptTo(files.filePath(choice+".script")) && window.loadScript(files.filePath(choice+".script")),"Libration choice round trip failed"); run(window); checkLibration(window,choice);
      }
      error=window.applyResourceChanges("Center",{{"BodyNames","Luna, Earth"}},editor->toPlainText()); require(error.isEmpty(),qPrintable(error)); run(window); checkWeightedCenter(window,{"Earth","Luna"}); require(Moderator::Instance()->GetConfiguredObject("Center")->GetStringArrayParameter("BodyNames").front()=="Luna","Barycenter order lost");
      require(window.applyResourceChanges("Center",{{"BodyNames","Luna"}},editor->toPlainText()).isEmpty(),"Single-body barycenter rejected"); run(window); checkWeightedCenter(window,{"Luna"});
      require(window.loadScript(saved),"Saved point mission restore failed"); run(window); require(bytes(report)==expected,"Saved point recovery changed reports");
      if (!capture.isEmpty()) {
         require(window.applyResourceChanges("LP",{{"Primary","Earth"},{"Secondary","Luna"},{"Point","L4"}},editor->toPlainText()).isEmpty(),"Rendered triangular point setup failed"); run(window); checkLibration(window,"L4");
         require(window.plotReceiver()->show("Orb"),"Point orbit unavailable"); PlotWidget *view=nullptr; for (auto *child:window.findChildren<QMdiSubWindow *>()) if (child->property("plotName")=="Orb") view=dynamic_cast<PlotWidget *>(child->widget()); require(view,"Point viewer widget missing"); view->canvas()->fit(); QApplication::processEvents();
         const auto image=view->canvas()->captureImage(); require(!image.isNull(),"Point viewer image empty"); int blue=0;
         for (int y=0;y<image.height();++y) for (int x=0;x<image.width();++x) { const auto pixel=image.pixelColor(x,y); if (std::abs(pixel.red()-30)<8 && std::abs(pixel.green()-180)<8 && std::abs(pixel.blue()-240)<8) ++blue; }
         require(blue>4,"Calculated-point marker color absent from rendered image"); require(image.save(capture+".view.png"),"Point viewer capture failed");
      }

      std::cout<<"PASS: typed barycenter/libration selections and five point choices, body add/remove/clear/reorder, pending/Cancel/validation/rollback, colors, exact Undo/Redo/Unicode save/reopen, coordinate-report invariance, mass-weighted and libration geometry, built-in protection, orbit publications and recovery\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; } return 0;
}
