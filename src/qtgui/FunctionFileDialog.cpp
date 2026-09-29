#include "FunctionFileDialog.hpp"
#include "ScriptEditor.hpp"
#include "FindReplaceDialog.hpp"
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QSaveFile>
#include <QStringConverter>

FunctionFileDialog::FunctionFileDialog(const QString &filename,QWidget *parent)
   : QDialog(parent),path(QFileInfo(filename).absoluteFilePath())
{
   setObjectName("functionFileDialog"); setWindowTitle("Edit function — "+path); resize(850,600);
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Save updates the function file. Run rebuilds the mission before using it. Cancel discards edits in this window.",this);
   help->setWordWrap(true); layout->addWidget(help);
   editor=new ScriptEditor(this); editor->setObjectName("functionFileText"); layout->addWidget(editor);
   status=new QLabel(this); status->setWordWrap(true); layout->addWidget(status);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   auto *saveAsButton=buttons->addButton("Save As…",QDialogButtonBox::ActionRole); saveAsButton->setObjectName("functionSaveAs");
   connect(saveAsButton,&QPushButton::clicked,this,[this] {
      QFileDialog chooser(this,"Save function as",path,"GMAT functions (*.gmf);;All files (*)");
      chooser.setObjectName("functionSaveAsDialog"); chooser.setAcceptMode(QFileDialog::AcceptSave); chooser.setDefaultSuffix("gmf");
      if (chooser.exec()!=QDialog::Accepted || chooser.selectedFiles().isEmpty()) return;
      const auto error=saveAs(chooser.selectedFiles().first(),true);
      if (error.isEmpty()) accept(); else status->setText(error);
   });
   auto *find=buttons->addButton("Find / replace…",QDialogButtonBox::ActionRole);
   connect(find,&QPushButton::clicked,this,[this] { auto *dialog=new FindReplaceDialog(editor,this); dialog->setAttribute(Qt::WA_DeleteOnClose); dialog->openSearch(); });
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons->button(QDialogButtonBox::Save),&QPushButton::clicked,this,[this] {
      const auto error=save(); if (error.isEmpty()) accept(); else status->setText(error);
   });
   QFile file(path);
   if (!file.open(QIODevice::ReadOnly)) status->setText("Cannot open function file: "+file.errorString());
   else {
      original=file.readAll();
      QStringDecoder decoder(QStringDecoder::Utf8); const QString text=decoder(original);
      if (file.error()!=QFileDevice::NoError) status->setText("Cannot read function file: "+file.errorString());
      else if (decoder.hasError()) status->setText("The function file is not valid UTF-8. It has not been opened for editing.");
      else { loaded=true; editor->setPlainText(text); editor->document()->setModified(false); }
   }
   editor->setReadOnly(!loaded); buttons->button(QDialogButtonBox::Save)->setEnabled(loaded); find->setEnabled(loaded); saveAsButton->setEnabled(loaded);
}
QString FunctionFileDialog::save()
{
   if (!loaded) return "Open a readable UTF-8 function file before saving.";
   QFile current(path);
   if (!current.open(QIODevice::ReadOnly)) return "Cannot check the current function file: "+current.errorString();
   const auto bytes=current.readAll();
   if (current.error()!=QFileDevice::NoError) return "Cannot read the current function file: "+current.errorString();
   if (bytes!=original) return "The function file changed outside this editor. Cancel and reopen it before saving.";
   current.close();
   if (!editor->document()->isModified()) return {};
   return writeTo(path);
}
QString FunctionFileDialog::saveAs(const QString &destination,bool replaceExisting)
{
   if (!loaded) return "Open a readable UTF-8 function file before saving.";
   if (destination.trimmed().isEmpty()) return "Choose a destination file.";
   const auto target=QFileInfo(destination).absoluteFilePath();
   if (target==path || (!QFileInfo(target).canonicalFilePath().isEmpty() && QFileInfo(target).canonicalFilePath()==QFileInfo(path).canonicalFilePath())) return save();
   if (QFileInfo::exists(target) && !replaceExisting) return "The destination already exists. Choose another file or confirm replacement.";
   return writeTo(target);
}
QString FunctionFileDialog::writeTo(const QString &destination)
{
   auto result=editor->toPlainText().toUtf8();
   if (original.contains("\r\n") && !QByteArray(original).replace("\r\n","").contains('\n')) result.replace("\n","\r\n");
   if (original.startsWith("\xef\xbb\xbf") && !result.startsWith("\xef\xbb\xbf")) result.prepend("\xef\xbb\xbf");
   QSaveFile file(destination);
   if (!file.open(QIODevice::WriteOnly)) return "Cannot save function file: "+file.errorString();
   if (file.write(result)!=result.size() || !file.commit()) return "Cannot save function file: "+file.errorString();
   original=result; path=destination; setWindowTitle("Edit function — "+path); editor->document()->setModified(false); return {};
}
