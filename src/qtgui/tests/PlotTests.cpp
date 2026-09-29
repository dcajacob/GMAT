#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "FileManager.hpp"
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTableWidget>
#include <QTreeWidget>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTimer>
#include <fstream>
#include <iostream>
#include <cmath>
#include <stdexcept>
static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static const PlotCurve &curve(const PlotModel &model,const QString &name)
{
   for (const auto &c:model.curves) if (c.name==name) return c;
   throw std::runtime_error("Expected curve missing");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtPlots");
   if (argc<3 || argc>4) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),script=QFileInfo(argv[2]).absoluteFilePath();
   const auto image=argc==4 ? QFileInfo(argv[3]).absoluteFilePath() : QString();
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      PlotModel sample(PlotModel::Kind::GroundTrack);
      sample.maxPoints=3; sample.append(0,170,2); sample.append(0,175,4);
      sample.append(0,std::numeric_limits<double>::quiet_NaN(),8); sample.append(0,-175,8);
      require(!sample.curves[0].points.back().connect,"Missing data bridged a track");
      sample.append(0,-170,9); require(sample.curves[0].points.size()==3,"Point bound not enforced");
      const auto split=PlotModel::groundSegments({170,0},{-170,20});
      require(split.size()==2 && split[0].second==QPointF(180,10) && split[1].first==QPointF(-180,10),"Map-edge interpolation wrong");
      MainWindow window; window.show();
      require(window.initialize(startup),"Initialization failed");
      QTemporaryDir output;
      require(output.isValid(),"Temporary output directory unavailable");
      FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",(output.path()+"/").toStdString());
      require(window.loadScript(script),"Plot mission could not open");
      require(window.runMission()==MainWindow::RunResult::Completed,"Plot mission failed");
      auto *receiver=window.plotReceiver();
      auto orbit=receiver->model("QtOrbit"),ground=receiver->model("QtGround"),xy=receiver->model("QtXY");
      require(orbit && ground && xy,"Mixed plots not created");
      require(receiver->model("QtData") && receiver->model("QtData")->kind==PlotModel::Kind::Table,"Dynamic data display missing");
      require(orbit->kind==PlotModel::Kind::Orbit && ground->kind==PlotModel::Kind::GroundTrack && xy->kind==PlotModel::Kind::XY,"Plot types mixed up");
      const auto &o=curve(*orbit,"QtSat"),&g=curve(*ground,"QtSat");
      require(o.points.size()>100 && g.points.size()>100 && xy->curves[0].points.size()>100,"No real plot histories recorded");
      require(!ground->map.isNull(),"Resolved central body map did not load");
      const auto sampled=receiver->model("QtSampledGround");
      require(sampled && sampled->frame==(ground->frame+6)/7,"Configured collection frequency was not honored");
      require(curve(*sampled,"QtSat").points.size()==50 && curve(*sampled,"QtSat").width==3,
         "Configured retention limit or line width was not honored");
      std::ifstream report((output.path()+"/QtPlotReference.txt").toStdString());
      double secs,x,y,z,lon,lat;
      require(static_cast<bool>(report>>secs>>x>>y>>z>>lon>>lat),"Reference report missing or malformed");
      require(std::abs(secs-12000)<1e-6,"Mission did not finish at requested epoch");
      const auto &last=o.points.back();
      require(std::abs(last.x-x)<1e-7 && std::abs(last.y-y)<1e-7 && std::abs(last.z-z)<1e-7,"Orbit coordinates differ from engine report");
      const auto &lastxy=xy->curves.constFind(0).value().points.back();
      require(std::abs(lastxy.x-secs)<1e-5 && std::abs(lastxy.y-x)<1e-7,"XY samples differ from engine report");
      const auto &lastg=g.points.back();
      require(std::abs(std::remainder(lastg.x-lon,360.0))<1e-6 && std::abs(lastg.y-lat)<1e-5,"Ground track differs from geodetic engine report");
      const auto count=o.points.size();
      auto *area=window.findChild<QMdiArea *>("workspace"); require(area!=nullptr,"Workspace missing");
      require(receiver->show("QtData"),"Dynamic display could not open");
      auto *table=qobject_cast<QTableWidget *>(area->activeSubWindow()->widget());
      require(table && table->rowCount()>0 && table->columnCount()>0,"Dynamic table is empty");
      QString tableText; QColor tableColor; bool elapsedFound=false;
      for (int r=0;r<table->rowCount();++r) for (int c=0;c<table->columnCount();++c) {
         if (auto *item=table->item(r,c)) if (item->text().contains("12000")) {
            tableText=item->text(); tableColor=item->foreground().color(); elapsedFound=true;
         }
      }
      require(elapsedFound && tableColor==QColor(20,80,150),"Dynamic table values or colors differ from mission");
      area->activeSubWindow()->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      require(receiver->show("QtData"),"Closed dynamic table could not reopen");
      table=qobject_cast<QTableWidget *>(area->activeSubWindow()->widget());
      const auto matches=table->findItems(tableText,Qt::MatchExactly);
      require(matches.size()==1 && matches.first()->foreground().color()==tableColor,"Reopened dynamic table lost values or colors");
      auto *outputTree=window.findChild<QTreeWidget *>("Output");
      const auto reports=outputTree->findItems("QtReport",Qt::MatchExactly|Qt::MatchRecursive);
      require(reports.size()==1,"Report not listed in Output tree");
      outputTree->itemDoubleClicked(reports.first(),0);
      auto *viewer=window.findChild<QPlainTextEdit *>("report:QtReport");
      require(viewer && viewer->toPlainText().contains("12000"),"Output report did not open generated file");
      viewer->parentWidget()->close();
      auto closePlots=[&] { for (auto *child:area->subWindowList()) if (!child->property("plotName").toString().isEmpty()) child->close(); };
      closePlots(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      require(!receiver->IsThere("QtOrbit") && receiver->show("QtOrbit"),"Closed plot could not reopen");
      require(receiver->model("QtOrbit")->curves[0].points.size()==count,"Reopening lost recorded samples");
      auto *plot=dynamic_cast<PlotWidget *>(area->activeSubWindow()->widget()); require(plot!=nullptr,"Reopened plot wrong widget type");
      const double zoom=plot->canvas()->zoomFactor(); plot->canvas()->zoomBy(1); plot->canvas()->zoomBy(-1);
      require(std::abs(plot->canvas()->zoomFactor()-zoom)<1e-12,"Wheel zoom not reversible");
      plot->canvas()->setFrame(2); plot->canvas()->grab(); plot->canvas()->setFrame(std::numeric_limits<quint64>::max());
      require(receiver->model("QtOrbit")->curves[0].points.size()==count,"Replay discarded recorded data");
      bool closedDuringRun=false;
      QTimer::singleShot(1,&window,[&] { closePlots(); closedDuringRun=window.isRunning(); });
      require(window.runMission()==MainWindow::RunResult::Completed,"Plot rerun failed");
      require(closedDuringRun,"Close-during-run was not exercised");
      require(receiver->model("QtOrbit")->curves[0].points.size()==count,"Rerun accumulated old data");
      receiver->CreateGroundTrackWindow("Sparse","","Sparse",0,0,.5,.5,false);
      receiver->TakeGroundTrackAction("Sparse","Satellites=QtSat|MissingSat");
      const double valid[]={10,20,30,40},missing[]={11,21,NAN,NAN},again[]={12,22,32,42};
      receiver->UpdateGroundTrackData("Sparse",1,valid,2); receiver->UpdateGroundTrackData("Sparse",2,missing,2); receiver->UpdateGroundTrackData("Sparse",3,again,2);
      auto sparse=receiver->model("Sparse");
      require(sparse->curves[0].points.size()==3 && sparse->curves[1].points.size()==2 && !sparse->curves[1].points.back().connect,"Absent satellite corrupted track slots");
      require(!receiver->TakeGroundTrackAction("Sparse","AddStation=QtSat"),"Non-station object accepted as station");
      receiver->DeleteGlPlot("Sparse");
      receiver->DeleteDynamicData("QtData","");
      receiver->DeleteGlPlot("QtSampledGround");
      for (const auto &name:receiver->names()) receiver->show(name);
      area->tileSubWindows(); QApplication::processEvents();
      const auto rendered=window.grab();
      require(std::abs(rendered.width()-window.width()*window.devicePixelRatioF())<=1,"Plot capture ignored physical display scale");
      if (!image.isEmpty()) require(rendered.save(image),"Plot screenshot failed");
      std::cout<<"PASS: real orbit/XY/geodetic samples, map, dateline, sparse data, close-during-run/reopen/rerun, bounded history, replay retention, reversible zoom, dynamic values/colors, Output report viewer\n";
   } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
