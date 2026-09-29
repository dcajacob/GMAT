#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "QtMessageReceiver.hpp"
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTimer>
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
      QTemporaryDir temporary; require(temporary.isValid(),"Temporary directory unavailable");
      const auto path=temporary.path()+QString::fromUtf8("/mission Δ.script");
      const auto original=QString::fromUtf8("% UTF-8: café Δ\nBeginMissionSequence;\n");
      write(path,original.toUtf8());
      MainWindow window; window.show();
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
