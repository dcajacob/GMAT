#include "MainWindow.hpp"
#include "ReportViewer.hpp"
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QElapsedTimer>
#include "TestSettings.hpp"
#include "QtMessageReceiver.hpp"
#include "StartupCompatibility.hpp"
#include "ScriptEditor.hpp"
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTimer>
#include <QTextBlock>
#include <QTextLayout>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static void write(const QString &path,const QByteArray &bytes)
{
   QFile file(path); require(file.open(QIODevice::WriteOnly),"Fixture open failed");
   require(file.write(bytes)==bytes.size(),"Fixture write failed");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtFiles");
   try {
      TestSettings isolatedSettings;
      ScriptEditor syntax;
      syntax.resize(500,180); syntax.show();
      const QString sample="Create Spacecraft Sat; % Propagate\nGMAT Sat.Label = '50% complete';\nTestCommand Sat;\n";
      syntax.setPlainText(sample); syntax.document()->setModified(false);
      syntax.setKeywords({"Spacecraft","TestCommand"}); QApplication::processEvents();
      auto formatAt=[&](int line,int column) {
         for (const auto &range:syntax.document()->findBlockByNumber(line).layout()->formats())
            if (range.start<=column && column<range.start+range.length) return range.format;
         return QTextCharFormat();
      };
      require(formatAt(0,0).fontWeight()==QFont::Bold && formatAt(2,0).fontWeight()==QFont::Bold,"GMAT or plugin keywords were not highlighted");
      require(formatAt(0,26).fontWeight()!=QFont::Bold,"Comment text was highlighted as a command");
      require(formatAt(1,20).foreground()!=formatAt(0,26).foreground(),"Percent inside quoted string became a comment");
      require(syntax.toPlainText()==sample && !syntax.document()->isModified(),"Highlighting changed script text or dirty state");
      auto *gutter=syntax.findChild<QWidget *>("lineNumbers");
      require(gutter && gutter->isVisible(),"Script line numbers are missing");
      const int narrow=gutter->width();
      syntax.setPlainText(QString("% line\n").repeated(100)); QApplication::processEvents();
      require(gutter->width()>narrow,"Line-number margin did not grow with the document");
      syntax.close();
      QTemporaryDir temporary; require(temporary.isValid(),"Temporary directory unavailable");
      {
         const auto report=temporary.filePath("paged-report.txt");
         constexpr int chunk=1024*1024;
         QByteArray bytes;
         while (bytes.size()<chunk-1) bytes.append("row 123\n");
         bytes.truncate(chunk-1); bytes.append(QString::fromUtf8("€").toUtf8());
         while (bytes.size()<chunk*2-2) bytes.append("next row\n");
         bytes.truncate(chunk*2-2); bytes.append(QString::fromUtf8("🚀").toUtf8()); bytes.append("\nlast row\n");
         while (bytes.size()<chunk*3-1) bytes.append("more rows\n");
         bytes.truncate(chunk*3-1); bytes.append("\r\nfinal row\n");
         write(report,bytes); ReportViewer viewer(report,"Paged");
         auto *text=viewer.findChild<QPlainTextEdit *>("report:Paged");
         require(text && text->isReadOnly(),"Report paging permits edits");
         QString joined=text->toPlainText();
         while (viewer.findChild<QPushButton *>("reportNext")->isEnabled()) {
            viewer.findChild<QPushButton *>("reportNext")->click(); joined+=text->toPlainText();
         }
         require(joined==QString::fromUtf8(bytes).replace("\r\n","\n"),"Report paging split UTF-8/CRLF or lost/duplicated content");
         require(!viewer.findChild<QPushButton *>("reportNext")->isEnabled(),"Last report page offers Next");
         viewer.findChild<QPushButton *>("reportPrevious")->click();
         require(viewer.findChild<QSpinBox *>("reportPage")->value()==3,"Previous page failed");
         viewer.findChild<QSpinBox *>("reportPage")->setValue(1);
         require(text->toPlainText().startsWith("row 123"),"Jump to report page failed");
         auto *query=viewer.findChild<QLineEdit *>("reportFileSearchText");
         auto *search=viewer.findChild<QPushButton *>("reportSearchFile"),*stop=viewer.findChild<QPushButton *>("reportStopSearch");
         auto complete=[&] { QElapsedTimer timer; timer.start(); while (stop->isEnabled() && timer.elapsed()<10000) app.processEvents(QEventLoop::AllEvents,10); require(!stop->isEnabled(),"File search did not finish"); };
         query->setText(QString::fromUtf8("123€next")); search->click(); complete();
         require(viewer.findChild<QLabel *>("reportPageStatus")->text().contains("continues on the next page") && text->textCursor().selectedText()=="123", "Cross-page file match was missed or highlighted incorrectly");
         query->setText(QString::fromUtf8("€")); search->click(); complete();
         require(text->textCursor().selectedText()==QString::fromUtf8("€"),"UTF-8 boundary match did not navigate/highlight");
         query->setText("next row"); search->click(); complete(); const auto matchPosition=text->textCursor().selectionStart();
         viewer.findChild<QPushButton *>("reportNextMatch")->click(); complete();
         require(text->textCursor().selectionStart()>matchPosition,"Next file match did not advance");
         query->setText("absent phrase"); search->click(); require(stop->isEnabled(),"Search cannot be canceled"); stop->click();
         require(viewer.findChild<QLabel *>("reportPageStatus")->text().contains("stopped"),"Canceled search lacked feedback");
         search->click(); complete(); require(viewer.findChild<QLabel *>("reportPageStatus")->text().contains("No further matches"),"Missing search text not diagnosed");
         const auto retained=text->toPlainText(); require(QFile::rename(report,report+".old"),"Report rename failed");
         viewer.findChild<QPushButton *>("reportReload")->click();
         require(text->toPlainText()==retained && viewer.findChild<QLabel *>("reportPageStatus")->text().contains("unavailable"),"Missing report discarded displayed content or hid error");
         QFile large(report); require(large.open(QIODevice::WriteOnly) && large.resize(17*chunk) && large.seek(17*chunk),"Large report fixture failed");
         large.write("beyond original preview limit\n"); large.close();
         query->setText("beyond original"); search->click(); complete();
         require(text->textCursor().selectedText()=="beyond original","Complete-file search did not reach beyond preview limit");
         viewer.findChild<QPushButton *>("reportLast")->click();
         require(text->toPlainText()=="beyond original preview limit\n","Cannot inspect report beyond 16 MiB");
         write(report,"short replacement\n"); viewer.findChild<QPushButton *>("reportReload")->click();
         require(text->toPlainText()=="short replacement\n" && viewer.findChild<QSpinBox *>("reportPage")->value()==1,"Shrunk/replaced report did not reload and clamp page");
      }
      const auto startup=temporary.path()+"/startup.txt";
      for (const auto &plugin : {"../plugins/libOpenFramesInterface", "C:\\GMAT Test\\plugins\\libOVtoOFId.dll",
                                "/opt/gmat/libOpenFramesInterface.so.1", "libOVtoOFI.dylib"}) {
         write(startup,QByteArray("# test startup\n  PLUGIN = ")+plugin+"\r\n");
         const auto issue=qtStartupCompatibilityError(startup);
         require(issue.contains("wxWidgets-only") && issue.contains("line 2"),"wx plugin was not diagnosed");
      }
      write(startup,"# PLUGIN = libOpenFramesInterface\nPLUGIN = ../plugins/libGmatEstimation\nPLUGIN = libOpenFramesInterfaceHelper\n");
      require(qtStartupCompatibilityError(startup).isEmpty(),"Comment or unrelated native plugin was rejected");
      const auto path=temporary.path()+QString::fromUtf8("/mission Δ.script");
      const auto original=QString::fromUtf8("% UTF-8: café Δ\nBeginMissionSequence;\n");
      write(path,original.toUtf8());
      MainWindow window; window.show();
      write(startup,"PLUGIN = libOpenFramesInterface\n");
      require(!window.initialize(startup),"Qt initialized a wx-only plugin configuration");
      bool diagnosed=false;
      for (auto *text : window.findChildren<QPlainTextEdit *>())
         diagnosed=diagnosed || text->toPlainText().contains("wxWidgets-only");
      require(diagnosed,"Startup compatibility error was not shown in the GUI");
      require(window.loadScript(path),"UTF-8 script did not open");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      require(editor && editor->toPlainText()==original,"UTF-8 script changed while loading");
      editor->moveCursor(QTextCursor::End); editor->insertPlainText("% unsaved change\n");
      const auto edited=editor->toPlainText(),title=window.windowTitle();
      auto dismissWarning=[&] {
         QTimer::singleShot(0,&window,[] {
            if (auto *dialog=qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) dialog->accept();
         });
      };
      auto preserved=[&] {
         require(editor->toPlainText()==edited && editor->document()->isModified(),"Failed IO lost edits or dirty state");
         require(window.windowTitle()==title,"Failed IO changed document identity");
         editor->undo(); require(editor->toPlainText()==original,"Failed IO lost undo history");
         editor->redo(); require(editor->toPlainText()==edited,"Failed IO lost redo history");
      };
      dismissWarning(); require(!window.loadScript(temporary.path()+"/missing.script"),"Missing file accepted"); preserved();
      const auto malformed=temporary.path()+"/malformed.script";
      write(malformed,QByteArray("% invalid byte: \xff\n"));
      dismissWarning(); require(!window.loadScript(malformed),"Malformed UTF-8 was silently replaced"); preserved();
      write(malformed,QByteArray("% truncated sequence: \xc3"));
      dismissWarning(); require(!window.loadScript(malformed),"Truncated UTF-8 was silently dropped"); preserved();
      const auto failedDestination=temporary.path()+"/missing-parent/new-name.script";
      dismissWarning(); require(!window.saveScriptTo(failedDestination),"Save to missing directory succeeded"); preserved();
      QFile originalFile(path); require(originalFile.open(QIODevice::ReadOnly),"Original file vanished");
      require(originalFile.readAll()==original.toUtf8(),"Failed save modified original file"); originalFile.close();
      const auto savedPath=temporary.path()+"/saved.script";
      require(window.saveScriptTo(savedPath),"Valid save failed");
      require(!editor->document()->isModified() && window.windowTitle().contains("saved.script"),"Successful save did not adopt new identity");
      QFile saved(savedPath); require(saved.open(QIODevice::ReadOnly) && saved.readAll()==edited.toUtf8(),"Saved bytes differ from editor");
      QString delivered;
      QtMessageReceiver receiver; receiver.SetLogEnable(false);
      receiver.SetMessageCallback(&window,[&](const QString &text) { delivered=text; });
      receiver.ShowMessage(original.toStdString()); QApplication::processEvents();
      require(delivered==original,"UTF-8 message decoding failed");
      const QByteArray legacy("legacy trailing byte: \xc3");
      receiver.ShowMessage(legacy.toStdString()); QApplication::processEvents();
      require(delivered==QString::fromLocal8Bit(legacy),"Incomplete UTF-8 message did not use locale fallback");
      std::cout<<"PASS: UTF-8 round trip, failed-open/save identity and dirty-state retention, undo/redo, malformed input rejection, save recovery\n";
   } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
