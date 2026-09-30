#pragma once
#include <QDialog>
class QPlainTextEdit;
class ScriptEditor;
class ScriptEventDialog final : public QDialog
{
public:
   explicit ScriptEventDialog(const QString &statement,QWidget *parent=nullptr);
   static bool supports(const QString &statement);
   QString statement() const;
private:
   QString prefix,header,footer,suffix,originalComments,originalBody;
   QPlainTextEdit *comments;
   ScriptEditor *body;
};
