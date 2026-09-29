#pragma once
#include <QDialog>
#include <QByteArray>
class ScriptEditor;
class QLabel;
class FunctionFileDialog final : public QDialog
{
public:
   explicit FunctionFileDialog(const QString &path,QWidget *parent=nullptr);
   QString save();
   QString saveAs(const QString &destination,bool replaceExisting=false);
   QString savedPath() const { return path; }
private:
   QString writeTo(const QString &destination);
   QString path;
   QByteArray original;
   ScriptEditor *editor;
   QLabel *status;
   bool loaded=false;
};
