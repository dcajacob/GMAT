#include "ScriptDocument.hpp"
#include <QCloseEvent>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStringDecoder>
#include <QMessageBox>
void ScriptSubWindow::closeEvent(QCloseEvent *event)
{
   if (mayClose && !mayClose()) { event->ignore(); return; }
   QMdiSubWindow::closeEvent(event);
}

bool readScriptDocumentFile(QWidget *parent,const QString &path,QString &text)
{
   QFile file(path);
   if (!file.open(QIODevice::ReadOnly)) { QMessageBox::warning(parent,"Open failed",file.errorString()); return false; }
   const auto bytes=file.readAll();
   if (file.error()!=QFileDevice::NoError) { QMessageBox::warning(parent,"Read failed",file.errorString()); return false; }
   QStringDecoder decoder(QStringDecoder::Utf8,QStringConverter::Flag::Stateless); const QString decoded=decoder(bytes);
   if (decoder.hasError()) { QMessageBox::warning(parent,"Open failed","This script is not valid UTF-8. Convert its encoding before opening it."); return false; }
   text=decoded; return true;
}
QString scriptDocumentIdentity(const QString &path)
{
   const QFileInfo file(path); const auto canonical=file.canonicalFilePath();
   return QDir::cleanPath(canonical.isEmpty() ? file.absoluteFilePath() : canonical);
}
