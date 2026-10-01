#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLocale>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QWindow>
#include <sys/resource.h>
#include <signal.h>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Export evidence file unavailable"); return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path); require(f.open(QIODevice::WriteOnly) && f.write(bytes)==bytes.size(),"Export fixture write failed"); }
static void run(MainWindow &window) { require(window.runMission()==MainWindow::RunResult::Completed,"XY export mission failed"); }
struct FileSizeLimit {
   rlimit saved; decltype(SIG_IGN) handler;
   FileSizeLimit() { require(getrlimit(RLIMIT_FSIZE,&saved)==0,"Cannot read export write limit"); handler=signal(SIGXFSZ,SIG_IGN); require(handler!=SIG_ERR,"Cannot handle export write failure"); auto limit=saved; limit.rlim_cur=128; require(setrlimit(RLIMIT_FSIZE,&limit)==0,"Cannot set export write failure limit"); }
   ~FileSizeLimit() { setrlimit(RLIMIT_FSIZE,&saved); signal(SIGXFSZ,handler); }
};
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtXYExport");
   try {
      const bool preview=argc==4 && QString::fromLocal8Bit(argv[2])=="--preview"; TestSettings settings; QTemporaryDir files; require((argc==2 || preview) && files.isValid(),"XY export setup failed"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"XY export runtime unavailable"); auto *area=window.findChild<QMdiArea *>("workspace"); auto *receiver=window.plotReceiver(); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); const auto saved=files.filePath("XY export Δ.script"),report=files.filePath("engine report.txt"),output=files.filePath("retained curves Δ.txt");
      if (preview) {
         receiver->CreateXyPlotWindow("Preview","",0,0,0,0,false,"Retained XY data","X","Y",true); receiver->AddXyPlotCurve("Preview",0,"Curve",0xff0000); receiver->UpdateXyPlotCurve("Preview",0,1,2); require(receiver->show("Preview"),"Native export preview plot missing"); auto *plot=dynamic_cast<PlotWidget *>(area->activeSubWindow()->widget()); std::exception_ptr failure;
         QTimer::singleShot(0,plot,[&] { auto *dialog=dynamic_cast<QFileDialog *>(QApplication::activeModalWidget()); try { require(dialog,"Native export picker missing"); dialog->selectFile(output); QEventLoop wait; QTimer::singleShot(200,&wait,&QEventLoop::quit); wait.exec(); require(dialog->windowHandle() && dialog->windowHandle()->isExposed() && dialog->grab().save(QString::fromLocal8Bit(argv[3])+".picker.png"),"Native export picker capture failed"); require(plot->grab().save(QString::fromLocal8Bit(argv[3])+".plot.png"),"Native export toolbar capture failed"); } catch (...) { failure=std::current_exception(); } if (dialog) dialog->reject(); }); plot->findChild<QAction *>("plotExportData")->trigger(); if (failure) std::rethrow_exception(failure); require(!QFileInfo::exists(output),"Native preview Cancel wrote data"); std::cout<<"PASS: native actual MDI XY export action/widget picker exposure and Cancel; no numerical execution or portal qualification.\n"; return 0;
      }
      const QString source="% retain export-independent mission α\nCreate Spacecraft Sat;\nCreate Propagator PS;\nPS.InitialStepSize = 5;\nCreate XYPlot Chart;\nChart.XVariable = Sat.ElapsedSecs;\nChart.YVariables = {Sat.EarthMJ2000Eq.X};\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 17;\nBeginMissionSequence;\nPropagate PS(Sat) {Sat.ElapsedSecs = 60};\nReport Values Sat.ElapsedSecs Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z;\n";
      editor->setPlainText(source); run(window); const auto expectedReport=read(report); require(window.saveScriptTo(saved) && receiver->show("Chart"),"XY export actual plot/save missing"); auto model=receiver->model("Chart"); auto *child=area->activeSubWindow(); auto *plot=dynamic_cast<PlotWidget *>(child->widget()); require(plot && plot->findChild<QAction *>("plotExportData"),"XY Export action missing"); const auto points=model->curves[0].points.size(); require(points>3,"Propagation did not publish actual XY samples");
      std::exception_ptr styleFailure; QTimer::singleShot(0,plot,[&] { auto *dialog=plot->findChild<QDialog *>("plotStyleDialog"); try { require(dialog,"Populated XY style editor missing"); dialog->findChild<QCheckBox *>("plotXAxisMinimumEnabled")->setChecked(true); dialog->findChild<QLineEdit *>("plotXAxisMinimum")->setText("100"); dialog->findChild<QSpinBox *>("plotXAxisPrecision")->setValue(2); dialog->findChild<QWidget *>("curveStyle_0")->findChild<QCheckBox *>("Visible")->setChecked(false); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); } catch (...) { styleFailure=std::current_exception(); if (dialog) dialog->reject(); } }); plot->findChild<QAction *>("plotStyleAction")->trigger(); if (styleFailure) std::rethrow_exception(styleFailure); require(model->xAxis.minimum==100 && !model->curves[0].visible && model->curves[0].points.size()==points,"Populated XY display changes lost samples"); const auto before=plot->canvas()->captureImage();
      auto chooser=[&](bool accept) { std::exception_ptr failure; QTimer::singleShot(0,plot,[&] { auto *dialog=dynamic_cast<QFileDialog *>(QApplication::activeModalWidget()); try { require(dialog,"XY export file chooser missing"); dialog->selectFile(output); if (accept) QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection); else dialog->reject(); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } }); plot->findChild<QAction *>("plotExportData")->trigger(); if (failure) std::rethrow_exception(failure); };
      chooser(false); require(!QFileInfo::exists(output) && editor->toPlainText()==source,"Export Cancel wrote/changed mission"); chooser(true); const auto exported=read(output); const auto lines=QString::fromUtf8(exported).split('\n'); require(lines.size()>3 && lines[0]==model->title && lines[1]==model->xLabel+"   "+model->yLabel && lines[2]==model->curves[0].name,"XY export wx-compatible headers missing"); int count=0; for (int i=3;i<lines.size();++i) if (!lines[i].isEmpty()) { const auto values=lines[i].split(','); require(values.size()==2,"XY export is not ordered X,Y pairs"); bool xOk=false,yOk=false; const double x=values[0].toDouble(&xOk),y=values[1].toDouble(&yOk); require(xOk && yOk && count<int(points) && x==model->curves[0].points[count].x && y==model->curves[0].points[count].y,"XY export changed/omitted stored samples"); ++count; } require(count==int(points),"XY export truncated retained samples");
      require(plot->canvas()->captureImage()==before && editor->toPlainText()==source && read(saved)==source.toUtf8() && read(report)==expectedReport,"Export altered display/source/calculation report"); const auto linked=files.filePath("mission link.script"); require(QFile::link(saved,linked),"Export alias fixture failed"); for (const auto &path:QStringList{saved,linked,report,startup}) require(!plot->exportData(path).isEmpty(),"Plot export overwrote mission/output/startup"); require(read(saved)==source.toUtf8() && read(report)==expectedReport,"Protected export changed original files");
      const auto failed=files.filePath("atomic failure.txt"); write(failed,"retain original bytes\n"); QString error; { FileSizeLimit limit; error=plot->exportData(failed); } if (error.isEmpty() || read(failed)!="retain original bytes\n") std::cerr<<"Export failure diagnostic: points="<<points<<", export bytes="<<exported.size()<<", error="<<error.toStdString()<<", destination bytes="<<read(failed).size()<<"\n"; require(!error.isEmpty() && read(failed)=="retain original bytes\n","Post-write export failure did not preserve old destination"); require(!plot->exportData(files.filePath("missing/export.txt")).isEmpty(),"Missing-directory export succeeded"); require(plot->exportData(failed).isEmpty() && read(failed)==exported,"Corrected export did not recover");
      child->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); require(receiver->show("Chart") && receiver->model("Chart").get()==model.get(),"Export close/reopen lost retained model"); plot=dynamic_cast<PlotWidget *>(area->activeSubWindow()->widget()); require(plot->exportData(output).isEmpty() && read(output)==exported,"Reopened viewer export changed data");
      require(window.loadScript(saved),"Export mission reopen failed"); run(window); require(read(report)==expectedReport,"Export save/reopen/rerun changed calculation");
      // Full precision and every retained curve are independent of display
      // visibility/range/label precision, with C-locale decimal syntax.
      auto fixture=std::make_shared<PlotModel>(PlotModel::Kind::XY); fixture->title="Synthetic Δ"; fixture->xLabel="Time"; fixture->yLabel="Value"; fixture->curves[2].name="Hidden Δ"; fixture->curves[2].visible=false; fixture->curves[7].name="Empty"; const double x=0.12345678901234566,y=-9.876543210987654; fixture->append(2,x,y); fixture->xAxis.minimum=100; fixture->xAxis.maximum=200; fixture->xAxis.precision=2; PlotWidget synthetic(fixture); const auto locale=QLocale(); QLocale::setDefault(QLocale(QLocale::German)); error=synthetic.exportData(output); QLocale::setDefault(locale); require(error.isEmpty(),"Locale-independent data export failed"); const auto text=QString::fromUtf8(read(output)); require(text.startsWith("Synthetic Δ\nTime   Value\nHidden Δ\n") && text.endsWith("\nEmpty\n\n"),"Hidden/empty curve sections omitted"); const auto pair=text.split('\n')[3].split(','); require(pair.size()==2 && pair[0].toDouble()==x && pair[1].toDouble()==y,"Export lost double precision or used locale/visible range");
      std::cout<<"PASS: actual MDI XY export chooser Cancel/accept, retained ordered full-precision samples and wx headers, Unicode save/reopen, source/output/startup alias protection, atomic post-write failure and correction, close/reopen/rerun report invariance; hidden/empty/nonconsecutive curves and C-locale data independent of display range/precision.\n";
   } catch (BaseException &e) { std::cerr<<"FAIL: "<<e.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
