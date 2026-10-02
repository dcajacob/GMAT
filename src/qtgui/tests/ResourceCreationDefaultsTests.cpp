#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTreeWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   if (QGuiApplication::platformName()!="offscreen" || argc<2 || argc>3) return 2;
   QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("ResourceCreationDefaults");
   const bool starProbe=argc==3 && QString::fromLocal8Bit(argv[2])=="--star";
   const bool probe=starProbe || (argc==3 && QString::fromLocal8Bit(argv[2])=="--probe");
   try {
      TestSettings settings;
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Runtime unavailable"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString baseline="% Preserve source and implicit defaults α\nCreate Spacecraft Vehicle;\nCreate Variable Keep;\nKeep = 7; % retained comment\nBeginMissionSequence;\nKeep = Keep + 2;\n";
      editor->setPlainText(baseline); require(window.buildScript(),"Baseline build failed");
      QStringList types;
      QTimer::singleShot(0,&window,[&] {
         auto *dialog=window.findChild<QDialog *>("newResourceDialog");
         if (!dialog) return;
         auto *choice=dialog->findChild<QComboBox *>("resourceType");
         for (int i=0;i<choice->count();++i) types.append(choice->itemText(i));
         dialog->reject();
      });
      window.findChild<QAction *>("createResource")->trigger(); require(!types.isEmpty(),"Creator list unavailable");
      if (!probe) {
         require(!types.contains("Star"),"Uncreatable Star offered in creator");
         const auto blocked=window.createResource("Star","UnsupportedStar",baseline);
         require(!blocked.isEmpty() && editor->toPlainText()==baseline && Moderator::Instance()->GetConfiguredObject("Vehicle"),"Unsupported Star mutated mission");
      }
      // The prior 18-type plugin matrix and user-parameter suites are already qualified.
      const QStringList previous{"Formation","GroundStation","Smoother","ProcessNoiseModel","EstimatedParameter","ExtendedKalmanFilter","Simulator","BatchEstimator","TrackingFileSet","ErrorModel","AcceptFilter","RejectFilter","FileInterface","GmatFunction","Yukon","ThrustHistoryFile","ThrustSegment","EclipseLocator","Variable","String","Array"};
      int checked=0; QStringList failed;
      for (const auto &type:types) {
         if (previous.contains(type)) continue;
         if (starProbe && type!="Star") continue;
         if (!probe && type!="EphemerisFile") continue;
         const auto name="New"+type;
         const auto error=window.createResource(type,name,baseline);
         ++checked;
         if (!error.isEmpty()) {
            failed.append(type); std::cout << "FAIL " << type.toStdString() << ": " << error.toStdString() << '\n';
            std::cout << window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString() << '\n';
            require(editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject(name.toStdString()),"Failed creation left source/model mutation");
         } else {
            require(Moderator::Instance()->GetConfiguredObject(name.toStdString()) && editor->toPlainText().endsWith(baseline),"Creation lost resource/source");
            std::cout << "PASS create " << type.toStdString() << '\n';
            editor->undo(); require(editor->toPlainText()==baseline && window.buildScript(),"Creation Undo/rebuild failed");
         }
      }
      std::cout << "Checked " << checked << " previously untested creator types; failures: " << failed.join(", ").toStdString() << '\n';
      if (!probe) require(failed.isEmpty(),"Resource default creation failures remain");
      if (!probe) {
         QTemporaryDir files; require(files.isValid(),"Temporary directory unavailable");
         auto *tree=window.findChild<QTreeWidget *>("Resources"); auto *area=window.findChild<QMdiArea *>("workspace");
         const auto noSpacecraft=QString("% No spacecraft yet α\nCreate Variable Keep;\nKeep = 7;\nBeginMissionSequence;\nKeep = Keep + 2;\n");
         editor->setPlainText(noSpacecraft); require(window.buildScript(),"Empty-spacecraft baseline failed");
         const auto error=window.createResource("EphemerisFile","Ephemeris",noSpacecraft);
         require(error.contains("Create a Spacecraft") && editor->toPlainText()==noSpacecraft && !Moderator::Instance()->GetConfiguredObject("Ephemeris"),"Missing spacecraft not rejected before transaction");
         std::exception_ptr failure;
         QTimer::singleShot(0,&window,[&] {
            auto *dialog=window.findChild<QDialog *>("newResourceDialog");
            try {
               require(dialog,"Empty-spacecraft dialog missing"); dialog->findChild<QComboBox *>("resourceType")->setCurrentText("EphemerisFile");
               auto *choice=dialog->findChild<QComboBox *>("ephemeris_Spacecraft"); require(choice->isVisible() && choice->count()==0,"Missing spacecraft selector state");
               dialog->findChild<QLineEdit *>("resourceName")->setText("Ephemeris");
               dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
               require(dialog->result()!=QDialog::Accepted && dialog->findChild<QLabel *>("resourceCreationStatus")->text().contains("Create a Spacecraft") && editor->toPlainText()==noSpacecraft,"Missing spacecraft error not correctable");
            } catch (...) { failure=std::current_exception(); }
            if (dialog) dialog->reject();
         });
         window.findChild<QAction *>("createResource")->trigger(); if (failure) std::rethrow_exception(failure);
         const auto multiple=QString("Create Spacecraft Another;\n")+baseline;
         editor->setPlainText(multiple); require(window.buildScript(),"Multiple-spacecraft baseline failed");
         require(!window.createResource("EphemerisFile","Invalid",multiple,1,1,std::nullopt,{},"Keep").isEmpty() && editor->toPlainText()==multiple,"Wrong-type spacecraft accepted");
         bool accepted=false;
         auto creator=[&](bool commit) {
            QTimer::singleShot(0,&window,[&] {
               auto *dialog=window.findChild<QDialog *>("newResourceDialog");
               try {
                  require(dialog,"Ephemeris creator missing"); auto *type=dialog->findChild<QComboBox *>("resourceType"); type->setCurrentText("EphemerisFile");
                  auto *choice=dialog->findChild<QComboBox *>("ephemeris_Spacecraft"); require(choice->isVisible() && choice->count()==2 && choice->findText("Vehicle")>=0 && choice->findText("Another")>=0,"Spacecraft choices incorrect");
                  choice->setCurrentText("Vehicle"); type->setCurrentText("ReportFile"); require(!choice->isVisible(),"Spacecraft chooser leaked into other types");
                  type->setCurrentText("EphemerisFile"); require(choice->currentText()=="Vehicle","Type switch lost pending selection");
                  choice->setCurrentText("Another"); dialog->findChild<QLineEdit *>("resourceName")->setText("Ephemeris");
                  if (commit) { dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); accepted=dialog->result()==QDialog::Accepted; }
               } catch (...) { failure=std::current_exception(); }
               if (dialog && (!commit || !accepted)) dialog->reject();
            });
            window.findChild<QAction *>("createResource")->trigger(); if (failure) std::rethrow_exception(failure);
         };
         creator(false); require(editor->toPlainText()==multiple && !Moderator::Instance()->GetConfiguredObject("Ephemeris"),"Cancel mutated mission");
         creator(true);
         const auto created=editor->toPlainText();
         auto *object=Moderator::Instance()->GetConfiguredObject("Ephemeris");
         require(accepted && object,"Configured Ephemeris creator did not accept");
         require(object->GetStringParameter("Spacecraft")=="Another","Selected spacecraft not committed");
         require(created.contains("Create EphemerisFile Ephemeris;"),"Ephemeris declaration absent");
         require(created.endsWith(multiple),"Unrelated creation source changed");
         require(tree->findItems("Ephemeris",Qt::MatchExactly|Qt::MatchRecursive).size()==1,"Ephemeris tree entry missing");
         require(area->activeSubWindow() && area->activeSubWindow()->property("resourceName").toString()=="Ephemeris","Ephemeris editor did not open");
         for (auto *child:area->subWindowList()) if (child->property("resourceName").toString()=="Ephemeris") child->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
         editor->undo(); require(editor->toPlainText()==multiple && window.buildScript() && !Moderator::Instance()->GetConfiguredObject("Ephemeris"),"Ephemeris Undo failed");
         editor->redo(); require(editor->toPlainText()==created && window.buildScript() && Moderator::Instance()->GetConfiguredObject("Ephemeris")->GetStringParameter("Spacecraft")=="Another","Ephemeris Redo failed");
         const auto saved=files.filePath("ephemeris α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==created && Moderator::Instance()->GetConfiguredObject("Ephemeris")->GetStringParameter("Spacecraft")=="Another","Ephemeris Unicode Save/reopen failed");
         std::cout << "PASS: required spacecraft selection/default, missing/wrong-type correction before transaction, pending selection and Cancel, selected spacecraft/source/tree/editor, Undo/Redo/Unicode Save/reopen; uncreatable Star not offered. No mission or old plugin matrix.\n";
      }
      return 0;
   } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
