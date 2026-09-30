#include "MainWindow.hpp"
#include "ReportParameterDialog.hpp"
#include "BulkPropertyDialog.hpp"
#include "ParameterReferences.hpp"
#include "CommandEditor.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QEventLoop>
#include <QWindow>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Selection report missing"); return file.readAll(); }
static void close(QDialog *dialog,bool accept=true) { dialog->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click(); }
template<class Open,class Inspect> static void modal(QWidget &owner,Open open,Inspect inspect)
{
   std::exception_ptr failure; bool visited=false;
   QTimer::singleShot(0,&owner,[&] { visited=true; auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); try { require(dialog,"Parameter selection modal missing"); inspect(dialog); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } });
   open(); require(visited,"Parameter selection modal unvisited"); if (failure) std::rethrow_exception(failure);
}
static int row(QTableWidget *table,const QString &name)
{
   for (int i=0;i<table->rowCount();++i) if (table->item(i,0)->text()==name) return i;
   return -1;
}
static void selectOwners(BulkPropertyDialog *dialog,const QStringList &names)
{
   auto *list=dialog->findChild<QListWidget *>("bulkPropertyObjects"); list->clearSelection();
   for (const auto &name:names) { const auto found=list->findItems(name,Qt::MatchExactly); require(found.size()==1,"Bulk owner unavailable"); found.first()->setSelected(true); }
}
static void check(BulkPropertyDialog *dialog,const QString &property,const QString &reference={})
{
   auto *table=dialog->findChild<QTableWidget *>("bulkPropertyTable"); const int index=row(table,property); require(index>=0,"Bulk property missing");
   table->item(index,0)->setCheckState(Qt::Checked);
   if (!reference.isEmpty()) { auto *combo=qobject_cast<QComboBox *>(table->cellWidget(index,2)); require(combo && combo->findText(reference)>=0,"Typed bulk reference unavailable"); combo->setCurrentText(reference); }
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtParameterSelection");
   if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Selection fixtures unavailable"); MainWindow window; window.show(); require(window.initialize(startup),"Selection runtime unavailable");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); const auto report=files.filePath("values.txt");
      const QString command="Report 'retain report label' Values V; % retain report comment\n";
      const QString base="% retain bulk source α\nCreate Spacecraft Alpha Beta Empty;\nCreate ChemicalTank TankA TankB;\nAlpha.Tanks = {TankA};\nBeta.Tanks = {TankB};\n"
         "Create CoordinateSystem Fixed;\nFixed.Origin = Earth;\nFixed.Axes = BodyFixed;\nCreate Variable V;\nV = 3;\nCreate String Label;\nLabel = 'text';\nCreate Array Grid[1,2];\nGrid(1,2) = 7;\n"
         "Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 16;\nBeginMissionSequence;\n";
      const QStringList expected{"V","Alpha.EarthMJ2000Eq.X","Alpha.EarthMJ2000Eq.Y","Beta.EarthMJ2000Eq.X","Beta.EarthMJ2000Eq.Y"};
      editor->setPlainText(base+"Report Values "+expected.join(' ')+";\n"); require(window.runMission()==MainWindow::RunResult::Completed,"Independent bulk reference failed"); const auto reference=read(report);
      editor->setPlainText(base+command); require(window.buildScript(),"Bulk base build failed"); const auto source=editor->toPlainText();
      auto *tree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItem *reportItem=nullptr;
      for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole+1)=="Report") reportItem=*it;
      require(reportItem,"Report command unavailable"); tree->itemDoubleClicked(reportItem,0);
      CommandEditor *panel=nullptr; for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (auto *candidate=dynamic_cast<CommandEditor *>(child->widget())) panel=candidate;
      require(panel,"Real report command editor missing"); auto *choose=panel->findChild<QPushButton *>("commandChoose_Parameters"); require(choose,"Report selector action missing");
      modal(*panel,[&] { choose->click(); },[&](QDialog *generic) {
         auto *picker=dynamic_cast<ReportParameterDialog *>(generic); require(picker,"Report picker missing");
         picker->findChild<QComboBox *>("reportPropertyObject")->setCurrentText("Alpha"); picker->findChild<QComboBox *>("reportPropertyType")->setCurrentText("X");
         auto open=[&] { picker->findChild<QPushButton *>("reportBulkProperties")->click(); };
         modal(*picker,open,[&](QDialog *generic) { auto *bulk=dynamic_cast<BulkPropertyDialog *>(generic); require(bulk,"Bulk dialog missing"); selectOwners(bulk,{"Alpha","Beta"}); check(bulk,"Y","EarthMJ2000Eq"); close(bulk,false); });
         require(picker->selection()==QStringList{"V"} && editor->toPlainText()==source,"Bulk Cancel changed pending selection or script");
         for (int repeat=0;repeat<2;++repeat) modal(*picker,open,[&](QDialog *generic) {
            auto *bulk=dynamic_cast<BulkPropertyDialog *>(generic); selectOwners(bulk,{"Alpha","Beta"}); bulk->findChild<QPushButton *>("bulkUncheckAll")->click();
            check(bulk,"X","EarthMJ2000Eq"); check(bulk,"Y","EarthMJ2000Eq");
            auto *table=bulk->findChild<QTableWidget *>("bulkPropertyTable"); require(table->horizontalHeader()->sectionResizeMode(0)==QHeaderView::Interactive,"Bulk columns locked"); table->setColumnWidth(0,230); require(table->columnWidth(0)==230,"Bulk column resize failed");
            require(bulk->selection()==expected.mid(1),"Multiple owner/property reference order wrong");
            table->scrollToItem(table->item(row(table,"X"),0),QAbstractItemView::PositionAtCenter);
            if (!capture.isEmpty() && repeat==0) { QEventLoop loop; QTimer::singleShot(250,&loop,&QEventLoop::quit); loop.exec(); require(bulk->windowHandle() && bulk->windowHandle()->isExposed() && bulk->grab().save(capture+".bulk.png"),"Native bulk window not exposed"); }
            close(bulk);
         });
         require(picker->selection()==expected,"Bulk duplicated an existing selection or lost order"); close(picker);
      });
      require(editor->toPlainText()==source && panel->hasChanges(),"Bulk picker modified engine/source before command Apply");
      panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
      const auto changed=editor->toPlainText(); require(changed.contains(expected.join(' ')) && changed.contains("'retain report label'") && changed.contains("% retain report comment") && changed.startsWith(base),qPrintable("Report Apply lost parameters, label, comment or resource source:\n"+changed));
      editor->undo(); require(editor->toPlainText()==source,"Bulk report Undo failed"); editor->redo(); require(editor->toPlainText()==changed,"Bulk report Redo failed");
      for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (dynamic_cast<CommandEditor *>(child->widget())) child->close();
      const auto saved=files.filePath("bulk selection ü.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.runMission()==MainWindow::RunResult::Completed && read(report)==reference,"Bulk GUI report save/reopen differs from independent source");
      // Configured bulk operations retain each caller's restrictions. Manual
      // function arguments may deliberately repeat an entry in two positions.
      for (auto mode:{ReportParameterDialog::Mode::Multiple,ReportParameterDialog::Mode::PlottableMultiple,ReportParameterDialog::Mode::FunctionInputs,ReportParameterDialog::Mode::FunctionOutputs}) {
         ReportParameterDialog picker({"V"},&window,mode); auto *entry=picker.findChild<QComboBox *>("reportParameterEntry");
         picker.findChild<QPushButton *>("reportParameterRemoveAll")->click(); require(picker.selection().isEmpty(),"Remove all retained parameters");
         picker.findChild<QPushButton *>("reportParameterAddAll")->click(); const auto all=picker.selection(); picker.findChild<QPushButton *>("reportParameterAddAll")->click(); require(!all.isEmpty() && picker.selection()==all && all.contains("V"),"Add all configured lost entries or duplicated them");
         if (mode==ReportParameterDialog::Mode::PlottableMultiple) require(!all.contains("Label") && !all.contains("Grid"),"Plot bulk accepted a string or whole array");
         else require(all.contains("Label"),"Report/function bulk omitted configured string");
         if (mode==ReportParameterDialog::Mode::FunctionInputs || mode==ReportParameterDialog::Mode::FunctionOutputs) { picker.findChild<QPushButton *>("reportParameterRemoveAll")->click(); entry->setEditText("V"); picker.findChild<QPushButton *>("reportAddParameter")->click(); picker.findChild<QPushButton *>("reportAddParameter")->click(); require(picker.selection()==QStringList{"V","V"},"Function argument positions were deduplicated"); }
      }
      for (auto mode:{ReportParameterDialog::Mode::Single,ReportParameterDialog::Mode::Writable,ReportParameterDialog::Mode::WritableReal,ReportParameterDialog::Mode::StopParameter,ReportParameterDialog::Mode::PlottableSingle}) {
         ReportParameterDialog picker({"V"},&window,mode); require(picker.findChild<QPushButton *>("reportBulkProperties")->isHidden() && picker.findChild<QPushButton *>("reportParameterAddAll")->isHidden() && picker.findChild<QPushButton *>("reportParameterRemoveAll")->isHidden(),"Single selector exposes invalid bulk actions");
      }
      ReportParameterDialog numeric({},&window,ReportParameterDialog::Mode::PlottableMultiple); numeric.findChild<QComboBox *>("reportPropertyObject")->setCurrentText("Alpha");
      modal(numeric,[&] { numeric.findChild<QPushButton *>("reportBulkProperties")->click(); },[&](QDialog *generic) {
         auto *bulk=dynamic_cast<BulkPropertyDialog *>(generic); selectOwners(bulk,{"Alpha","Beta"}); auto *table=bulk->findChild<QTableWidget *>("bulkPropertyTable"); require(row(table,"UTCGregorian")<0 && row(table,"X")>=0,"Bulk bypassed plottable caller filter");
         bulk->findChild<QPushButton *>("bulkCheckAll")->click(); require(bulk->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled() && bulk->selection().contains("Alpha.EarthMJ2000Eq.X") && table->item(row(table,"FuelMass"),0)->checkState()==Qt::Unchecked,"Check all did not select available references or included conflicting hardware");
         bulk->findChild<QPushButton *>("bulkUncheckAll")->click(); require(!bulk->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Empty bulk property selection accepted");
         check(bulk,"FuelMass"); require(!bulk->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled() && bulk->findChild<QLabel *>("bulkPropertyStatus")->text().contains("no reference shared"),"Different attached tanks were falsely shared");
         selectOwners(bulk,{"Alpha"}); check(bulk,"FuelMass","TankA"); require(bulk->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Single hardware owner recovery failed");
         require(parameterDependency("Alpha","FuelMass").choices==QStringList{"TankA"} && parameterDependency("Empty","FuelMass").choices.isEmpty(),"Dependency helper leaked unrelated hardware");
         const auto fixed=parameterDependency("Alpha","PlanetodeticLAT"); require(fixed.choices.contains("Fixed") && !fixed.choices.contains("EarthMJ2000Eq"),"Body-fixed property restriction lost");
         selectOwners(bulk,{}); require(table->rowCount()==0 && !bulk->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Empty owner selection retained properties"); close(bulk,false);
      });
      std::cout<<"PASS: real Report command bulk owners/properties and typed frames; Cancel, order, repeated bulk deduplication, pending Apply, source/labels/comments, exact Undo/Redo/Unicode save/reopen and independent numeric report; Add all/Remove all and caller restrictions; positional function duplicates; adjustable columns, Check/Uncheck all, empty selection, attached hardware conflict/recovery and body-fixed frames\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
