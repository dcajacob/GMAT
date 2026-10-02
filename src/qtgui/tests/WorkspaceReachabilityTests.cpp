#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QMenu>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static QByteArray bytes(const QString &path)
{
   QFile file(path); require(file.open(QIODevice::ReadOnly),"Reachability report missing"); return file.readAll();
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("QtWorkspaceReachability");
   try {
      require(argc==2 && QGuiApplication::platformName()=="offscreen","Reachability test requires offscreen startup");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Reachability fixture directory unavailable");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Reachability runtime initialization failed"); window.resize(1280,900); window.show(); app.processEvents();
      auto *area=window.findChild<QMdiArea *>("workspace"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      auto *tree=window.findChild<QTreeWidget *>("Resources"); require(area && editor && tree,"Reachability controls missing");
      const auto report=files.filePath("reachable states α.txt");
      const QString baseline="% preserve reachability source α\nCreate Spacecraft Sat;\nCreate ForceModel FM;\n"
         "FM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator P;\nP.FM = FM;\n"
         "P.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\n"
         "Create OrbitView Orb;\nOrb.Add = {Sat, Earth};\nCreate ReportFile Values;\n"
         "Values.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.WriteReport = false;\n"
         "BeginMissionSequence;\nPropagate 'Retain mission label' P(Sat) {Sat.ElapsedSecs = 60};\n"
         "Report Values Sat.EarthMJ2000Eq.X;\n";
      editor->setPlainText(baseline);
      const auto build=[&] {
         if (!window.buildScript()) throw std::runtime_error(("Reachability fixture build failed\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()).toStdString());
      };
      build();
      // Use the unified creator. Its successful transaction opens the actual
      // settings panel retained while the next build/run replaces the engine.
      auto *create=window.findChild<QAction *>("createResource"); require(create && create->isEnabled(),"Create resource action missing");
      std::exception_ptr failure;
      QTimer::singleShot(0,&window,[&] {
         auto *dialog=window.findChild<QDialog *>("newResourceDialog");
         try {
            require(dialog,"Unified resource creator missing");
            auto *type=dialog->findChild<QComboBox *>("resourceType"); require(type && type->findText("XYPlot")>=0,"XYPlot type missing"); type->setCurrentText("XYPlot");
            auto *name=dialog->findChild<QLineEdit *>("resourceName"); auto *x=dialog->findChild<QLineEdit *>("xyXVariable"); auto *y=dialog->findChild<QListWidget *>("xyYVariables");
            require(name && x && y,"Unified XY creation controls missing"); name->setText("SettingsPlot"); x->setText("Sat.ElapsedSecs"); y->clear(); y->addItem("Sat.EarthMJ2000Eq.X");
            auto *buttons=dialog->findChild<QDialogButtonBox *>("resourceCreationButtons"); require(buttons,"Create buttons missing"); buttons->button(QDialogButtonBox::Ok)->click();
            require(dialog->result()==QDialog::Accepted,"Configured XY creation did not commit");
         } catch (...) { failure=std::current_exception(); }
         if (dialog && dialog->isVisible()) dialog->reject();
      });
      create->trigger(); if (failure) std::rethrow_exception(failure); app.processEvents();
      QMdiSubWindow *child=nullptr;
      for (auto *candidate:area->subWindowList()) if (candidate->property("resourceName").toString()=="SettingsPlot") child=candidate;
      require(child && child->property("configurationPanel").toBool(),"Created resource panel missing");
      auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel && !panel->hasChanges(),"Created resource panel not clean");
      QPointer<ResourceEditor> retained=panel; const auto source=editor->toPlainText();
      const auto placement=[&](const char *stage) {
         const auto r=child->geometry(),v=area->viewport()->rect();
         const auto m=child->minimumSize(),h=child->minimumSizeHint(),cm=panel->minimumSize(),ch=panel->minimumSizeHint();
         std::cerr<<stage<<": viewport "<<v.x()<<","<<v.y()<<" "<<v.width()<<"x"<<v.height()
            <<" child "<<r.x()<<","<<r.y()<<" "<<r.width()<<"x"<<r.height()
            <<" minimum "<<m.width()<<"x"<<m.height()<<" minHint "<<h.width()<<"x"<<h.height()
            <<" contentMin "<<cm.width()<<"x"<<cm.height()<<" contentHint "<<ch.width()<<"x"<<ch.height()
            <<" minimized="<<child->isMinimized()<<" maximized="<<child->isMaximized()<<'\n';
      };
      placement("initial after settled Create");
      require(area->viewport()->rect().contains(child->geometry()),"Initial resource panel outside workspace");
      require(window.runMission()==MainWindow::RunResult::Completed,"Short reachability mission failed"); app.processEvents();
      require(retained && child->widget()==retained && editor->toPlainText()==source,"Build/run replaced retained settings or changed source");
      const auto reportBytes=bytes(report); require(!reportBytes.trimmed().isEmpty(),"Short reachability report empty");
      auto *x=panel->findChild<QLineEdit *>("xyXVariable"); require(x,"Retained XY parameter control missing"); x->setText("Sat.A1ModJulian");
      require(panel->hasChanges(),"Reachability pending edit missing");
      // Simulate a late layout minimum growing after original placement, then
      // a retained panel restored at a position beyond the viewport.
      const auto bounds=area->viewport()->rect();
      require(bounds.width()>800 && bounds.height()>500,"Reachability viewport unexpectedly small");
      panel->setMinimumSize(qMin(bounds.width()-20,child->width()+120),panel->minimumHeight()); app.processEvents();
      const auto otherGeometry=[&] {
         QMap<QMdiSubWindow *,QRect> result;
         for (auto *other:area->subWindowList()) if (other!=child) result.insert(other,other->geometry());
         return result;
      };
      const auto unchanged=[&](const QMap<QMdiSubWindow *,QRect> &before) {
         for (auto it=before.cbegin();it!=before.cend();++it) require(it.key()->geometry()==it.value(),"Selected panel activation moved another window");
         require(retained && child->widget()==retained && retained->hasChanges() && x->text()=="Sat.A1ModJulian" &&
            editor->toPlainText()==source && bytes(report)==reportBytes,"Panel reachability changed pending settings/source/report or replaced owner");
      };
      const auto reachable=[&] {
         placement("explicit activation");
         require(!child->isMinimized() && !child->isMaximized() && area->viewport()->rect().contains(child->geometry()),"Activated normal settings panel outside workspace");
         auto *close=panel->findChild<QPushButton *>("closePanel"); require(close && close->isVisible(),"Retained Close control missing");
         const QRect closeRect(close->mapTo(area->viewport(),QPoint()),close->size());
         require(area->viewport()->rect().contains(closeRect),"Settings Close control remains clipped");
      };
      const auto placeOutside=[&] {
         child->move(bounds.right()-50,bounds.bottom()-50); app.processEvents();
         require(!area->viewport()->rect().contains(child->geometry()),"Outside-panel fixture did not leave viewport");
         child->showMinimized(); app.processEvents(); require(child->isMinimized(),"Restore fixture did not minimize");
      };
      placeOutside(); auto before=otherGeometry();
      auto *menu=window.findChild<QMenu *>("windowMenu"); require(menu,"Window menu missing");
      const auto windowEntry=[&](QMdiSubWindow *selected) {
         menu->aboutToShow(); QList<QAction *> entries;
         for (auto *action:menu->actions()) if (action->objectName()=="windowEntry") entries.append(action);
         int index=0;
         // A resource settings panel and its viewer have the same title.
         // Select the actual retained child by the menu's workspace ordering.
         for (auto *candidate:area->subWindowList()) if (candidate->isVisible()) {
            require(index<entries.size(),"Window menu omitted a visible child");
            if (candidate==selected) return entries[index];
            ++index;
         }
         throw std::runtime_error("Selected Window menu entry missing");
      };
      windowEntry(child)->trigger(); app.processEvents(); reachable(); unchanged(before);
      // Resources must reuse the exact pending panel rather than reconstruct
      // it, and apply the same settled-layout restoration clamp.
      placeOutside(); before=otherGeometry();
      QTreeWidgetItem *item=nullptr;
      for (auto *candidate:tree->findItems("SettingsPlot",Qt::MatchExactly|Qt::MatchRecursive)) if (candidate->data(0,Qt::UserRole).toString()=="SettingsPlot") item=candidate;
      require(item,"Created resource tree entry missing"); tree->itemDoubleClicked(item,0); app.processEvents(); reachable(); unchanged(before);
      // The Window route deliberately leaves ordinary viewer geometry alone.
      QMdiSubWindow *viewer=nullptr;
      for (auto *candidate:area->subWindowList()) if (candidate->property("plotName").toString()=="Orb") viewer=candidate;
      require(viewer,"Ordinary Orbit viewer missing"); viewer->move(bounds.right()-40,20); const auto viewerRect=viewer->geometry();
      windowEntry(viewer)->trigger(); app.processEvents();
      require(viewer->geometry()==viewerRect && panel->hasChanges() && editor->toPlainText()==source,"Settings reachability altered explicit viewer placement");
      panel->discardChanges(); child->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      std::cout<<"PASS actual unified XY creation and one short run retain its settings owner; later layout minimum growth/outside minimize-restore through Window and Resources keep Close reachable, pending settings/source/report and other window/viewer geometry unchanged. Offscreen geometry qualification only.\n";
      return 0;
   } catch (const std::exception &error) { std::cerr<<"FAIL "<<error.what()<<'\n'; return 1; }
}
