#include "MainWindow.hpp"
#include "OrbitViewDialog.hpp"
#include "PlotWidget.hpp"
#include "QtPlotReceiver.hpp"
#include "ResourceEditor.hpp"
#include "ResourceRename.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "PlotInterface.hpp"
#include <QAction>
#include <QApplication>
#include <QDataStream>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QSlider>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static QByteArray bytes(const QString &path)
{
   QFile file(path); require(file.open(QIODevice::ReadOnly),"Mission report unavailable"); return file.readAll();
}
static QTreeWidgetItem *resource(QTreeWidget *tree,const QString &name)
{
   for (auto *item:tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive))
      if (!item->data(0,Qt::UserRole).toString().isEmpty()) return item;
   throw std::runtime_error(("Missing resource "+name).toStdString());
}
static QByteArray retainedHistory(const PlotModel &model)
{
   // Serialize every sampled point field, including runtime color/orientation/
   // velocity, and every camera sample. Compare bytes, not just sample counts.
   QByteArray result; QDataStream out(&result,QIODevice::WriteOnly);
   out<<qint32(model.kind)<<model.title<<model.coordinates<<model.frame<<model.historyGeneration<<model.lastEpoch<<model.endOfRun;
   out<<quint64(model.curves.size());
   for (auto it=model.curves.cbegin();it!=model.curves.cend();++it) {
      const auto &curve=it.value();
      out<<qint32(it.key())<<curve.name<<curve.color<<curve.radius<<curve.texturePath<<curve.modelPath
         <<curve.visible<<curve.lines<<curve.showObject<<curve.breaks<<quint64(curve.points.size());
      for (const auto &point:curve.points) {
         out<<point.x<<point.y<<point.z<<point.epoch<<point.frame<<point.color<<point.connect<<point.solver
            <<point.high<<point.low<<point.hasSun<<qint32(point.marker)<<point.highlighted<<point.provider<<point.hasVelocity;
         for (auto value:point.bodyToView) out<<value;
         for (auto value:point.sunPosition) out<<value;
         for (auto value:point.inertialToView) out<<value;
         for (auto value:point.viewVelocity) out<<value;
      }
   }
   const auto cameras=[&](const std::deque<PlotCamera> &values) {
      out<<quint64(values.size());
      for (const auto &camera:values) {
         out<<camera.frame<<camera.solver;
         for (auto value:camera.eye) out<<value;
         for (auto value:camera.target) out<<value;
         for (auto value:camera.up) out<<value;
      }
   };
   cameras(model.cameras); out<<quint64(model.cameraViews.size());
   for (const auto &view:model.cameraViews) { out<<view.name<<view.perspective<<view.fieldOfView; cameras(view.cameras); }
   out<<model.userView.has_value();
   if (model.userView) {
      const auto &view=*model.userView;
      out<<view.zoom<<view.yaw<<view.pitch<<view.pan<<view.fit<<view.perspective<<view.fieldOfView<<view.camera;
   }
   return result;
}
static void modal(MainWindow &window,const std::function<void()> &open,const std::function<void(QDialog *)> &check)
{
   std::exception_ptr failure;
   QTimer::singleShot(0,&window,[&] {
      auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
      try { require(dialog,"Expected modal resource dialog missing"); check(dialog); }
      catch (...) { failure=std::current_exception(); }
      if (dialog && dialog->isVisible()) dialog->reject();
   });
   open(); if (failure) std::rethrow_exception(failure);
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("QtPreviewLifetime");
   try {
      require(argc==2 && QGuiApplication::platformName()=="offscreen","Preview lifetime test needs an offscreen startup argument");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Preview fixture directory unavailable");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Preview runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree=window.findChild<QTreeWidget *>("Resources");
      auto *area=window.findChild<QMdiArea *>("workspace"); auto *receiver=window.plotReceiver();
      require(editor && tree && area && receiver,"Preview workspace controls missing");
      const auto report=files.filePath("retained states α.txt");
      const QString source="% retained preview lifetime mission α\nCreate Spacecraft Sat;\nSat.OrbitColor = Green;\n"
         "Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\n"
         "Create Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\n"
         "Create OrbitView Orb;\nOrb.Add = {Sat, Earth};\nOrb.ViewPointVector = [0 0 20000];\nOrb.ViewDirection = Sat;\n"
         "Create GroundTrack Ground;\nGround.Add = {Sat};\n"
         "Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.WriteReport = false;\n"
         "BeginMissionSequence;\nPropagate 'First segment' P(Sat) {Sat.ElapsedSecs = 60};\n"
         "Sat.OrbitColor = 'Red'; % retain runtime color transition\nPropagate 'Second segment' P(Sat) {Sat.ElapsedSecs = 120};\n"
         "Report Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z;\n";
      editor->setPlainText(source);
      if (!window.buildScript()) throw std::runtime_error(("Preview fixture build failed\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()).toStdString());
      require(window.runMission()==MainWindow::RunResult::Completed,"Short preview mission did not complete");
      const auto reportBytes=bytes(report); require(!reportBytes.trimmed().isEmpty(),"Short preview mission report is empty");
      const auto plotWindow=[&](const QString &name)->QMdiSubWindow * {
         for (auto *child:area->subWindowList()) if (child->property("plotName").toString()==name) return child;
         return nullptr;
      };
      const auto plot=[&](const QString &name)->PlotWidget * {
         auto *child=plotWindow(name); auto *widget=child ? dynamic_cast<PlotWidget *>(child->widget()) : nullptr;
         require(widget,"Retained plot widget missing"); return widget;
      };
      const auto orbit=receiver->model("Orb"),ground=receiver->model("Ground");
      const auto describe=[](const QString &name,const std::shared_ptr<const PlotModel> &model) {
         if (!model) return name+": missing model";
         QStringList curves;
         for (const auto &curve:model->curves) curves.append(curve.name+"="+QString::number(curve.points.size())+" samples");
         return name+": frame="+QString::number(model->frame)+", cameras="+QString::number(model->cameras.size())+
            ", scriptedCamera="+QString::number(model->scriptedCamera)+", curves=["+curves.join(", ")+"]";
      };
      std::cerr<<describe("Orb",orbit).toStdString()<<"; "<<describe("Ground",ground).toStdString()<<'\n';
      require(orbit && ground,"Mission did not populate both plot models");
      // OrbitView::Initialize also registers an undrawn Sun for lighting.
      // Require the authored spacecraft/body, not an exact total curve count.
      bool earth=false,groundSamples=false;
      for (const auto &curve:orbit->curves) if (curve.name=="Earth") earth=curve.radius>0 && !curve.points.empty();
      for (const auto &curve:ground->curves) if (curve.name=="Sat") groundSamples=curve.points.size()>5;
      require(earth && groundSamples && !orbit->cameras.empty(),"Mission did not populate authored body/ground histories and camera samples");
      bool samples=false,colors=false;
      for (const auto &curve:orbit->curves) if (curve.name=="Sat") {
         samples=curve.points.size()>5;
         for (const auto &point:curve.points) if (point.color!=curve.points.front().color) colors=true;
      }
      require(samples && colors,"Retained fixture lacks meaningful samples or runtime color transition");
      receiver->setReplayPosition(476); plot("Orb")->canvas()->setViewAngles(.71,.32); plot("Orb")->canvas()->zoomBy(2);
      const auto orbitHistory=retainedHistory(*orbit),groundHistory=retainedHistory(*ground);
      const auto orbitFrame=plot("Orb")->canvas()->frame(),groundFrame=plot("Ground")->canvas()->frame();
      const auto preserved=[&](const char *stage) {
         if (editor->toPlainText()!=source || bytes(report)!=reportBytes || receiver->model("Orb")!=orbit || receiver->model("Ground")!=ground ||
             retainedHistory(*orbit)!=orbitHistory || retainedHistory(*ground)!=groundHistory || receiver->replayPosition()!=476)
            throw std::runtime_error(std::string("Preview cleanup changed source/report/model/history/view/replay: ")+stage);
         for (const auto &name:QStringList{"Orb","Ground"}) if (plotWindow(name)) {
            auto *widget=plot(name); auto *slider=widget->findChild<QSlider *>("plotTimeline");
            require(slider && slider->value()==476,"Retained viewer lost shared replay position");
            require(widget->canvas()->frame()==(name=="Orb" ? orbitFrame : groundFrame),"Retained viewer lost replay frame");
         }
      };
      const auto openPanel=[&](const QString &name)->QMdiSubWindow * {
         tree->itemDoubleClicked(resource(tree,name),0);
         for (auto *child:area->subWindowList()) if (child->property("resourceName").toString()==name) return child;
         throw std::runtime_error("Resource panel did not open");
      };
      const auto form=[](QMdiSubWindow *child)->ResourceEditor * {
         auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"Unified resource panel missing"); return panel;
      };
      const auto closeClean=[&](QMdiSubWindow *child) {
         QPointer<QMdiSubWindow> deleted=child; require(!form(child)->hasChanges(),"Expected clean resource panel");
         require(child->close(),"Clean resource panel did not close"); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
         require(deleted.isNull(),"Resource preview owner was not destroyed");
      };
      // These are the real same-name shared previews captured by ResourceEditor
      // buttons, including clean closure without ever opening a setup dialog.
      closeClean(openPanel("Orb")); preserved("clean Orbit panel close");
      closeClean(openPanel("Ground")); preserved("clean Ground panel close");
      auto *child=openPanel("Orb"); auto *panel=form(child);
      modal(window,[&] { panel->findChild<QPushButton *>("editOrbitView")->click(); },[&](QDialog *dialog) {
         require(dialog->objectName()=="orbitViewDialog","Nested Orbit setup missing");
         auto *scale=dialog->findChild<QLineEdit *>("orbit_ViewScaleFactor"); require(scale,"Nested view scale missing"); scale->setText("2"); dialog->reject();
      });
      require(!panel->hasChanges(),"Nested Cancel retained pending settings"); preserved("nested Orbit Cancel");
      panel->requestApply(); preserved("unchanged parent Apply serialization");
      QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      // Close the accepted unchanged snapshot, then edit a fresh panel.
      closeClean(child); preserved("unchanged applied panel close");
      child=openPanel("Orb"); panel=form(child); auto *scale=panel->findChild<QLineEdit *>("orbit_ViewScaleFactor"); require(scale,"Primary Orbit view scale missing");
      scale->setText("0"); panel->requestApply();
      require(panel->hasChanges() && !panel->findChild<QLabel *>("resourceStatus")->text().isEmpty(),"Invalid parent Apply did not remain pending");
      preserved("failed parent Apply validation");
      QPointer<QMdiSubWindow> discarded=child;
      modal(window,[&] { child->close(); },[&](QDialog *dialog) {
         auto *message=qobject_cast<QMessageBox *>(dialog); require(message && message->standardButtons().testFlag(QMessageBox::Discard),"Pending-close Discard prompt missing");
         message->button(QMessageBox::Discard)->click();
      });
      require(discarded.isNull(),"Discarded resource preview was not destroyed"); preserved("pending resource Discard");
      // Standalone serializers and typed-reference previews also create
      // unregistered same-name OrbitViews outside resource-panel ownership.
      auto *configured=Moderator::Instance()->GetConfiguredObject("Orb"); require(configured,"Configured OrbitView missing");
      require(!orbitViewScript(*configured).isEmpty(),"Orbit serialization returned no script"); preserved("Orbit serializer");
      auto *sat=Moderator::Instance()->GetConfiguredObject("Sat"); require(sat,"Configured spacecraft missing");
      require(renameResourceSource(source,*sat,"PreviewSat").contains("Orb.ViewDirection = PreviewSat;"),"Typed-reference rename preview was not exercised"); preserved("typed-reference rename serialization");
      for (const auto &operation:QStringList{"clone","rename"}) {
         tree->setCurrentItem(resource(tree,"Orb"));
         auto *action=window.findChild<QAction *>(operation+"Resource"); require(action,"Resource action missing");
         modal(window,[&] { action->trigger(); },[&](QDialog *dialog) {
            require(dialog->objectName()==operation+"ResourceDialog","Resource action dialog missing");
            auto *name=dialog->findChild<QLineEdit *>("resourceName"); require(name,"Resource action name field missing"); name->setText("CancelledDisplay"); dialog->reject();
         });
         require(!Moderator::Instance()->GetConfiguredObject("CancelledDisplay"),"Cancelled resource action registered a draft"); preserved(qPrintable(operation+" Cancel"));
      }
      // Destroy the actual viewer widgets, not just hide them. Reopen from
      // Output through the retained receiver entry and reapply shared replay.
      QPointer<QMdiSubWindow> oldOrbit=plotWindow("Orb"),oldGround=plotWindow("Ground");
      oldOrbit->close(); oldGround->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      require(oldOrbit.isNull() && oldGround.isNull(),"Closed plot widgets were not destroyed"); preserved("deleted plot widgets");
      auto *output=window.findChild<QTreeWidget *>("Output"); require(output,"Output tree missing");
      for (const auto &name:QStringList{"Orb","Ground"}) {
         const auto items=output->findItems(name,Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Retained Output entry missing");
         output->itemDoubleClicked(items.first(),0); require(plotWindow(name),"Output did not reconstruct closed plot");
      }
      preserved("actual Output reopen");
      require(plot("Orb")->canvas()->zoomFactor()==orbit->userView->zoom,"Reopened Orbit lost user zoom");
      // Engine-owned cleanup remains effective outside the preview deleter.
      require(PlotInterface::TakeGlAction("Orb","ClearObjects"),"Ordinary engine ClearObjects was rejected");
      require(orbit->curves.empty() && orbit->cameras.empty() && retainedHistory(*ground)==groundHistory && editor->toPlainText()==source && bytes(report)==reportBytes,
         "Preview suppression blocked legitimate engine cleanup or cleared an unrelated display");
      std::cout<<"PASS one short real mission; same-name Orbit/Ground preview close, nested Cancel, no-op/failed Apply, Discard, serializers, Clone/Rename Cancel retain exact histories/colors/cameras/source/report/model pointers and shared47.6%/user view through widget destruction and Output reopen; ordinary engine ClearObjects still clears. Offscreen model qualification only.\n";
      return 0;
   } catch (const std::exception &error) { std::cerr<<"FAIL "<<error.what()<<'\n'; return 1; }
}
