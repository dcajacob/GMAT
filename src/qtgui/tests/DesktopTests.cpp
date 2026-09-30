#include "MainWindow.hpp"
#include "AboutDialog.hpp"
#include "ResourceEditor.hpp"
#include "CelestialBodyPanel.hpp"
#include "SolarSystemPanel.hpp"
#include "TestSettings.hpp"
#include "FileManager.hpp"
#include "GmatGlobal.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QKeyEvent>
#include <QTemporaryDir>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTreeWidget>
#include <QLineEdit>
#include <QTableWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QMessageBox>
#include <QTimer>
#include <iostream>
#include <stdexcept>
#include <functional>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void write(const QString &path,const QByteArray &bytes) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Desktop fixture write failed"); }
static void escape(QWidget *widget) { QKeyEvent key(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier); QApplication::sendEvent(widget,&key); }
static void later(QWidget *owner,std::exception_ptr &failure,std::function<void()> action)
{
   QTimer::singleShot(0,owner,[owner,&failure,action] { try { action(); } catch (...) { failure=std::current_exception(); for (auto *dialog:owner->findChildren<QDialog *>()) dialog->reject(); } });
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtDesktop");
   if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString();
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; MainWindow window; window.show(); require(window.initialize(startup),"Desktop runtime unavailable");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); const auto original=editor->toPlainText();
      editor->appendPlainText("% retain pending text while inspecting About"); const auto pending=editor->toPlainText();
      const auto root=QString::fromStdString(FileManager::Instance()->GetRootPath()); QFile license(QDir(root).filePath("License.txt"));
      require(license.open(QIODevice::ReadOnly),"Runtime license missing"); const auto expected=QString::fromUtf8(license.readAll());
      auto *action=window.findChild<QAction *>("aboutGMAT"); require(action,"About menu action missing"); std::exception_ptr failure;
      later(&window,failure,[&] {
         auto *about=dynamic_cast<AboutDialog *>(window.findChild<QDialog *>("aboutDialog")); require(about && about->isVisible(),"About menu did not open dialog");
         const auto info=about->findChild<QLabel *>("aboutInfo")->text(); auto *global=GmatGlobal::Instance();
         require(info.contains(QString::fromStdString(global->GetGmatVersion())) && info.contains(QString::fromStdString(global->GetGMATBuildDate())) && info.contains(QString::fromLatin1(qVersion())) && info.contains("64-bit"),"About build/runtime details incorrect");
         auto *credits=about->findChild<QPlainTextEdit *>("aboutCredits"); require(credits && credits->isReadOnly() && credits->toPlainText().contains("SPICE") && credits->toPlainText().contains("OpenSceneGraph"),"About credits unavailable");
         const auto links=about->findChild<QLabel *>("aboutLinks")->text(); require(links.contains("https://sourceforge.net/projects/gmat/") && links.contains("mailto:gmat@gsfc.nasa.gov"),"About project/contact actions missing");
         later(about,failure,[&] {
            auto *dialog=about->findChild<QDialog *>("licenseDialog"); require(dialog && dialog->isVisible(),"Offline license did not open");
            auto *text=dialog->findChild<QPlainTextEdit *>("inspectionText"); require(text && text->isReadOnly() && text->lineWrapMode()==QPlainTextEdit::NoWrap && text->toPlainText()==expected,"License text missing or modified");
            require(!dialog->findChild<QPushButton *>("inspectionSave") && dialog->findChild<QPushButton *>("inspectionFind"),"License inspection actions incorrect");
            if (!capture.isEmpty()) require(dialog->grab().save(capture+".license.png"),"License capture failed");
            escape(dialog);
         });
         about->findChild<QPushButton *>("showLicense")->click(); if (failure) std::rethrow_exception(failure);
         require(about->findChild<QLabel *>("aboutError")->text().isEmpty(),"Valid license produced an error");
         about->resize(460,380); QApplication::processEvents();
         auto *close=about->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Close);
         require(about->rect().contains(close->mapTo(about,close->rect().bottomRight())),"Compact About hides Close");
         if (!capture.isEmpty()) require(about->grab().save(capture+".about.png"),"About capture failed");
         escape(about);
      }); action->trigger(); if (failure) std::rethrow_exception(failure);
      require(editor->toPlainText()==pending && editor->document()->isUndoAvailable(),"About/license changed pending mission source or Undo");
      editor->undo(); require(editor->toPlainText()==original,"About/license damaged pending Undo");
      QTemporaryDir files; require(files.isValid(),"Desktop fixture directory unavailable");
      AboutDialog missing(files.path()); missing.show(); auto *show=missing.findChild<QPushButton *>("showLicense");
      show->click(); auto *error=missing.findChild<QLabel *>("aboutError"); require(error->text().contains(files.filePath("License.txt")) && error->text().contains("Cannot read"),"Missing license is silently accepted");
      write(files.filePath("License.txt"),QByteArray("bad UTF-8 \xff")); show->click(); require(error->text().contains("UTF-8"),"Malformed license is silently truncated");
      write(files.filePath("License.txt"),expected.toUtf8());
      later(&missing,failure,[&] { auto *dialog=missing.findChild<QDialog *>("licenseDialog"); require(dialog && dialog->findChild<QPlainTextEdit *>("inspectionText")->toPlainText()==expected,"Restored license cannot reopen"); escape(dialog); });
      show->click(); if (failure) std::rethrow_exception(failure); require(error->text().isEmpty(),"Restored license retained stale failure"); escape(&missing);
      const auto report=files.filePath("apply.txt");
      const QString prefix="% keep Apply comments α\nCreate Variable V W;\nV = 1; % first initializer\nW = 2;\nCreate Spacecraft Sat;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\n";
      const QString mission="BeginMissionSequence;\nReport 'keep label' Values V W Sat.EarthMJ2000Eq.X;\n";
      editor->setPlainText(prefix+mission); require(window.buildScript(),"Apply fixture build failed");
      auto *workspace=window.findChild<QMdiArea *>("workspace");
      auto open=[&](const QString &name) {
         auto *tree=window.findChild<QTreeWidget *>("Resources"); require(tree,"Resource tree missing");
         const auto items=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Apply resource missing");
         require(QMetaObject::invokeMethod(tree,"itemDoubleClicked",Qt::DirectConnection,Q_ARG(QTreeWidgetItem *,items[0]),Q_ARG(int,0)),"Resource cannot open");
         for (auto *child:workspace->subWindowList()) if (child->property("resourceName").toString()==name) return child;
         throw std::runtime_error("Apply panel missing");
      };
      auto apply=[](QMdiSubWindow *child) { child->widget()->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); };
      auto *v=open("V"),*w=open("W"),*sat=open("Sat");
      auto *oldSat=sat->widget(); auto *section=oldSat->findChild<QTabBar *>("propertySections"); section->setCurrentIndex(section->count()-1);
      oldSat->findChild<QLineEdit *>("propertyFilter")->setText("DryMass"); oldSat->findChild<QTableWidget *>("resourceProperties")->setColumnWidth(0,211);
      w->widget()->findChild<QLineEdit *>("parameterValue")->setText("7");
      for (const auto &value:QStringList{"5","9"}) {
         auto *previous=v->widget(); auto *input=previous->findChild<QLineEdit *>("parameterValue"); input->setText(value); input->setFocus(); apply(v);
         require(v->isVisible() && v->widget()!=previous && !dynamic_cast<EditablePanel *>(v->widget())->hasChanges() && v->widget()->findChild<QLineEdit *>("parameterValue")->text()==value,"Apply did not refresh the open resource");
         require(v->property("sourceScript").toString()==editor->toPlainText() && editor->toPlainText().endsWith(mission) && editor->toPlainText().contains("% first initializer"),"Apply lost source or its updated snapshot");
      }
      require(sat->widget()!=oldSat && sat->isVisible() && sat->widget()->findChild<QLineEdit *>("propertyFilter")->text()=="DryMass" && sat->widget()->findChild<QTableWidget *>("resourceProperties")->columnWidth(0)==211,"Clean dependent panel lost its state on refresh");
      require(dynamic_cast<EditablePanel *>(w->widget())->hasChanges() && w->widget()->findChild<QLineEdit *>("parameterValue")->text()=="7","Apply discarded another panel's pending edit");
      const auto current=editor->toPlainText(); apply(w); require(editor->toPlainText()==current && dynamic_cast<EditablePanel *>(w->widget())->hasChanges(),"Stale pending Apply changed the mission");
      dynamic_cast<EditablePanel *>(w->widget())->discardChanges(); w->close();
      require(window.runMission()==MainWindow::RunResult::Completed,"Repeated Apply mission failed");
      QFile result(report); require(result.open(QIODevice::ReadOnly),"Apply report missing"); const auto expectedReport=result.readAll(); result.close();
      editor->undo(); const auto firstApply=editor->toPlainText(); require(firstApply!=current,"Repeated Apply Undo missing"); editor->undo(); require(editor->toPlainText()==prefix+mission,"First Apply Undo not exact"); editor->redo(); require(editor->toPlainText()==firstApply,"First Apply Redo not exact"); editor->redo(); require(editor->toPlainText()==current,"Repeated Apply Redo not exact");
      require(window.saveScriptTo(files.filePath("Apply ü.script")) && window.loadScript(files.filePath("Apply ü.script")) && window.runMission()==MainWindow::RunResult::Completed,"Apply save/reopen failed");
      require(result.open(QIODevice::ReadOnly) && result.readAll()==expectedReport,"Apply save/reopen changed calculations"); result.close();
      auto reference=prefix; reference.replace("V = 1;","V = 9;"); editor->setPlainText(reference+mission);
      require(window.runMission()==MainWindow::RunResult::Completed && result.open(QIODevice::ReadOnly) && result.readAll()==expectedReport,"Apply report differs from independent script"); result.close();
      std::cout<<"PASS: About menu engine/build/runtime details, credits, exact offline license, read-only/search/Close/Escape, compact layout, missing/malformed license correction and pending mission/Undo preservation\n";
      std::cout<<"PASS: repeated workspace Apply keeps resource windows open, refreshes clean snapshots/page/filter/columns, preserves other pending edits, rejects stale Apply, exact Undo/Redo, Unicode save/reopen and independent numeric reports\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
