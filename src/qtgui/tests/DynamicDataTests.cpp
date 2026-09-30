#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "DynamicDataDialog.hpp"
#include "CommandForm.hpp"
#include "QtPlotReceiver.hpp"
#include "DynamicDataDisplay.hpp"
#include "Moderator.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QPlainTextEdit>
#include <QDockWidget>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QColorDialog>
#include <QLabel>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTemporaryDir>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QWindow>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <functional>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QString diagnostics(MainWindow &window) { QApplication::processEvents(); return window.findChild<QDockWidget *>("messages")->findChild<QPlainTextEdit *>()->toPlainText().right(3500); }
static void run(MainWindow &window) { if (window.runMission()!=MainWindow::RunResult::Completed) throw std::runtime_error(("Mission failed: "+diagnostics(window)).toStdString()); }
static void later(QWidget *owner,std::exception_ptr &failure,std::function<void()> action) { QTimer::singleShot(0,owner,[owner,&failure,action] { try { action(); } catch (...) { failure=std::current_exception(); for (auto *dialog:owner->findChildren<QDialog *>()) dialog->reject(); } }); }
static void close(QDialog *dialog,bool accept=true) { dialog->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click(); }
static bool waitUntil(const std::function<bool()> &ready) { if (ready()) return true; QEventLoop loop; QElapsedTimer elapsed; elapsed.start(); QTimer timer; timer.setInterval(10); QObject::connect(&timer,&QTimer::timeout,&loop,[&] { if (ready() || elapsed.elapsed()>3000) loop.quit(); }); timer.start(); loop.exec(); return ready(); }
static QTableWidget *live(MainWindow &window) { require(window.plotReceiver()->show("Display"),"Live display unavailable"); auto *area=window.findChild<QMdiArea *>(); auto *table=qobject_cast<QTableWidget *>(area->activeSubWindow()->widget()); require(table,"Live table missing"); return table; }
static QByteArray bytes(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Numeric report missing"); return file.readAll(); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setAttribute(Qt::AA_DontUseNativeDialogs); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtDynamicData"); if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Fixtures unavailable"); MainWindow window; window.show(); require(window.initialize(startup),"Runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); const auto report=files.filePath("values.txt"),saved=files.filePath("dynamic ü.script");
      const QString resources="Create Spacecraft Sat;\nCreate Variable V W;\nV = 0;\nW = 0;\nCreate String Label;\nLabel = 'Ready';\nCreate Array A[1,2];\nA(1,1) = 5;\nA(1,2) = 9;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 16;\nCreate DynamicDataDisplay Display;\n";
      const QString mission="BeginMissionSequence;\nV = 0.5;\nW = 100;\nLabel = 'Active';\nA(1,2) = 15;\nUpdateDynamicData 'First refresh' Display;\nV = 1.5;\nW = 200;\nLabel = 'Stale';\nA(1,2) = 99;\nUpdateDynamicData 'Selected refresh' Display V; % retain selection comment\nReport Values V W A(1,2);\n";
      const QString reference=resources+"Display.AddParameters = {1, V, W, Label};\nDisplay.AddParameters = {2, A(1,2), '', Sat.ElapsedSecs};\nDisplay.WarnColor = [200 120 0];\nDisplay.CritColor = [180 0 60];\nDisplay.WarnBounds = {V, [-1 1]};\nDisplay.CritBounds = {V, [-2 2]};\nDisplay.TextColor = {W, [20 80 150]};\nDisplay.BackgroundColor = {Label, [240 245 250]};\nDisplay.WarnBounds = {A(1,2), [0 10]};\nDisplay.CritBounds = {A(1,2), [-10 20]};\n"+mission;
      editor->setPlainText(reference); require(window.buildScript(),"Reference build failed"); run(window); const auto expected=bytes(report); auto check=[&] { auto *table=live(window); require(table->rowCount()==2 && table->columnCount()==3,"Dynamic grid shape changed"); require(table->item(0,0)->text().startsWith("V = ") && table->item(0,0)->text().section(" = ",1).toDouble()==1.5 && table->item(0,0)->foreground().color()==QColor(200,120,0),qPrintable("Selected warning update wrong: "+table->item(0,0)->text()+"; color "+table->item(0,0)->foreground().color().name())); require(table->item(0,1)->text().section(" = ",1).toDouble()==100 && table->item(0,1)->foreground().color()==QColor(20,80,150),"Custom color or unselected value changed"); require(table->item(0,2)->text()=="Label = Active" && table->item(0,2)->background().color()==QColor(240,245,250),"String/background update wrong"); require(table->item(1,0)->text().section(" = ",1).toDouble()==15 && table->item(1,0)->foreground().color()==QColor(200,120,0),"Array selective update wrong"); require(table->item(1,1)->text().isEmpty(),"Blank cell lost"); require(table->horizontalHeader()->sectionResizeMode(0)==QHeaderView::Interactive,"Live widths not adjustable"); }; check();
      editor->setPlainText(resources+"Display.AddParameters = {1, V};\n"+mission); require(window.buildScript(),"Editable build failed"); const auto source=editor->toPlainText(); QString error="Not applied"; std::exception_ptr failure;
      {
         QWidget owner; auto *object=Moderator::Instance()->GetConfiguredObject("Display"); ResourceEditor panel(*object,[&](const auto &changes) { error=window.applyResourceChanges("Display",changes,source); return error; },&owner,source); auto *setup=panel.findChild<QPushButton *>("editDynamicData"); require(setup,"Dynamic setup missing");
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("dynamicDataDialog"); dialog->findChild<QSpinBox *>("dynamicRows")->setValue(5); dialog->findChild<QPushButton *>("dynamicResize")->click(); close(dialog,false); }); setup->click(); if (failure) std::rethrow_exception(failure); require(!panel.hasChanges(),"Cancel changed pending grid");
         later(&panel,failure,[&] {
            auto *dialog=panel.findChild<QDialog *>("dynamicDataDialog"); require(waitUntil([dialog] { return dialog->windowHandle() && dialog->windowHandle()->isExposed(); }),"Setup not exposed"); auto *grid=dialog->findChild<QTableWidget *>("dynamicGrid"); dialog->findChild<QSpinBox *>("dynamicRows")->setValue(2); dialog->findChild<QSpinBox *>("dynamicColumns")->setValue(3); dialog->findChild<QPushButton *>("dynamicResize")->click(); require(grid->item(0,0)->text()=="V" && grid->item(1,2)->text().isEmpty(),"Resize lost retained/default cells");
            auto pickColor=[&](QWidget *owner,const QString &button,const QColor &color) { later(owner,failure,[&,owner,color] { auto *picker=owner->findChild<QColorDialog *>("dynamicColorPicker"); require(picker,"Color picker missing"); picker->setCurrentColor(color); picker->accept(); }); owner->findChild<QPushButton *>(button)->click(); if (failure) std::rethrow_exception(failure); };
            auto edit=[&](int r,int c,const QString &name,const QStringList &bounds={},bool custom=false,bool background=false) {
               grid->setCurrentCell(r,c); later(dialog,failure,[&,name,bounds,custom,background] {
                  auto *cell=dialog->findChild<QDialog *>("dynamicCellDialog"); require(cell,"Cell editor missing");
                  later(cell,failure,[&] { auto *picker=cell->findChild<QDialog *>("reportParameterDialog"); require(picker,"Cell parameter browser missing"); picker->findChild<QComboBox *>("reportParameterEntry")->setCurrentText(name); close(picker); }); cell->findChild<QPushButton *>("dynamicSelectParameter")->click(); if (failure) std::rethrow_exception(failure);
                  if (!bounds.isEmpty()) { const QStringList names={"warnLower","warnUpper","critLower","critUpper"}; for (int i=0;i<4;++i) cell->findChild<QLineEdit *>("dynamic_"+names[i])->setText(bounds[i]); }
                  if (custom) pickColor(cell,"dynamicTextColor",QColor(20,80,150)); if (background) pickColor(cell,"dynamicBackgroundColor",QColor(240,245,250));
                  close(cell); require(!cell->isVisible(),qPrintable(cell->findChild<QLabel *>("dynamicCellError")->text()));
               }); dialog->findChild<QPushButton *>("dynamicEditCell")->click(); if (failure) std::rethrow_exception(failure);
            };
            edit(0,0,"V",{"-1","1","-2","2"}); edit(0,1,"W",{},true); edit(0,2,"Label",{},false,true); edit(1,0,"A(1,2)",{"0","10","-10","20"}); edit(1,2,"Sat.ElapsedSecs");
            grid->setCurrentCell(1,1); later(dialog,failure,[&] { auto *cell=dialog->findChild<QDialog *>("dynamicCellDialog"); cell->findChild<QLineEdit *>("dynamicParameter")->setText("V"); close(cell); require(cell->isVisible(),"Duplicate parameter accepted"); cell->findChild<QLineEdit *>("dynamicParameter")->setText("A"); close(cell); require(cell->isVisible(),"Whole array accepted"); cell->findChild<QLineEdit *>("dynamicParameter")->setText("Sat.EarthMJ2000Eq.X"); cell->findChild<QLineEdit *>("dynamic_warnLower")->setText("bad"); close(cell); require(cell->isVisible(),"Nonnumeric bound accepted"); cell->findChild<QLineEdit *>("dynamic_warnLower")->setText("2"); cell->findChild<QLineEdit *>("dynamic_warnUpper")->setText("1"); close(cell); require(cell->isVisible(),"Reversed warning bounds accepted"); close(cell,false); }); dialog->findChild<QPushButton *>("dynamicEditCell")->click(); if (failure) std::rethrow_exception(failure); require(grid->item(1,1)->text().isEmpty(),"Cell Cancel changed blank selection");
            edit(1,1,"Sat.EarthMJ2000Eq.X",{},false,true); grid->setCurrentCell(1,1); dialog->findChild<QPushButton *>("dynamicClear")->click(); require(grid->item(1,1)->text().isEmpty() && grid->item(1,1)->background().color()==QColor(Qt::white),"Clear selection did not restore blank defaults");
            pickColor(dialog,"dynamic_WarnColor",QColor(200,120,0)); pickColor(dialog,"dynamic_CritColor",QColor(180,0,60)); grid->setColumnWidth(0,220); dialog->findChild<QSpinBox *>("dynamicRows")->setValue(3); dialog->findChild<QPushButton *>("dynamicResize")->click(); require(grid->columnWidth(0)==220,"Grid resize overwrote manual width"); dialog->findChild<QSpinBox *>("dynamicRows")->setValue(2); dialog->findChild<QPushButton *>("dynamicResize")->click();
            dialog->resize(760,400); require(waitUntil([dialog] { return dialog->height()<=400; }),"Compact grid resize not acknowledged"); auto *ok=dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok); require(dialog->rect().contains(ok->mapTo(dialog,ok->rect().bottomRight())),"Compact setup actions inaccessible");
            if (!capture.isEmpty()) { dialog->resize(760,540); require(waitUntil([dialog] { return dialog->height()==540; }),"Capture not resized"); require(dialog->grab().save(capture),"Capture failed"); } close(dialog);
         }); setup->click(); if (failure) std::rethrow_exception(failure); require(panel.hasChanges() && dynamic_cast<DynamicDataDisplay *>(object)->GetDynamicDataStruct().size()==1,"Setup changed engine before Apply");
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("dynamicDataDialog"); auto *grid=dialog->findChild<QTableWidget *>("dynamicGrid"); require(grid->rowCount()==2 && grid->item(0,2)->text()=="Label","Pending reopen lost grid"); close(dialog,false); }); setup->click(); if (failure) std::rethrow_exception(failure); panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
      }
      const auto applied=editor->toPlainText(); require(applied.endsWith(mission),"Apply changed mission/comments"); editor->undo(); require(editor->toPlainText()==source,"Undo not exact"); editor->redo(); require(editor->toPlainText()==applied,"Redo not exact"); require(window.saveScriptTo(saved) && window.loadScript(saved),"Unicode save/reopen failed"); run(window); require(bytes(report)==expected,"GUI grid changed numerical report"); check(); auto *area=window.findChild<QMdiArea *>(); area->activeSubWindow()->close(); require(window.plotReceiver()->show("Display"),"Closed display cannot reopen"); check();
      const auto current=editor->toPlainText(); auto cells=QJsonDocument::fromJson(dynamicDataSettings(*Moderator::Instance()->GetConfiguredObject("Display")).toUtf8()).array(); auto row=cells[0].toArray(); auto cell=row[0].toObject(); cell["name"]="DoesNotExist"; row[0]=cell; cells[0]=row; const auto bad=QString::fromUtf8(QJsonDocument(cells).toJson(QJsonDocument::Compact)); require(!window.applyResourceChanges("Display",{{"@DynamicData",bad}},current).isEmpty() && editor->toPlainText()==current,"Missing parameter did not roll back"); run(window); check();
      QString changed; const QString command="UpdateDynamicData 'Selected refresh' Display; % retain selection comment"; CommandForm form([&](const QString &text) { changed=text; }); form.setStatement(command); require(form.findChild<QPushButton *>("commandChoose_Parameters"),"Dynamic command selectors missing"); later(&form,failure,[&] { auto *dialog=form.findChild<QDialog *>("dynamicUpdateDialog"); auto *list=dialog->findChild<QListWidget *>("dynamicUpdateList"); require(list->count()==5,"Update list includes missing/non-grid resources"); list->findItems("V",Qt::MatchExactly).first()->setCheckState(Qt::Checked); close(dialog); }); form.findChild<QPushButton *>("commandChoose_Parameters")->click(); if (failure) std::rethrow_exception(failure); require(changed=="UpdateDynamicData 'Selected refresh' Display V; % retain selection comment","Update selection changed labels/comments or spacing");
      auto clearUpdates=[&](bool accept) { later(&form,failure,[&,accept] { auto *dialog=form.findChild<QDialog *>("dynamicUpdateDialog"); auto *list=dialog->findChild<QListWidget *>("dynamicUpdateList"); for (int i=0;i<list->count();++i) list->item(i)->setCheckState(Qt::Unchecked); close(dialog,accept); }); form.findChild<QPushButton *>("commandChoose_Parameters")->click(); if (failure) std::rethrow_exception(failure); };
      const auto selective=changed; clearUpdates(false); require(changed==selective,"Update picker Cancel changed command"); clearUpdates(true); require(changed==command,"Empty selection did not restore all-cell command");
      const auto snapshot=window.missionSnapshot(); int update=-1; for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type=="UpdateDynamicData" && snapshot.nodes[i].statement.contains("Selected refresh")) update=i;
      require(update>=0 && window.applyMissionChange(snapshot,update,MissionEdit::Replace,changed).isEmpty(),"All-cell form command failed Apply");
      require(window.saveScriptTo(files.filePath("all-updates.script")) && window.loadScript(files.filePath("all-updates.script")),"Command save/reopen failed"); run(window); require(bytes(report)==expected,"Update command changed numeric report"); auto *all=live(window); require(all->item(0,1)->text().section(" = ",1).toDouble()==200 && all->item(0,2)->text()=="Label = Stale" && all->item(1,0)->text().section(" = ",1).toDouble()==99,"All-cell command did not refresh previously unselected values");
      require(window.loadScript(saved),"Selective command restore failed"); run(window); check();
      for (const auto &value:QStringList{"0.5","1","3","-3"}) { auto candidate=current; candidate.replace("V = 1.5;","V = "+value+";"); editor->setPlainText(candidate); run(window); auto *table=live(window); require(table->item(0,0)->foreground().color()==(value=="3" || value=="-3" ? QColor(180,0,60) : QColor(Qt::black)),"Normal/boundary/critical condition color wrong"); }
      auto ragged=dynamic_cast<DynamicDataDisplay *>(Moderator::Instance()->GetConfiguredObject("Display"))->GetDynamicDataStruct(); ragged[1].resize(1); ragged[1][0].paramValue="17"; auto *table=live(window); table->setColumnWidth(0,240); require(window.plotReceiver()->UpdateDynamicDataDisplay("Display",ragged),"Ragged callback rejected"); require(table->columnWidth(0)==240 && !table->item(1,2),"Callback retained stale padding or changed manual width"); area->activeSubWindow()->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); ragged[1][0].paramValue="18"; require(window.plotReceiver()->UpdateDynamicDataDisplay("Display",ragged),"Closed-table callback rejected"); table=live(window); require(table->item(1,0)->text()=="A(1,2) = 18" && table->item(1,2)->text().isEmpty(),"Reopen lost closed-table update or retained stale padding");
      require(window.loadScript(saved),"Recovery reopen failed"); run(window); check();
      // Clear all and execute an empty table using all-cell updates.
      auto empty=editor->toPlainText(); empty.replace("UpdateDynamicData 'Selected refresh' Display V;","UpdateDynamicData 'Selected refresh' Display;"); editor->setPlainText(empty); require(window.buildScript(),"Empty-table mission failed build"); error=window.applyResourceChanges("Display",{{"@DynamicData","[]"}},editor->toPlainText()); require(error.isEmpty(),qPrintable(error)); require(window.saveScriptTo(files.filePath("empty.script")) && window.loadScript(files.filePath("empty.script")),"Empty table round trip failed"); run(window); require(live(window)->rowCount()==0,"Empty table retained cells");
      std::cout<<"PASS: dynamic setup/cells/colors/bounds, reference selection, resize/clear/blank cells, pending/Cancel/reopen, invalid input rollback, Undo/Redo/Unicode round trips, unchanged numeric reports, selective/string/array updates, normal/boundary/warning/critical/custom colors, immediate close/reopen, ragged/closed-table callbacks and adjustable widths, command selection/Cancel/Apply/save/reopen, empty-table execution/recovery\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; } return 0;
}
