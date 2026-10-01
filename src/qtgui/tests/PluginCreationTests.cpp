#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QEventLoop>
#include <QWindow>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QTreeWidget>
#include <QKeyEvent>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Plugin source evidence unavailable"); return file.readAll(); }
static void categoryMenuChecks(MainWindow &window,QPlainTextEdit *editor,QTreeWidget *tree,const QString &baseline,const QString &saved,const QString &capture)
{
   auto category=[&](const QString &name) { const auto found=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive); for (auto *item:found) if (item->data(0,Qt::UserRole+1).isValid()) return item; throw std::runtime_error("Resource category absent"); };
   auto inspect=[&](QTreeWidgetItem *item,const QStringList &expected,bool generic=false) {
      tree->scrollToItem(item); QApplication::processEvents(); std::exception_ptr failure;
      QTimer::singleShot(0,&window,[&] { auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget()); try {
         require(menu,"Category context menu missing"); QStringList types; bool hasGeneric=false; QAction *remove=nullptr;
         for (auto *action:menu->actions()) { if (action->objectName().startsWith("addResource_")) { types.append(action->data().toString()); require(action->text()=="Add "+action->data().toString()+"…","Category Add label generic/wrong"); } if (action->objectName()=="createResource") hasGeneric=true; if (action->text()=="Delete resource…") remove=action; }
         require(hasGeneric==generic && (!generic || types.isEmpty()),"Generic creator leaked into category or missing at root");
         for (const auto &type:expected) require(types.contains(type),qPrintable("Missing category Add "+type));
         if (item->text(0)!="Spacecraft") require(!types.contains("Spacecraft"),"Unrelated Spacecraft creation offered");
         require(remove && remove->isEnabled()==!item->data(0,Qt::UserRole).toString().isEmpty(),"Category/child delete availability changed");
         if (!capture.isEmpty() && item->text(0)=="Ground Stations") require(menu->grab().save(capture+".menu.png"),"GroundStation category menu capture failed");
      } catch (...) { failure=std::current_exception(); } if (menu) menu->close(); });
      tree->customContextMenuRequested(tree->visualItemRect(item).center()); if (failure) std::rethrow_exception(failure); require(editor->toPlainText()==baseline,"Inspect/Cancel category menu changed source");
   };
   inspect(category("Ground Stations"),{"GroundStation"}); inspect(category("Spacecraft"),{"Spacecraft"}); inspect(category("Burns"),{"ImpulsiveBurn","FiniteBurn"}); inspect(category("Variables, Arrays, Strings"),{"Array","Variable","String"}); inspect(category("Solvers"),{"DifferentialCorrector","Yukon","Smoother"}); inspect(category("Functions"),{"GmatFunction"}); inspect(category("Process Noise Models"),{"ProcessNoiseModel"}); inspect(category("Estimated Parameters"),{"EstimatedParameter"});
   auto *root=tree->topLevelItem(0); inspect(root,{},true); const auto keep=tree->findItems("Keep",Qt::MatchExactly|Qt::MatchRecursive); require(keep.size()==1,"Parameter child missing"); inspect(keep.first(),{"Array","Variable","String"});
   auto create=[&](const QString &categoryName,const QString &selectedType,bool accept) {
      auto *item=category(categoryName); tree->scrollToItem(item); QApplication::processEvents(); std::exception_ptr failure; bool opened=false;
      QTimer::singleShot(0,&window,[&] { auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget()); try {
         require(menu,"Selected category menu missing"); QAction *action=nullptr; for (auto *candidate:menu->actions()) if (candidate->objectName()=="addResource_"+selectedType) action=candidate; require(action && action->isEnabled(),"Selected category Add unavailable");
         QTimer::singleShot(0,&window,[&] { auto *dialog=window.findChild<QDialog *>("newResourceDialog"); try {
            require(dialog,"Category creator not opened"); opened=true; auto *type=dialog->findChild<QComboBox *>("resourceType"); require(type && type->count()==1 && type->currentText()==selectedType && !type->isEnabled() && dialog->windowTitle()=="New "+selectedType,"Category creator wrong/unconstrained type/title");
            auto *name=dialog->findChild<QLineEdit *>("resourceName"); name->setText("AddedGround");
            if (!capture.isEmpty() && selectedType=="GroundStation" && !accept) require(dialog->grab().save(capture+".creator.png"),"Context GroundStation creator capture failed");
            auto *buttons=dialog->findChild<QDialogButtonBox *>(); if (accept) {
               name->setText("not valid"); buttons->button(QDialogButtonBox::Ok)->click(); require(dialog->isVisible() && editor->toPlainText()==baseline && dialog->findChild<QLabel *>("resourceCreationStatus")->text().contains("name"),"Context invalid name did not preserve correctable creator");
               name->setText("Keep"); buttons->button(QDialogButtonBox::Ok)->click(); require(dialog->isVisible() && editor->toPlainText()==baseline && dialog->findChild<QLabel *>("resourceCreationStatus")->text().contains("already in use"),"Context duplicate name changed source");
               name->setText("AddedGround"); buttons->button(QDialogButtonBox::Ok)->click(); require(dialog->result()==QDialog::Accepted,"Context station Create failed");
            } else buttons->button(QDialogButtonBox::Cancel)->click();
         } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } });
         menu->setActiveAction(action); QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(menu,&enter);
      } catch (...) { failure=std::current_exception(); if (menu) menu->close(); } });
      tree->customContextMenuRequested(tree->visualItemRect(item).center()); if (failure) std::rethrow_exception(failure); require(opened,"Selected context Add did not launch creator");
   };
   create("Ground Stations","GroundStation",false); require(editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject("AddedGround"),"Context Cancel created station");
   create("Variables, Arrays, Strings","Array",false); require(editor->toPlainText()==baseline,"Category Array Cancel changed source");
   create("Ground Stations","GroundStation",true); const auto created=editor->toPlainText(); require(created=="Create GroundStation AddedGround;\n"+baseline && Moderator::Instance()->GetConfiguredObject("AddedGround") && tree->findItems("AddedGround",Qt::MatchExactly|Qt::MatchRecursive).size()==1,"Context station Create changed unrelated source or tree/model missing");
   auto *area=window.findChild<QMdiArea *>("workspace"); require(area->activeSubWindow() && area->activeSubWindow()->property("resourceName").toString()=="AddedGround","Context station did not open its editor");
   editor->undo(); require(editor->toPlainText()==baseline && window.buildScript() && !Moderator::Instance()->GetConfiguredObject("AddedGround"),"Context station Undo failed"); editor->redo(); require(editor->toPlainText()==created && window.buildScript() && Moderator::Instance()->GetConfiguredObject("AddedGround"),"Context station Redo failed");
   require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==created && Moderator::Instance()->GetConfiguredObject("AddedGround"),"Context station Unicode Save/reopen changed source/model");
   // Global Edit action retains the complete creator and the normal Spacecraft default.
   std::exception_ptr failure; QTimer::singleShot(0,&window,[&] { auto *dialog=window.findChild<QDialog *>("newResourceDialog"); try { require(dialog,"Global creator missing"); auto *type=dialog->findChild<QComboBox *>("resourceType"); require(type->count()>10 && type->isEnabled() && type->currentText()=="Spacecraft" && dialog->windowTitle()=="New resource","Global creator narrowed by previous context"); } catch (...) { failure=std::current_exception(); } if (dialog) dialog->reject(); }); window.findChild<QAction *>("createResource")->trigger(); if (failure) std::rethrow_exception(failure); require(editor->toPlainText()==created,"Global creator Cancel changed source");
   std::cout<<"PASS category-specific Add actions for eight categories, child inheritance/root generic, delete availability, GroundStation/Array constrained creators and Cancel, invalid/duplicate correction, actual GroundStation creation/opened editor, exact unrelated source/Undo/Redo/Unicode save/reopen and unchanged global creator; no mission or old plugin matrix.\n";
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtPluginCreation");
   try {
      // A native --context run coincided with a confirmed GNOME compositor crash
      // after Qt popup-grab warnings. Keep this probe off that surface.
      if ((argc==3 || argc==4) && QString::fromLocal8Bit(argv[2])=="--context" && QGuiApplication::platformName()=="wayland")
         throw std::runtime_error("Category menu automation is disabled on Wayland; use the offscreen test. Native desktop acceptance remains unqualified.");
      TestSettings settings; QTemporaryDir files; const bool preview=argc==4 && QString::fromLocal8Bit(argv[2])=="--preview"; const bool context=(argc==3 || argc==4) && QString::fromLocal8Bit(argv[2])=="--context"; const bool import=argc==3 && QString::fromLocal8Bit(argv[2])=="--import"; require((argc==2 || preview || import || context) && files.isValid(),"Plugin creation setup failed"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath()); MainWindow window; window.show(); require(window.initialize(startup),"Plugin creation runtime unavailable");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree=window.findChild<QTreeWidget *>("Resources"); auto *area=window.findChild<QMdiArea *>("workspace"); require(editor && tree && area,"Plugin creation workspace missing");
      const QString baseline="% retain unrelated implicit defaults and mission α\nCreate Variable Keep;\nKeep = 7; % retain comment\nBeginMissionSequence;\nKeep = Keep + 2;\n"; editor->setPlainText(baseline); require(window.buildScript(),"Plugin baseline build failed");
      if (context) { categoryMenuChecks(window,editor,tree,baseline,files.filePath("category Δ.script"),argc==4 ? QFileInfo(argv[3]).absoluteFilePath() : QString()); return 0; }
      if (preview) {
         std::exception_ptr failure; QTimer::singleShot(0,&window,[&] { auto *dialog=window.findChild<QDialog *>("newResourceDialog"); try { require(dialog,"Native plugin creator unavailable"); auto *choice=dialog->findChild<QComboBox *>("resourceType"); for (const auto &type:QStringList{"Formation","GroundStation","Smoother","ProcessNoiseModel","EstimatedParameter"}) require(choice->findText(type)>=0,"Native missing plugin category"); choice->setCurrentText("GmatFunction"); dialog->findChild<QLineEdit *>("resourceName")->setText("NewFunction"); QEventLoop wait; QTimer::singleShot(200,&wait,&QEventLoop::quit); wait.exec(); auto *path=dialog->findChild<QLineEdit *>("resourceFunctionPath"); require(path && path->isVisible() && dialog->windowHandle() && dialog->windowHandle()->isExposed() && dialog->grab().save(QString::fromLocal8Bit(argv[3])+".creator.png"),"Native function creation capture failed"); } catch (...) { failure=std::current_exception(); } if (dialog) dialog->reject(); }); window.findChild<QAction *>("createResource")->trigger(); if (failure) std::rethrow_exception(failure); require(editor->toPlainText()==baseline,"Native creator Cancel changed source"); std::cout<<"PASS: native exposed New resource with five recovered plugin types and function file controls; Cancel preserves source; no mission execution or portal qualification.\n"; return 0;
      }
      if (import) {
         const auto path=files.filePath("Existing.gmf"),saved=files.filePath("imported Δ.script"),report=files.filePath("imported report.txt"); const QByteArray body="function [output] = Existing(input)\nCreate Variable output;\nBeginMissionSequence;\noutput = 3 * input - 2;\n"; QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(body)==body.size(),"Existing function fixture failed"); file.close();
         std::exception_ptr failure; bool accepted=false; QTimer::singleShot(0,&window,[&] { auto *dialog=window.findChild<QDialog *>("newResourceDialog"); try { require(dialog,"Function import creator missing"); dialog->findChild<QComboBox *>("resourceType")->setCurrentText("GmatFunction"); dialog->findChild<QLineEdit *>("resourceName")->setText("Imported"); auto *field=dialog->findChild<QLineEdit *>("resourceFunctionPath"); auto *browse=dialog->findChild<QPushButton *>("resourceFunctionBrowse");
            auto picker=[&](bool accept) { QTimer::singleShot(0,dialog,[&,accept] { auto *chooser=dynamic_cast<QFileDialog *>(QApplication::activeModalWidget()); if (!chooser) return; chooser->selectFile(path); if (accept) QMetaObject::invokeMethod(chooser,"accept",Qt::DirectConnection); else chooser->reject(); }); browse->click(); };
            picker(false); require(field->text().isEmpty() && editor->toPlainText()==baseline,"Import Browse Cancel changed pending/source"); field->setText(files.filePath("missing.gmf")); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); require(dialog->result()!=QDialog::Accepted && editor->toPlainText()==baseline && dialog->findChild<QLabel *>("resourceCreationStatus")->text().contains("readable"),"Missing import file did not remain correctable"); picker(true); require(field->text()==path,"Import Browse lost selected path"); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); accepted=dialog->result()==QDialog::Accepted; if (!accepted) dialog->reject();
         } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } }); window.findChild<QAction *>("createResource")->trigger(); if (failure) std::rethrow_exception(failure); require(accepted && Moderator::Instance()->GetConfiguredObject("Imported") && read(path)==body,"Function import failed or edited original file");
         const auto imported=editor->toPlainText(); require(imported=="Create GmatFunction Imported;\nGMAT Imported.FunctionPath = '"+path+"';\n"+baseline && window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"Function import source/save/reopen changed"); for (auto *child:area->subWindowList()) if (child->property("resourceName").toString()=="Imported") child->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
         require(window.applyMissionChange(window.missionSnapshot(),-1,MissionEdit::Append,"Keep = Imported(Keep);").isEmpty(),"Imported aliased function call could not be inserted"); require(window.createResource("ReportFile","Values",editor->toPlainText()).isEmpty() && window.applyResourceChanges("Values",{{"Filename",report},{"WriteHeaders","false"}},editor->toPlainText()).isEmpty() && window.applyMissionChange(window.missionSnapshot(),-1,MissionEdit::Append,"Report Values Keep;").isEmpty(),"Imported function report setup failed"); require(window.runMission()==MainWindow::RunResult::Completed && QString::fromUtf8(read(report)).trimmed().toDouble()==25 && read(path)==body,"Imported function alias result or source preservation failed"); std::cout<<"PASS: actual function import Browse Cancel/accept, missing-file correction, different resource/file names, exact file/source and Unicode save/reopen, independently expected aliased function result 25.\n"; return 0;
      }
      const QStringList types{"Formation","GroundStation","Smoother","ProcessNoiseModel","EstimatedParameter","ExtendedKalmanFilter","Simulator","BatchEstimator","TrackingFileSet","ErrorModel","AcceptFilter","RejectFilter","FileInterface","GmatFunction","Yukon","ThrustHistoryFile","ThrustSegment","EclipseLocator"};
      const auto functionFile=files.filePath("Added13.gmf");
      auto create=[&](const QString &type,const QString &name,bool accept) {
         std::exception_ptr failure; bool accepted=false;
         QTimer::singleShot(0,&window,[&] { auto *dialog=window.findChild<QDialog *>("newResourceDialog"); try { require(dialog,"Plugin New resource dialog missing"); auto *choice=dialog->findChild<QComboBox *>("resourceType"); require(choice && choice->findText(type)>=0,"Selected plugin resource missing from New resource"); choice->setCurrentText(type); dialog->findChild<QLineEdit *>("resourceName")->setText(name); if (accept) {
                  if (type=="GmatFunction") {
                     auto *path=dialog->findChild<QLineEdit *>("resourceFunctionPath"); auto *button=dialog->findChild<QPushButton *>("resourceFunctionNew"); require(path && path->isVisible() && button,"Function creation controls missing");
                     const auto sourceBeforeMissing=editor->toPlainText(); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); require(dialog->result()!=QDialog::Accepted && editor->toPlainText()==sourceBeforeMissing && dialog->findChild<QLabel *>("resourceCreationStatus")->text().contains("Choose a function file"),"Missing function path did not remain correctable");
                     QTimer::singleShot(0,dialog,[&] { auto *picker=dialog->findChild<QFileDialog *>("newFunctionFileDialog"); if (picker) picker->reject(); }); button->click(); require(!QFileInfo::exists(functionFile) && path->text().isEmpty(),"New function chooser Cancel wrote a file");
                     QTimer::singleShot(0,dialog,[&] { auto *picker=dialog->findChild<QFileDialog *>("newFunctionFileDialog"); if (!picker) return; picker->selectFile(functionFile); QTimer::singleShot(0,dialog,[&] { auto *fileEditor=dialog->findChild<QDialog *>("functionFileDialog"); if (fileEditor) fileEditor->reject(); }); QMetaObject::invokeMethod(picker,"accept",Qt::DirectConnection); }); button->click(); require(!QFileInfo::exists(functionFile) && path->text().isEmpty(),"New function editor Cancel wrote a file");
                     QTimer::singleShot(0,dialog,[&] { auto *picker=dialog->findChild<QFileDialog *>("newFunctionFileDialog"); if (!picker) return; picker->selectFile(functionFile); QTimer::singleShot(0,dialog,[&] { auto *fileEditor=dialog->findChild<QDialog *>("functionFileDialog"); if (!fileEditor) return; auto *text=fileEditor->findChild<QPlainTextEdit *>("functionFileText"); text->setPlainText("function [output] = "+name+"(input)\nCreate Variable output;\nBeginMissionSequence;\noutput = 2 * input + 1;\n"); fileEditor->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click(); }); QMetaObject::invokeMethod(picker,"accept",Qt::DirectConnection); }); button->click(); require(QFileInfo::exists(functionFile) && path->text()==functionFile,"New function file save did not populate creator");
                  }
                  dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); accepted=dialog->result()==QDialog::Accepted; if (!accepted) { std::cerr<<"Creation diagnostic "<<type.toStdString()<<": "; for (auto *label:dialog->findChildren<QLabel *>()) std::cerr<<label->text().toStdString()<<" "; std::cerr<<'\n'<<window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()<<'\n'; dialog->reject(); } } else dialog->reject(); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } });
         window.findChild<QAction *>("createResource")->trigger(); if (failure) std::rethrow_exception(failure); require(accepted==accept,"Plugin resource creation did not commit");
      };
      QStringList names; for (int i=0;i<types.size();++i) {
         const auto name=QString("Added%1").arg(i); const auto before=editor->toPlainText(); create(types[i],name,false); require(editor->toPlainText()==before && !Moderator::Instance()->GetConfiguredObject(name.toStdString()),"Plugin creation Cancel changed source/model");
         create(types[i],name,true); auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString()); const auto after=editor->toPlainText(); require(object && object->IsOfType(types[i].toStdString()) && after=="Create "+types[i]+" "+name+";\n"+(types[i]=="GmatFunction" ? "GMAT "+name+".FunctionPath = '"+functionFile+"';\n" : QString())+before && tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive).size()==1,"Plugin creation changed unrelated source or failed tree/model synchronization");
         bool panel=false; for (auto *child:area->subWindowList()) if (child->property("resourceName").toString()==name) { panel=true; child->close(); } require(panel,"Created plugin resource did not open its editor"); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
         editor->undo(); require(editor->toPlainText()==before && window.buildScript() && !Moderator::Instance()->GetConfiguredObject(name.toStdString()),"Plugin creation Undo did not restore source/model"); editor->redo(); require(editor->toPlainText()==after && window.buildScript() && Moderator::Instance()->GetConfiguredObject(name.toStdString()),"Plugin creation Redo did not restore source/model"); names.append(name); std::cout<<"Created/canceled/Undo/Redo: "<<types[i].toStdString()<<'\n';
      }
      const auto combined=editor->toPlainText(); const auto saved=files.filePath("plugin resources Δ.script"); require(window.saveScriptTo(saved) && read(saved)==combined.toUtf8() && window.loadScript(saved) && window.buildScript(),"Plugin resource Unicode save/reopen failed");
      for (int i=0;i<names.size();++i) {
         const auto name=names[i]; require(Moderator::Instance()->GetConfiguredObject(name.toStdString()),"Plugin source reopen lost object"); const auto before=editor->toPlainText(); auto items=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Plugin removal tree item missing"); tree->expandAll(); tree->scrollToItem(items.first()); bool confirmed=false; std::exception_ptr failure;
         QTimer::singleShot(0,&window,[&] { auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget()); try { require(menu,"Plugin removal context menu missing"); for (auto *action:menu->actions()) if (action->text()=="Delete resource…") { menu->setActiveAction(action); QTimer::singleShot(0,&window,[&] { auto *confirmation=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); if (confirmation) { confirmed=true; confirmation->button(QMessageBox::Yes)->click(); } }); QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(menu,&enter); return; } require(false,"Plugin deletion action missing"); } catch (...) { failure=std::current_exception(); if (menu) menu->close(); } });
         tree->customContextMenuRequested(tree->visualItemRect(items.first()).center()); if (failure) std::rethrow_exception(failure); auto expected=before; expected.remove("Create "+types[i]+" "+name+";\n"); if (types[i]=="GmatFunction") expected.remove("GMAT "+name+".FunctionPath = '"+functionFile+"';\n"); require(confirmed && editor->toPlainText()==expected && !Moderator::Instance()->GetConfiguredObject(name.toStdString()) && tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive).isEmpty(),"Plugin deletion changed unrelated source or retained object/tree"); editor->undo(); require(editor->toPlainText()==before && window.buildScript() && Moderator::Instance()->GetConfiguredObject(name.toStdString()),"Plugin deletion Undo failed"); editor->redo(); require(editor->toPlainText()==expected && window.buildScript() && !Moderator::Instance()->GetConfiguredObject(name.toStdString()),"Plugin deletion Redo failed"); std::cout<<"Removed/Undo/Redo: "<<types[i].toStdString()<<'\n';
      }
      require(editor->toPlainText()==baseline && window.saveScriptTo(saved) && read(saved)==baseline.toUtf8() && window.loadScript(saved) && window.buildScript(),"Plugin deletion round trip did not restore baseline");
      const auto report=files.filePath("new function report.txt"); editor->setPlainText("Create GmatFunction Added13;\nAdded13.FunctionPath = '"+functionFile+"';\nCreate Variable Keep;\nKeep = 9;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nBeginMissionSequence;\nKeep = Added13(Keep);\nReport Values Keep;\n"); require(window.runMission()==MainWindow::RunResult::Completed && QString::fromUtf8(read(report)).trimmed().toDouble()==19,"Newly created function did not execute its independently expected result");
      std::cout<<"PASS: 18 selected plugin resource types through actual New resource and Delete menus, Cancel, opened editor/tree/model, exact unrelated source, per-type Undo/Redo, combined Unicode save/reopen and restored baseline. Existing plugin execution/report evidence reused; newly created function executes to independently expected 19.\n";
   } catch (BaseException &e) { std::cerr<<"FAIL: "<<e.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
