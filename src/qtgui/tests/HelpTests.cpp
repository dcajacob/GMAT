#include "MainWindow.hpp"
#include "HelpDialog.hpp"
#include "EditablePanel.hpp"
#include "CommandEditor.hpp"
#include "TestSettings.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QEventLoop>
#include <QTimer>
#include <QWindow>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *text) { if (!ok) throw std::runtime_error(text); }
static void key(QWidget *widget,Qt::Key value) { QKeyEvent event(QEvent::KeyPress,value,Qt::NoModifier); QApplication::sendEvent(widget,&event); }
static void write(const QString &path,const QByteArray &bytes) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Help fixture write failed"); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtHelp");
   if (argc<2 || argc>3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString();
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; MainWindow window; window.show(); require(window.initialize(startup),"Help runtime failed");
      const auto helpPath=QString::fromStdString(FileManager::Instance()->GetFullPathname("HELP_PATH"));
      require(QFileInfo::exists(QDir(helpPath).filePath("html/Spacecraft.html")),"Build offline help with the build-qt-help target before qualifying it");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString original="% preserve help source α\nCreate Variable V;\nV = 1;\nCreate Spacecraft Sat;\nBeginMissionSequence;\nV = V + 1; % keep command\n";
      editor->setPlainText(original); require(window.buildScript(),"Help mission build failed");
      auto *area=window.findChild<QMdiArea *>("workspace");
      auto open=[&](const QString &name) {
         auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto items=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);
         require(items.size()==1,"Help resource missing");
         QMetaObject::invokeMethod(tree,"itemDoubleClicked",Qt::DirectConnection,Q_ARG(QTreeWidgetItem *,items[0]),Q_ARG(int,0));
         for (auto *child:area->subWindowList()) if (child->property("resourceName")==name) return child;
         throw std::runtime_error("Help resource editor did not open");
      };
      auto *v=open("V"),*sat=open("Sat"); auto *input=v->widget()->findChild<QLineEdit *>("parameterValue"); input->setText("9");
      editor->appendPlainText("% retain pending script"); const auto pending=editor->toPlainText();
      auto dialog=[&]() {
         for (auto *candidate:window.findChildren<QDialog *>("helpDialog"))
            if (auto *help=dynamic_cast<HelpDialog *>(candidate);help && help->isVisible()) return help;
         throw std::runtime_error("Context help did not open");
      };
      auto check=[&](const QString &page,const QString &text) {
         auto *help=dialog(); auto *browser=help->findChild<QTextBrowser *>("helpBrowser");
         if (browser->source().fileName()!=page+".html" || !browser->toPlainText().contains(text) || !help->findChild<QLabel *>("helpStatus")->text().isEmpty())
            std::cerr<<"Expected "<<page.toStdString()<<" / "<<text.toStdString()<<", got "<<browser->source().toString().toStdString()<<" status="<<help->findChild<QLabel *>("helpStatus")->text().toStdString()<<'\n';
         require(browser->source().fileName()==page+".html" && browser->toPlainText().contains(text) && help->findChild<QLabel *>("helpStatus")->text().isEmpty(),"Wrong or unreadable Help topic");
         require(browser->isReadOnly(),"Help content is editable"); return help;
      };
      key(input,Qt::Key_F1); auto *help=check("Variable","Variable");
      auto *browser=help->findChild<QTextBrowser *>("helpBrowser"); auto *search=help->findChild<QLineEdit *>("helpFindText"); search->setText("Variable");
      help->findChild<QPushButton *>("helpFindNext")->click(); require(!browser->textCursor().selectedText().isEmpty(),"Help find did not select text");
      search->setText("no such help text α123"); help->findChild<QPushButton *>("helpFindNext")->click(); require(help->findChild<QLabel *>("helpStatus")->text().contains("not found"),"Missing Help search text did not report failure");
      help->findChild<QPushButton *>("helpContents")->click(); check("index","Table of Contents");
      help->findChild<QPushButton *>("helpBack")->click(); require(browser->source().fileName()=="Variable.html","Help Back lost the original topic");
      help->findChild<QPushButton *>("helpForward")->click(); require(browser->source().fileName()=="index.html","Help Forward failed");
      key(help,Qt::Key_Escape); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      sat->widget()->findChild<QPushButton *>("contextHelp")->click(); help=check("Spacecraft","Spacecraft");
      if (!capture.isEmpty()) {
         QEventLoop loop; QTimer::singleShot(400,&loop,&QEventLoop::quit); loop.exec();
         require(help->windowHandle() && help->windowHandle()->isExposed() && help->grab().save(capture+".spacecraft.png"),"Native Help did not expose or capture");
      }
      key(help,Qt::Key_Escape); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      // F1 and a Help button also work inside a modal subordinate editor.
      QDialog subordinate(sat->widget()); auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&subordinate);
      subordinate.setModal(true); subordinate.show(); QApplication::processEvents();
      require(buttons->button(QDialogButtonBox::Help),"Modal editor lacks context Help");
      buttons->button(QDialogButtonBox::Help)->click(); help=check("Spacecraft","Spacecraft");
      require(help->parentWidget()==&subordinate,"Help cannot receive focus through modal parent");
      key(help,Qt::Key_Escape); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); subordinate.reject();
      std::exception_ptr failure;
      QTimer::singleShot(0,sat->widget(),[&] {
         try {
            auto *mass=sat->widget()->findChild<QDialog *>("ballisticsMassDialog"); require(mass,"Ballistics editor missing");
            auto *dryMass=mass->findChild<QLineEdit *>("ballistics_DryMass"); dryMass->setText("1234");
            require(mass->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Help),"Ballistics Help button missing");
            key(dryMass,Qt::Key_F1); auto *massHelp=check("SpacecraftBallisticMass","DryMass");
            if (!capture.isEmpty()) require(massHelp->grab().save(capture+".ballistics.png"),"Ballistics Help capture failed");
            key(massHelp,Qt::Key_Escape); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
            require(dryMass->text()=="1234","Modal Help discarded pending ballistics values"); mass->reject();
         } catch (...) { failure=std::current_exception(); for (auto *modal:sat->widget()->findChildren<QDialog *>()) modal->reject(); }
      });
      sat->widget()->findChild<QPushButton *>("spacecraftBallisticsMass")->click(); if (failure) std::rethrow_exception(failure);
      require(editor->toPlainText()==pending && input->text()=="9" && dynamic_cast<EditablePanel *>(v->widget())->hasChanges(),"Help altered script or unapplied resource values");
      editor->undo(); require(editor->toPlainText()==original,"Help changed mission Undo history");
      window.findChild<QAction *>("helpTutorials")->trigger(); help=check("Tutorials","Tutorials"); key(help,Qt::Key_Escape); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      CommandEditor command("GMAT V = 3;",false,{},[](const QString &) { return QString(); });
      require(command.property("helpTopic")=="Assignment","Assignment Help topic incorrect");
      command.findChild<QPlainTextEdit *>("commandSource")->setPlainText("Propagate Prop(Sat) {Sat.ElapsedSecs = 10};");
      require(command.property("helpTopic")=="Propagate","Edited command Help topic did not refresh");
      QTemporaryDir missing; require(missing.isValid(),"Help recovery fixture missing"); FileManager::Instance()->SetAbsPathname("HELP_PATH",missing.path().toStdString());
      HelpDialog recovery("Spacecraft"); recovery.show(); require(recovery.findChild<QLabel *>("helpStatus")->text().contains("Cannot read"),"Missing Help is silently accepted");
      QDir(missing.path()).mkpath("html"); const auto page=missing.filePath("html/Spacecraft.html");
      write(page,QByteArray("invalid UTF-8 \xff")); recovery.findChild<QPushButton *>("helpRetry")->click(); require(recovery.findChild<QLabel *>("helpStatus")->text().contains("UTF-8"),"Invalid Help UTF-8 accepted");
      write(page,"<html><body>Recovered Spacecraft α</body></html>"); recovery.findChild<QPushButton *>("helpRetry")->click();
      require(recovery.findChild<QLabel *>("helpStatus")->text().isEmpty() && recovery.findChild<QTextBrowser *>("helpBrowser")->toPlainText().contains("Recovered Spacecraft α"),"Help file correction cannot recover");
      QSettings override; override.setValue("Help/Spacecraft",QUrl::fromLocalFile(page).toString()); FileManager::Instance()->SetAbsPathname("HELP_PATH",helpPath.toStdString());
      HelpDialog configured("Spacecraft"); require(configured.findChild<QTextBrowser *>("helpBrowser")->source().toLocalFile()==page,"Help topic override ignored");
      override.remove("Help/Spacecraft");
      HelpDialog tank("ChemicalTank"),prop("PropSetup"); require(tank.findChild<QTextBrowser *>("helpBrowser")->source().fileName()=="FuelTank.html" && prop.findChild<QTextBrowser *>("helpBrowser")->source().fileName()=="Propagator.html","Engine Help aliases unresolved");
      require(editor->toPlainText()==original && input->text()=="9","Help recovery changed mission state");
      std::cout<<"PASS: offline Help contents/tutorials/resource topics, command topic refresh, F1 and button/modal routing, search/navigation/Escape, pending resource/source and Undo preservation, missing/malformed file recovery, overrides and engine topic aliases\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
