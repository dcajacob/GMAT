#pragma once
#include <QPointer>
#include <QString>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include "FindReplaceDialog.hpp"
#include <functional>
class QCloseEvent;
bool readScriptDocumentFile(QWidget *parent,const QString &path,QString &text);
QString scriptDocumentIdentity(const QString &path);
struct ScriptDocument
{
   QPointer<QPlainTextEdit> editor;
   QPointer<QMdiSubWindow> window;
   QPointer<FindReplaceDialog> search;
   QString path,savedText;
};
class ScriptSubWindow final : public QMdiSubWindow
{
public:
   std::function<bool()> mayClose;
protected:
   void closeEvent(QCloseEvent *event) override;
};
