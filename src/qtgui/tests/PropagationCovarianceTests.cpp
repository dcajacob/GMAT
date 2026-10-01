#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "PropagationGroupsDialog.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include "Moderator.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QStatusBar>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTimer>
#include <QWindow>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Covariance propagation fixture/report unavailable"); return file.readAll(); }
static void run(MainWindow &window) { if (window.runMission()!=MainWindow::RunResult::Completed) { QApplication::processEvents(); throw std::runtime_error(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()); } }
static void sourceCases()
{
   for (const auto &flag:QStringList{"Covariance","'Covariance'"}) {
      const auto source="Propagate 'Retain' BackProp Synchronized P(Sat) Other(Peer, "+flag+") {Sat.ElapsedSecs = -60, StopTolerance = 1e-8, OrbitColor = Green}; % keep source";
      require(PropagationGroupsDialog::supports(source),"Covariance source does not offer assignment controls"); PropagationGroupsDialog dialog(source,{"P","Other"},{"Sat","Peer"});
      auto *cov=dialog.findChild<QCheckBox *>("propagationGroupCovariance"),*stm=dialog.findChild<QCheckBox *>("propagationGroupSTM");
      require(cov->isChecked() && stm->isChecked() && !stm->isEnabled() && dialog.statement()==source,"Implicit STM/source not retained on opening"); cov->setChecked(false); require(!stm->isChecked() && stm->isEnabled() && !dialog.statement().contains("Covariance"),"Covariance removal left implicit STM enabled"); cov->setChecked(true); require(dialog.statement()==source,"Restoring covariance changed original source");
   }
   const QString source="Propagate P(Sat, 'STM', 'AMatrix', 'Covariance') {Sat.ElapsedSecs = 60}; % explicit STM";
   PropagationGroupsDialog dialog(source,{"P"},{"Sat"}); dialog.findChild<QCheckBox *>("propagationGroupCovariance")->setChecked(false);
   require(dialog.statement()=="Propagate P(Sat, 'STM', 'AMatrix') {Sat.ElapsedSecs = 60}; % explicit STM","Covariance removal lost explicitly selected variational flags");
   require(!PropagationGroupsDialog::supports("Propagate P(Sat, 'FutureFlag') {Sat.ElapsedSecs = 60};"),"Unknown propagation flag offered lossy controls");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtPropagationCovariance");
   try {
      TestSettings settings; QTemporaryDir files; const bool preview=argc==4 && QString::fromLocal8Bit(argv[2])=="--preview"; require((argc==2 || preview) && files.isValid(),"Propagation covariance setup failed");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath()); MainWindow window; window.show(); require(window.initialize(startup),"Propagation covariance runtime unavailable");
      if (preview) {
         PropagationGroupsDialog dialog("Propagate 'Covariance preview' Prop(Sat, 'Covariance') {Sat.ElapsedSecs = 600};",{"Prop"},{"Sat"},&window); dialog.show(); QEventLoop wait; QTimer::singleShot(250,&wait,&QEventLoop::quit); wait.exec();
         for (const auto &name:QStringList{"propagationGroupSTM","propagationGroupAMatrix","propagationGroupCovariance"}) { auto *field=dialog.findChild<QCheckBox *>(name); require(field && field->isVisible() && field->mapTo(&dialog,field->rect().bottomRight()).x()<dialog.width(),"Native variational controls overflow dialog"); }
         require(dialog.windowHandle() && dialog.windowHandle()->isExposed() && dialog.grab().save(QString::fromLocal8Bit(argv[3])),"Native covariance propagation preview unavailable"); std::cout<<"PASS: exposed native propagation covariance controls with automatic STM; numerical workflow was not repeated.\n"; return 0;
      }
      sourceCases(); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); const auto covariance=files.filePath("propagated covariance.txt"),state=files.filePath("propagated state.txt"),saved=files.filePath("propagation covariance Δ.script");
      auto source=QString::fromUtf8(read("../samples/Ex_Propagate_Covariance.script")); source.replace("'Ex_R2022a_Propagate_Covariance.txt'","'"+covariance+"'");
      const QString command="Propagate 'Covariance stage' Prop(Sat) {Sat.ElapsedSecs = 600, StopTolerance = 1e-8, OrbitColor = Green}; % retain flags comment";
      source.replace("Propagate Prop(Sat, 'Covariance') {Sat.ElapsedDays = 1}",command);
      source.replace("BeginMissionSequence","Create ReportFile States;\nStates.Filename = '"+state+"';\nStates.WriteHeaders = false;\nStates.Precision = 16;\nBeginMissionSequence"); source+="\nReport States Sat.MoonMJ2000Eq.X Sat.MoonMJ2000Eq.Y Sat.MoonMJ2000Eq.Z Sat.MoonMJ2000Eq.VX Sat.MoonMJ2000Eq.VY Sat.MoonMJ2000Eq.VZ;\n";
      const auto covCommand=QString(command).replace("Prop(Sat)","Prop(Sat, 'Covariance')"),bothCommand=QString(command).replace("Prop(Sat)","Prop(Sat, 'STM', 'AMatrix', 'Covariance')");
      QMap<QString,QByteArray> covResults,stateResults;
      for (const auto &mode:QStringList{"off","covariance","all"}) { auto reference=source; reference.replace(command,mode=="off" ? command : mode=="covariance" ? covCommand : bothCommand); editor->setPlainText(reference); run(window); covResults[mode]=read(covariance); stateResults[mode]=read(state); require(!covResults[mode].isEmpty() && !stateResults[mode].isEmpty(),"Independent propagation reports empty"); }
      require(covResults["off"]!=covResults["covariance"],"Covariance flag did not change independent covariance report");
      // Build-only source-mapping cases: the engine adds command-wide flags
      // to earlier groups and removes their original quote spelling.
      const QString multi="Propagate 'Retain (STM)' Prop(Sat) Other(Peer, 'AMatrix', 'Covariance') {Sat.ElapsedSecs = 600, StopTolerance = 1e-8, OrbitColor = Green}; % group flags";
      auto multiSource=source; multiSource.replace(command,multi); multiSource.replace("BeginMissionSequence","Create Spacecraft Peer;\nCreate Propagator Other;\nOther.FM = FM;\nBeginMissionSequence");
      editor->setPlainText(multiSource); require(window.buildScript(),"Multi-group covariance source failed");
      const auto mapped=window.missionSnapshot(); int mappedIndex=-1; for (int i=0;i<mapped.nodes.size();++i) if (mapped.nodes[i].type=="Propagate") mappedIndex=i;
      require(mappedIndex>=0 && mapped.nodes[mappedIndex].editable && mapped.nodes[mappedIndex].statement==multi,"Engine-added multi-group flags lost original source mapping");
      for (const auto &unsafe:QStringList{QString(multiSource).replace("'Covariance'","'STM'"),QString(multiSource).replace("Other(Peer","Prop(Peer"),QString(multiSource).replace("'Retain (STM)'","'Changed (STM)'"),QString(multiSource).replace("ElapsedSecs = 600","ElapsedSecs = 601")}) {
         const auto mismatched=snapshotMission(Moderator::Instance()->GetFirstCommand(),mapped.canonicalScript,unsafe);
         require(!mismatched.nodes[mappedIndex].editable,"Different flag, group, label or stop was mapped as the same command");
      }
      editor->setPlainText(source); require(window.buildScript(),"Covariance control fixture failed");
      auto open=[&] {
         const auto snapshot=window.missionSnapshot(); int index=-1; for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.contains("'Covariance stage'")) index=i; require(index>=0,"Labeled covariance command missing");
         auto *tree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItemIterator item(tree); while (*item && (!(*item)->data(0,Qt::UserRole).isValid() || (*item)->data(0,Qt::UserRole).toInt()!=index)) ++item; require(*item,"Covariance command tree item missing"); tree->itemDoubleClicked(*item,0); auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow(); require(child && dynamic_cast<CommandEditor *>(child->widget()),"Actual covariance command panel missing"); return child;
      };
      auto groups=[&](CommandEditor &panel,std::function<void(PropagationGroupsDialog &)> inspect) {
         std::exception_ptr failure; QTimer::singleShot(0,&panel,[&] { auto *dialog=dynamic_cast<PropagationGroupsDialog *>(panel.findChild<QDialog *>("propagationGroupsDialog")); try { require(dialog,"Propagation assignment dialog missing"); inspect(*dialog); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } });
         auto *button=panel.findChild<QPushButton *>("editPropagationGroups"); require(button && button->isVisible(),"Covariance command does not offer assignment editor"); button->click(); if (failure) std::rethrow_exception(failure);
      };
      auto *child=open(); auto *panel=dynamic_cast<CommandEditor *>(child->widget()); 
      groups(*panel,[](PropagationGroupsDialog &dialog) { dialog.findChild<QCheckBox *>("propagationGroupCovariance")->setChecked(true); dialog.reject(); }); require(!panel->hasChanges() && editor->toPlainText()==source,"Covariance Cancel changed pending/mission source");
      for (const auto &mode:QStringList{"covariance","all","off"}) {
         if (mode!="covariance") { child=open(); panel=dynamic_cast<CommandEditor *>(child->widget()); }
         const auto before=editor->toPlainText(); const auto oldCommand=panel->findChild<QPlainTextEdit *>("commandSource")->toPlainText();
         groups(*panel,[&](PropagationGroupsDialog &dialog) {
            auto *cov=dialog.findChild<QCheckBox *>("propagationGroupCovariance"),*stm=dialog.findChild<QCheckBox *>("propagationGroupSTM"),*amat=dialog.findChild<QCheckBox *>("propagationGroupAMatrix");
            if (mode=="all") { cov->setChecked(false); stm->setChecked(true); amat->setChecked(true); }
            if (mode=="off") { cov->setChecked(false); stm->setChecked(false); amat->setChecked(false); }
            else cov->setChecked(true);
            require(mode=="off" || (stm->isChecked() && !stm->isEnabled()),"Covariance control does not show automatic STM"); dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
         });
         auto *text=panel->findChild<QPlainTextEdit *>("commandSource"); const auto pending=text->toPlainText(); text->undo(); require(text->toPlainText()==oldCommand,"Pending covariance edit was not one exact Undo"); text->redo(); require(text->toPlainText()==pending && panel->hasChanges() && editor->toPlainText()==before,"Pending covariance Redo/source protection failed");
         panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         if (!child->isVisible() || child->widget()==panel || dynamic_cast<EditablePanel *>(child->widget())->hasChanges()) {
            QString details=QString("Covariance Apply did not refresh retained panel (%1): visible=%2 sameWidget=%3 pending=%4 parent=%5 status=%6\n").arg(mode).arg(child->isVisible()).arg(child->widget()==panel).arg(dynamic_cast<EditablePanel *>(child->widget())->hasChanges()).arg(panel->parentWidget() ? panel->parentWidget()->metaObject()->className() : "none").arg(window.statusBar()->currentMessage())+pending; for (auto *label:child->widget()->findChildren<QLabel *>()) details+='\n'+label->text(); details+='\n'+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText(); throw std::runtime_error(details.toStdString());
         }
         const auto applied=editor->toPlainText(); editor->undo(); require(editor->toPlainText()==before,"Covariance mission Undo was not exact"); editor->redo(); require(editor->toPlainText()==applied,"Covariance mission Redo was not exact");
         require(applied.contains("% retain flags comment") && applied.contains("StopTolerance = 1e-8, OrbitColor = Green"),"Covariance edit lost label/options/comment");
         require(window.saveScriptTo(saved) && window.loadScript(saved),"Covariance propagation Unicode save/reopen failed"); run(window); require(read(covariance)==covResults[mode] && read(state)==stateResults[mode],"GUI covariance flags differ from independent covariance/state reports");
      }
      std::cout<<"PASS: actual Mission/MDI covariance flag with implicit/explicit STM and A-matrix, Cancel, pending/applied exact Undo/Redo, retained panels, labels/options/comments and Unicode save/reopen; independent shortened shipped Moon/SNC covariance and six-state reports for enabled, combined and disabled modes; quoted/bare flags and unchanged multi-group source.\n";
   } catch (BaseException &failure) { std::cerr<<"FAIL: "<<failure.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &failure) { std::cerr<<"FAIL: "<<failure.what()<<'\n'; return 1; }
}
