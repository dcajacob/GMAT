#pragma once
#include <QDialog>
#include <QByteArray>
// Function declarations use the file basename; a resource may be an alias.
QString functionFileTemplate(const QString &path);
class ScriptEditor;
class QLabel;
class FunctionFileDialog final : public QDialog
{
public:
   explicit FunctionFileDialog(const QString &path,QWidget *parent=nullptr,const QString &initialText={});
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
   bool newFile=false;
};
