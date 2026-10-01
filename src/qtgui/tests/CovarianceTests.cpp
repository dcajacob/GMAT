#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "ResourceProperties.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QTableWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QDialog>
#include <QLabel>
#include <QSpinBox>
#include <QHeaderView>
#include <QWindow>
#include <QTimer>
#include <QEventLoop>
#include <QRegularExpression>
#include <QTextDocument>
#include <iostream>
#include <stdexcept>
static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Covariance fixture/report unavailable"); return QString::fromUtf8(file.readAll()); }
static void write(const QString &path,const QString &text) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(text.toUtf8())==text.toUtf8().size(),"Covariance fixture write failed"); }
static void dialogAction(MainWindow &window,std::function<void()> open,std::function<void(QDialog *)> inspect)
{
   std::exception_ptr failure;
   QTimer::singleShot(0,&window,[&] { auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); try { require(dialog,"Covariance dialog missing"); inspect(dialog); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } });
   open(); if (failure) std::rethrow_exception(failure);
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtCovariance");
   try {
      const bool preview=argc>3 && QString::fromLocal8Bit(argv[3])=="--preview";
      TestSettings settings; QTemporaryDir files; require(files.isValid() && argc>1,"Covariance setup failed");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(),capture=argc>2 ? QFileInfo(QString::fromLocal8Bit(argv[2])).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto warm=files.filePath("covariance.csv"),state=files.filePath("states.txt"),saved=files.filePath("covariance ü.script"),basePath=files.filePath("initial.script");
      auto source=read("../samples/Navigation/Ex_FilterSmoother_GpsPosVec.script");
      const auto replace=[&](const QString &key,const QString &value) { const QRegularExpression field("^"+QRegularExpression::escape(key)+"[ \\t]*=[^;\\n]*;?",QRegularExpression::MultilineOption); require(field.match(source).hasMatch(),"Covariance sample setting missing"); source.replace(field,key+" = "+value+";"); };
      source.replace("'Ex_FilterSmoother_GpsPosVec.gmd'","'"+files.filePath("observations.gmd")+"'"); replace("Sim.FinalEpoch","'10 Jun 2010 00:10:00.000'"); replace("Sim.AddNoise","Off"); replace("EKF.ReportFile","'"+files.filePath("filter.txt")+"'"); replace("FPS.ReportFile","'"+files.filePath("smoother.txt")+"'"); replace("EKF.OutputWarmStartFile","'"+warm+"'"); replace("FilterCdReport.Filename","'"+files.filePath("drag.csv")+"'");
      // The shipped example sets covariance at runtime after simulation. Move
      // that initializer into configuration so the GUI owns the cold-start input.
      source.remove(QRegularExpression("^EstSat\\.OrbitErrorCovariance[^\\n]*\\n",QRegularExpression::MultilineOption)); source.remove(QRegularExpression("^RunSmoother[^\\n]*\\n",QRegularExpression::MultilineOption));
      const QString initial="EstSat.OrbitErrorCovariance = [0.01 0 0 0 0 0; 0 0.01 0 0 0 0; 0 0 0.01 0 0 0; 0 0 0 2.5e-7 0 0; 0 0 0 0 2.5e-7 0; 0 0 0 0 0 2.5e-7]; % initial covariance literal α\n";
      source.replace("BeginMissionSequence",initial+"Create ReportFile States;\nStates.Filename = '"+state+"';\nStates.WriteHeaders = false;\nStates.Precision = 16;\nBeginMissionSequence");
      source.replace("RunEstimator EKF;","RunEstimator EKF;\nReport States EstSat.X EstSat.Y EstSat.Z EstSat.VX EstSat.VY EstSat.VZ EstSat.Cd;");
      const QString matrix="0.02 0.005 0 0 0 0; 0.005 0.03 0 0 0 0; 0 0 0.04 0 0 0; 0 0 0 5e-7 1e-7 0; 0 0 0 1e-7 6e-7 0; 0 0 0 0 0 7e-7";
      auto direct=source; direct.replace(initial,"EstSat.OrbitErrorCovariance = ["+matrix+"]; % independent correlated covariance\n");
      MainWindow window; window.show(); require(window.initialize(startup),"Covariance runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto run=[&] { require(window.runMission()==MainWindow::RunResult::Completed,qPrintable(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().right(3000))); };
      QString expectedCovariance,expectedState; if (!preview) { editor->setPlainText(direct); run(); expectedCovariance=read(warm); expectedState=read(state); require(expectedCovariance.split('\n').size()>2 && expectedState.trimmed().split(QRegularExpression("\\s+")).size()==7,"Independent covariance/state outputs missing"); }
      write(basePath,source); require(window.loadScript(basePath) && window.buildScript(),"Covariance GUI fixture failed");
      auto *tree=window.findChild<QTreeWidget *>("Resources"); auto *area=window.findChild<QMdiArea *>("workspace"); const auto items=tree->findItems("EstSat",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Covariance spacecraft missing"); tree->itemDoubleClicked(items.first(),0);
      auto *child=area->activeSubWindow(); auto *panel=child->widget(); auto *properties=panel->findChild<QTableWidget *>("resourceProperties"); auto *edit=panel->findChild<QPushButton *>("editCells_OrbitErrorCovariance"); require(edit && properties,"Orbit covariance cells unavailable");
      int covarianceRow=-1,solveForRow=-1; for (int row=0;row<properties->rowCount();++row) { if (properties->item(row,0)->text()=="OrbitErrorCovariance") covarianceRow=row; if (properties->item(row,0)->text()=="SolveFors") solveForRow=row; }
      require(covarianceRow>=0 && solveForRow>=0,"Covariance properties missing"); const auto untouched=properties->item(covarianceRow,1)->text(),solveFors=properties->item(solveForRow,1)->text();
      if (preview) {
         require(!capture.isEmpty(),"Covariance preview needs an output path");
         dialogAction(window,[&] { edit->click(); },[&](QDialog *dialog) { auto *grid=dialog->findChild<QTableWidget *>("numericGrid"); require(grid && grid->horizontalHeaderItem(0)->text()=="X (km)","Native covariance labels missing"); QEventLoop loop; QTimer::singleShot(200,&loop,&QEventLoop::quit); loop.exec(); require(grid->horizontalHeader()->length()<=grid->viewport()->width(),"Native covariance columns overflow their initial viewport"); const auto raw=untouched.split(';')[3].trimmed().split(QRegularExpression("\\s+"))[3]; require(grid->item(3,3)->text()==raw && grid->item(3,3)->toolTip()==raw,"Covariance display formatting rounded stored digits"); require(dialog->windowHandle() && dialog->windowHandle()->isExposed() && dialog->grab().save(capture),"Native covariance editor not exposed or capture failed"); dialog->reject(); });
         std::cout<<"PASS: exposed native covariance editor preview with six visible columns and full stored precision; numerical workflow was not repeated.\n"; return 0;
      }
      dialogAction(window,[&] { edit->click(); },[](QDialog *dialog) { auto *grid=dialog->findChild<QTableWidget *>("numericGrid"); require(grid && grid->rowCount()==6 && grid->columnCount()==6 && !dialog->findChild<QSpinBox *>(),"Covariance matrix dimensions are editable"); require(grid->horizontalHeaderItem(0)->text()=="X (km)" && grid->verticalHeaderItem(5)->text()=="VZ (km/s)","Cartesian covariance state/units missing"); grid->item(0,0)->setText("42"); dialog->reject(); }); require(properties->item(covarianceRow,1)->text()==untouched && editor->toPlainText()==source,"Covariance Cancel changed pending/source values");
      // Labels follow pending solve-fors without converting matrix values.
      properties->item(solveForRow,1)->setText("KeplerianState");
      dialogAction(window,[&] { edit->click(); },[](QDialog *dialog) { auto *grid=dialog->findChild<QTableWidget *>("numericGrid"); require(grid->horizontalHeaderItem(0)->text()=="SMA (km)" && grid->horizontalHeaderItem(5)->text()=="MA (deg)","Keplerian covariance must use mean anomaly"); dialog->reject(); }); properties->item(solveForRow,1)->setText(solveFors);
      dialogAction(window,[&] { edit->click(); },[&](QDialog *dialog) {
         auto *grid=dialog->findChild<QTableWidget *>("numericGrid"); auto *buttons=dialog->findChild<QDialogButtonBox *>(); auto *error=dialog->findChild<QLabel *>("numericGridError"); auto *mirror=dialog->findChild<QPushButton *>("covarianceMirrorUpper"); require(error && mirror && dialog->property("helpTopic").toString()=="SpacecraftNavigation","Covariance validation/symmetry/help controls missing");
         grid->item(0,1)->setText("0.005"); buttons->button(QDialogButtonBox::Ok)->click(); require(dialog->isVisible() && error->text().contains("symmetric"),"Asymmetric covariance accepted"); mirror->click(); require(grid->item(1,0)->text()=="0.005","Explicit covariance mirror failed");
         grid->item(0,0)->setText("-0.02"); buttons->button(QDialogButtonBox::Ok)->click(); require(dialog->isVisible() && error->text().contains("positive definite"),"Indefinite covariance accepted"); grid->item(0,0)->setText("0.02");
         grid->item(2,2)->setText("nan"); buttons->button(QDialogButtonBox::Ok)->click(); require(dialog->isVisible() && error->text().contains("finite"),"Nonfinite covariance accepted");
         const auto rows=matrix.split(';'); for (int r=0;r<6;++r) { const auto cells=rows[r].trimmed().split(' '); for (int c=0;c<6;++c) grid->item(r,c)->setText(cells[c]); }
         const int width=grid->columnWidth(0); grid->setColumnWidth(0,width+25); require(grid->horizontalHeader()->sectionResizeMode(0)==QHeaderView::Interactive && grid->columnWidth(0)==width+25,"Covariance columns are not adjustable");
         if (!capture.isEmpty()) { QEventLoop loop; QTimer::singleShot(200,&loop,&QEventLoop::quit); loop.exec(); require(dialog->windowHandle() && dialog->windowHandle()->isExposed() && dialog->grab().save(capture),"Native covariance editor not exposed or capture failed"); }
         buttons->button(QDialogButtonBox::Ok)->click(); require(dialog->result()==QDialog::Accepted,"Corrected covariance rejected");
      });
      require(editor->toPlainText()==source && properties->item(covarianceRow,1)->text()!=untouched,"Covariance did not remain pending");
      panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); const auto applied=editor->toPlainText(); require(applied!=source && applied.contains("% initial covariance literal α") && child==area->activeSubWindow() && child->isVisible(),"Covariance Apply failed/lost comments or closed panel");
      editor->undo(); require(editor->toPlainText()==source,"Covariance Undo not exact"); editor->redo(); require(editor->toPlainText()==applied,"Covariance Redo not exact");
      run(); require(read(warm)==expectedCovariance && read(state)==expectedState,"GUI covariance changed independent filter state/covariance output");
      require(window.saveScriptTo(saved) && read(saved)==applied && window.loadScript(saved) && window.buildScript(),"Covariance Unicode save/reopen changed source"); run(); require(read(warm)==expectedCovariance && read(state)==expectedState,"Reopened covariance changed filter output");
      auto invalid=matrix; invalid.replace("0.02 0.005","0.02 0.006"); require(!window.applyResourceChanges("EstSat",{{"OrbitErrorCovariance",invalid}},applied).isEmpty() && editor->toPlainText()==applied,"Direct covariance Apply bypassed symmetry validation");
      auto singular=matrix; singular.replace("7e-7","0"); require(!orbitCovarianceError(singular).isEmpty(),"Singular covariance accepted");
      std::cout<<"PASS: fixed 6x6 covariance editor, Cartesian and pending Keplerian/MA labels and units, Cancel and pending Apply, explicit upper/lower symmetry, nonfinite/asymmetric/indefinite/singular recovery, adjustable columns and Help topic, retained MDI panel, exact Undo/Redo and Unicode save/reopen, byte-identical independent ten-minute GPS filter state and covariance CSV, direct Apply protection.\n";
      return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
