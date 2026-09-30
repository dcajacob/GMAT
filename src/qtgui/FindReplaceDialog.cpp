#include "FindReplaceDialog.hpp"
#include <QPlainTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTextCursor>

FindReplaceDialog::FindReplaceDialog(QPlainTextEdit *source,QWidget *parent) : QDialog(parent),editor(source)
{
   setObjectName("findReplaceDialog"); setWindowTitle(source->isReadOnly() ? "Find in text" : "Find and replace in script");
   auto *layout=new QVBoxLayout(this); auto *form=new QFormLayout; layout->addLayout(form);
   search=new QComboBox(this); search->setEditable(true); search->setObjectName("findText");
   replacement=new QComboBox(this); replacement->setEditable(true); replacement->setObjectName("replaceText");
   for (auto *combo:{search,replacement}) { combo->setInsertPolicy(QComboBox::NoInsert); combo->setMinimumWidth(300); }
   form->addRow("Find",search);
   if (!source->isReadOnly()) form->addRow("Replace with",replacement); else replacement->hide();
   auto *options=new QHBoxLayout; layout->addLayout(options);
   matchCase=new QCheckBox("Match case",this); matchCase->setObjectName("findMatchCase"); options->addWidget(matchCase);
   wholeWord=new QCheckBox("Whole words",this); wholeWord->setObjectName("findWholeWords"); options->addWidget(wholeWord);
   auto *buttons=new QHBoxLayout; layout->addLayout(buttons);
   auto add=[&](const QString &label,const QString &name,auto callback) {
      auto *button=new QPushButton(label,this); button->setObjectName(name); buttons->addWidget(button);
      button->setAutoDefault(false); connect(button,&QPushButton::clicked,this,callback); return button;
   };
   add("Previous","findPrevious",[this] { findNext(true); });
   add("Next","findNext",[this] { findNext(); });
   if (!source->isReadOnly()) {
      add("Replace","replaceCurrent",[this] { replaceCurrent(); });
      add("Replace all","replaceAll",[this] { replaceAll(); });
   }
   add("Close","findClose",[this] { hide(); });
   status=new QLabel(this); status->setObjectName("findStatus"); layout->addWidget(status);
   connect(search->lineEdit(),&QLineEdit::returnPressed,this,[this] { findNext(); });
}
void FindReplaceDialog::openSearch()
{
   const auto selected=editor->textCursor().selectedText();
   if (!selected.isEmpty() && !selected.contains(QChar::ParagraphSeparator)) search->setEditText(selected);
   show(); raise(); activateWindow(); search->setFocus(); search->lineEdit()->selectAll();
}
QTextDocument::FindFlags FindReplaceDialog::flags() const
{
   QTextDocument::FindFlags result;
   if (matchCase->isChecked()) result|=QTextDocument::FindCaseSensitively;
   if (wholeWord->isChecked()) result|=QTextDocument::FindWholeWords;
   return result;
}
void FindReplaceDialog::remember(QComboBox *combo)
{
   const auto text=combo->currentText(); const int old=combo->findText(text);
   if (old>=0) combo->removeItem(old);
   combo->insertItem(0,text); combo->setCurrentIndex(0);
   while (combo->count()>12) combo->removeItem(combo->count()-1);
}
bool FindReplaceDialog::findNext(bool backward)
{
   if (search->currentText().isEmpty()) { openSearch(); status->setText("Enter text to find."); return false; }
   remember(search); auto options=flags(); if (backward) options|=QTextDocument::FindBackward;
   auto found=editor->document()->find(search->currentText(),editor->textCursor(),options);
   bool wrapped=found.isNull();
   if (wrapped) {
      QTextCursor start(editor->document()); start.movePosition(backward ? QTextCursor::End : QTextCursor::Start);
      found=editor->document()->find(search->currentText(),start,options);
   }
   if (found.isNull()) { status->setText("No matches found."); show(); raise(); return false; }
   editor->setTextCursor(found); editor->ensureCursorVisible(); status->setText(wrapped ? "Search wrapped." : "Match found."); return true;
}
void FindReplaceDialog::replaceCurrent()
{
   if (editor->isReadOnly() || search->currentText().isEmpty()) return;
   auto selected=editor->textCursor();
   const auto match=editor->document()->find(search->currentText(),selected.selectionStart(),flags());
   if (!selected.hasSelection() || match.isNull() || match.selectionStart()!=selected.selectionStart() || match.selectionEnd()!=selected.selectionEnd()) { findNext(); return; }
   remember(search); remember(replacement); selected.insertText(replacement->currentText()); editor->setTextCursor(selected); findNext();
}
void FindReplaceDialog::replaceAll()
{
   if (editor->isReadOnly() || search->currentText().isEmpty()) return;
   remember(search); remember(replacement);
   QTextCursor cursor(editor->document()); cursor.beginEditBlock(); int count=0;
   while (true) {
      const auto found=editor->document()->find(search->currentText(),cursor,flags());
      if (found.isNull()) break;
      cursor=found; cursor.insertText(replacement->currentText()); ++count;
   }
   cursor.endEditBlock(); status->setText(QString("Replaced %1 occurrence(s).").arg(count));
}
