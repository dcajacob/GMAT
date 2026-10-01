#include "MainWindow.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
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

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Search evidence unavailable"); return file.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Search fixture/evidence write failed"); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtReportSearchOptions");
   try {
      TestSettings settings; QTemporaryDir files; require((argc==2 || argc==3) && files.isValid(),"Search-option setup failed"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Search-option runtime initialization failed"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); const auto report=files.filePath("calculation Δ.txt"),saved=files.filePath("search Δ.script");
      const QString source="% preserve report search α\nCreate Variable Value;\nValue = 2;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Precision = 17;\nBeginMissionSequence;\nReport Values Value;\nValue = Value + 3;\nReport Values Value;\n";
      editor->setPlainText(source); require(window.saveScriptTo(saved) && window.loadScript(saved),"Report source Unicode reopen failed"); require(window.runMission()==MainWindow::RunResult::Completed,"Report calculation failed"); const auto expected=read(report); require(QString::fromUtf8(expected).simplified()=="2 5","Independent known report values incorrect");
      auto *output=window.findChild<QTreeWidget *>("Output"); const auto items=output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Report Output entry missing"); output->itemDoubleClicked(items.front(),0); auto *area=window.findChild<QMdiArea *>("workspace"); auto *actual=dynamic_cast<ReportViewer *>(area->activeSubWindow()->widget()); require(actual,"Actual report viewer missing");
      auto *actualCase=actual->findChild<QCheckBox *>("reportFileMatchCase"),*actualWords=actual->findChild<QCheckBox *>("reportFileWholeWords"); require(actualCase && actualWords,"Complete-file case/whole-word options missing"); require(actualCase->isChecked() && !actualWords->isChecked(),"Existing case-sensitive literal default changed");
      auto finish=[&](ReportViewer &viewer) { auto *stop=viewer.findChild<QPushButton *>("reportStopSearch"); QElapsedTimer elapsed; elapsed.start(); while (stop->isEnabled() && elapsed.elapsed()<10000) app.processEvents(QEventLoop::AllEvents,10); require(!stop->isEnabled(),"Bounded file search did not finish"); };
      actualWords->setChecked(true); actual->findChild<QLineEdit *>("reportFileSearchText")->setText("5"); actual->findChild<QPushButton *>("reportSearchFile")->click(); finish(*actual); require(actual->findChild<QPlainTextEdit *>("report:Values")->textCursor().selectedText()=="5","Actual Output full-file options failed to select known value");
      constexpr qsizetype chunk=1024*1024; QByteArray bytes; auto pad=[&](qsizetype size) { while (bytes.size()<size) bytes.append("state row 123\n"); bytes.truncate(size); };
      pad(chunk-6); bytes.append(" "); const auto unicodeOffset=bytes.size(); bytes.append(QString::fromUtf8("ÉPHÉMÉRIDE").toUtf8()); bytes.append("\r\n");
      pad(2*chunk-6); bytes.append(" Orbiting "); const auto wordOffset=bytes.size()+QString::fromUtf8("_orbit orbit_ λorbit orbitλ 𝔘orbit orbit𝔘 orbit́ ́orbit ").toUtf8().size(); bytes.append(QString::fromUtf8("_orbit orbit_ λorbit orbitλ 𝔘orbit orbit𝔘 orbit́ ́orbit ORBIT orbit\r\n").toUtf8());
      pad(3*chunk-3); bytes.append(" "); const auto kelvinOffset=bytes.size(); bytes.append(QString::fromUtf8("K 𐐀\r\n").toUtf8());
      pad(4*chunk-37); bytes.append("xedge "); pad(4*chunk-25); bytes.append("xedge ");
      pad(18*chunk+12); bytes.append(" "); const auto finalOffset=bytes.size(); bytes.append(QString::fromUtf8("éphéméride\r\nVX\r\nVY\r\n").toUtf8()); const auto large=files.filePath("large Δ.txt"); write(large,bytes);
      auto *viewer=new ReportViewer(large,"Search options",area); auto *child=area->addSubWindow(viewer); child->resize(850,530); child->show(); area->setActiveSubWindow(child);
      auto *matchCase=viewer->findChild<QCheckBox *>("reportFileMatchCase"),*words=viewer->findChild<QCheckBox *>("reportFileWholeWords"); auto *query=viewer->findChild<QLineEdit *>("reportFileSearchText"); auto *search=viewer->findChild<QPushButton *>("reportSearchFile"),*next=viewer->findChild<QPushButton *>("reportNextMatch"),*stop=viewer->findChild<QPushButton *>("reportStopSearch"); auto *text=viewer->findChild<QPlainTextEdit *>("report:Search options"); auto *status=viewer->findChild<QLabel *>("reportPageStatus");
      auto find=[&](const QString &value) { query->setText(value); search->click(); finish(*viewer); };
      find("éphéméride"); require(status->text().contains(QString("byte %1.").arg(finalOffset+1)),"Match-case search did not skip different-case Unicode");
      matchCase->setChecked(false); require(!next->isEnabled(),"Changing case retained stale Next match"); find("éphéméride"); require(status->text().contains(QString("byte %1.").arg(unicodeOffset+1)) && status->text().contains("continues on the next page") && text->textCursor().selectedText()=="ÉPH","Case-insensitive Unicode cross-page result/selection wrong"); next->click(); finish(*viewer); require(status->text().contains(QString("byte %1.").arg(finalOffset+1)) && text->textCursor().selectedText()=="éphéméride","Next insensitive Unicode match did not reach beyond 18 MiB");
      words->setChecked(true); find("orbit"); require(status->text().contains(QString("byte %1.").arg(wordOffset+1)) && text->textCursor().selectedText()=="ORBIT","Whole-word search accepted embedded/Unicode/combining-mark word or chunk-edge Orbiting"); next->click(); finish(*viewer); require(text->textCursor().selectedText()=="orbit" && status->text().contains(QString("byte %1.").arg(wordOffset+7)),"Next whole-word match failed"); next->click(); finish(*viewer); require(status->text().contains("No further matches"),"Whole-word search invented an extra match");
      find("edge"); require(status->text().contains("No further matches"),"Retained chunk overlap turned an embedded name into a whole word");
      find("k"); require(status->text().contains(QString("byte %1.").arg(kelvinOffset+1)) && text->textCursor().selectedText()=="K","Case-equivalent different UTF-8 byte lengths lost offset/highlight"); find("𐐨"); require(text->textCursor().selectedText()=="𐐀","Supplementary Unicode case/word boundary failed");
      words->setChecked(false); find("VX\r\nVY"); require(text->textCursor().selectedText().replace(QChar::ParagraphSeparator,'\n')=="VX\nVY","CRLF literal match selection was not normalized correctly");
      query->setText("absent marker"); search->click(); require(stop->isEnabled(),"Search Stop unavailable"); const auto retained=text->toPlainText(); matchCase->setChecked(true); require(!stop->isEnabled() && !next->isEnabled() && text->toPlainText()==retained,"Changing options did not cancel scan/retain display"); search->click(); stop->click(); require(status->text().contains("stopped"),"Search Stop feedback missing"); search->click(); viewer->findChild<QPushButton *>("reportFirst")->click(); require(!stop->isEnabled(),"Report navigation left search running");
      require(QFile::rename(large,large+".old"),"Search fixture rename failed"); search->click(); require(!stop->isEnabled() && status->text().contains("Cannot search") && text->toPlainText()!=QString(),"Missing search file did not preserve displayed text"); require(QFile::rename(large+".old",large),"Search fixture restore failed"); matchCase->setChecked(false); words->setChecked(true); find("éphéméride"); next->click(); finish(*viewer); require(text->textCursor().selectedText()=="éphéméride","Restored file/options did not recover");
      require(read(large)==bytes && read(report)==expected && editor->toPlainText()==source && read(saved)==source.toUtf8(),"Report search changed fixture/calculation/source bytes");
      if (!capture.isEmpty()) { QEventLoop exposed; QTimer::singleShot(200,&exposed,&QEventLoop::quit); exposed.exec(); require(viewer->grab().save(capture+".report.png") && window.grab().save(capture+".png"),"Native report search captures failed"); write(capture+".calculation.txt",expected); }
      query->setText("never present"); search->click(); QPointer<ReportViewer> guarded(viewer); child->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); require(!guarded,"Closing report left scan/timer alive");
      std::cout<<"PASS actual Output case/whole-word controls; Unicode/different-byte-length/supplementary/CRLF and chunk-boundary search, Next beyond 18 MiB, embedded-word exclusion, options/Stop/navigation/close cancellation and missing-file recovery; complete fixture/source/calculation bytes unchanged.\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
