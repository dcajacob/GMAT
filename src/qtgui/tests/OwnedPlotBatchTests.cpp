#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "OwnedPlot.hpp"
#include "PlotInterface.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QFile>
#include <QImage>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPointer>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static QString read(const QString &path)
{
   QFile file(path); require(file.open(QIODevice::ReadOnly),"Cannot read residual export");
   return QString::fromUtf8(file.readAll());
}
static PlotWidget *viewer(QMdiArea &area)
{
   auto *child=area.activeSubWindow();
   auto *plot=child ? dynamic_cast<PlotWidget *>(child->widget()) : nullptr;
   require(plot,"Owned residual viewer is absent"); return plot;
}
static void samples(const PlotCurve &curve,const RealArray &x,const RealArray &y,
                    const RealArray &high,const RealArray &low)
{
   require(curve.points.size()==x.size(),"Deactivated owned plot discarded or accumulated residual samples");
   for (size_t i=0;i<x.size();++i) {
      const auto &point=curve.points[i];
      require(std::isfinite(point.x) && std::isfinite(point.y) && point.x==x[i] && point.y==y[i],
         "Owned residual sample differs from supplied numeric data");
      require(point.high==high[i] && point.low==low[i],"Owned residual asymmetric error bars were lost");
      require(point.connect==(i>0),"Redraw suspension changed residual connections");
   }
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   if (QGuiApplication::platformName()!="offscreen" || argc>2) return 2;
   QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("OwnedPlotBatch");
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Residual artifacts unavailable");
      QMdiArea area; area.resize(1100,760); area.show(); QApplication::processEvents();
      QtPlotReceiver receiver(&area);
      PlotInterface::SetPlotReceiver(&receiver);
      struct ResetReceiver { ~ResetReceiver() { PlotInterface::SetPlotReceiver(nullptr); } } reset;
      OwnedPlot residual("EstimatorResiduals","Owned residual batch","TAIModJulian","Observed minus computed (km)");
      residual.SetBooleanParameter("UseHiLow",true);
      residual.SetStringParameter("Add","Residual X"); residual.SetStringParameter("Add","Residual Y");
      require(residual.Initialize(),"Actual OwnedPlot initialization failed");
      auto model=receiver.model("EstimatorResiduals");
      require(model && model->kind==PlotModel::Kind::XY && model->curves.size()==2,"Owned residual curves were not created");
      auto *plot=viewer(area); const auto emptyImage=plot->canvas()->captureImage();
      require(!emptyImage.isNull(),"Empty residual viewer did not render");
      // Exactly the estimator's OwnedPlot bulk sequence. Shared replay gives
      // a finite visible frame, making the deferred/final refresh observable.
      residual.TakeAction("ClearData"); receiver.setReplayPosition(500);
      const auto beforeFrame=plot->canvas()->frame(); require(beforeFrame==0,"Empty replay interval is unexpected");
      require(residual.Deactivate(),"Owned residual Deactivate failed");
      RealArray x{31000,31000.25,31000.5}, first{-3,2,8}, second{4,-2,6};
      RealArray high{.25,.5,.75},low{.1,.2,.3};
      std::vector<RealArray *> blast{&x,&first,&second};
      residual.SetData(blast,high,low);
      samples(model->curves[0],x,first,high,low); samples(model->curves[1],x,second,high,low);
      require(model->active && plot->canvas()->frame()==beforeFrame,"Deactivate changed data admission or refreshed the batch prematurely");
      require(residual.TakeAction("Rescale"),"Owned residual Rescale failed");
      require(residual.Activate(),"Owned residual Activate failed");
      require(plot->canvas()->frame()==2 && receiver.replayPosition()==500,"Activate did not refresh the retained replay interval");
      receiver.releaseSharedReplay(); plot->setSharedReplayPosition(1000);
      QApplication::processEvents(); const auto populatedImage=plot->canvas()->captureImage();
      require(populatedImage!=emptyImage,"Activated residual plot still renders the empty axes");
      const auto exported=files.filePath("residual α.txt");
      require(plot->exportData(exported).isEmpty(),"Owned residual export failed");
      const auto text=read(exported);
      require(text.contains("Residual X\n31000, -3\n31000.25, 2\n31000.5, 8\n") &&
              text.contains("Residual Y\n31000, 4\n31000.25, -2\n31000.5, 6\n"),
         "Activated residual export contains labels without exact numeric series");
      // A second suspended append must keep its connection to the previous
      // batch. SetCurveData uses the other real PlotInterface callback path.
      residual.Deactivate(); RealArray nextX{31000.75},nextY{-4},nextHigh{.9},nextLow{.4};
      residual.SetCurveData(0,&nextX,&nextY,&nextHigh,&nextLow);
      // const QMap indexing copies the curve; retain the point by value.
      const auto last=model->curves[0].points.back();
      require(model->curves[0].points.size()==4 && last.x==nextX[0] && last.y==nextY[0] &&
         last.high==nextHigh[0] && last.low==nextLow[0] && last.connect,"Suspended single-curve update broke or dropped its sample");
      residual.Activate();
      QPointer<QMdiSubWindow> closed=area.activeSubWindow(); closed->close();
      QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      require(closed.isNull() && receiver.model("EstimatorResiduals")==model,"Viewer close removed the retained residual data");
      require(receiver.show("EstimatorResiduals"),"Owned residual reopen failed"); plot=viewer(area);
      const auto reopened=files.filePath("reopened α.txt"); require(plot->exportData(reopened).isEmpty(),"Reopened residual export failed");
      require(read(reopened).contains("31000.75, -4"),"Reopened residual viewer lost the latest batch");
      // Clear/repopulate mirrors the next estimator iteration: keep the model
      // and curve definitions, replace history rather than accumulating it.
      residual.TakeAction("ClearData"); residual.Deactivate(); residual.SetData(blast,high,low);
      residual.TakeAction("Rescale"); residual.Activate();
      require(receiver.model("EstimatorResiduals")==model,"Bulk refill replaced the retained plot model");
      samples(model->curves[0],x,first,high,low); samples(model->curves[1],x,second,high,low);
      // Ordinary subscriber Toggle controls data admission independently.
      require(receiver.TakeXYAction("EstimatorResiduals","ToggleOff"),"ToggleOff failed");
      residual.Deactivate(); residual.SetCurveData(0,&nextX,&nextY); residual.Activate();
      require(!model->active && model->curves[0].points.size()==3,"Owned Activate bypassed subscriber ToggleOff");
      require(receiver.TakeXYAction("EstimatorResiduals","ToggleOn"),"ToggleOn failed");
      residual.SetCurveData(0,&nextX,&nextY);
      require(model->active && model->curves[0].points.size()==4 && !model->curves[0].points.back().connect,
         "Subscriber ToggleOn did not resume with the required data gap");
      std::cout<<"PASS: actual OwnedPlot batch/single-curve storage while deactivated, asymmetric errors and connections, Activate refresh, populated export, retained close/reopen/refill and independent subscriber Toggle semantics\n";
      return 0;
   } catch (BaseException &error) { std::cerr<<"Owned residual check failed: "<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<"Owned residual check failed: "<<error.what()<<'\n'; return 1; }
}
