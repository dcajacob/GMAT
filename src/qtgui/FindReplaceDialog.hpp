#pragma once
#include <QDialog>
#include <QTextDocument>
class QPlainTextEdit;
class QComboBox;
class QCheckBox;
class QLabel;
class FindReplaceDialog final : public QDialog
{
public:
   FindReplaceDialog(QPlainTextEdit *editor,QWidget *parent);
   void openSearch();
   bool findNext(bool backward=false);
private:
   QTextDocument::FindFlags flags() const;
   void remember(QComboBox *combo);
   void replaceCurrent();
   void replaceAll();
   QPlainTextEdit *editor;
   QComboBox *search,*replacement;
   QCheckBox *matchCase,*wholeWord;
   QLabel *status;
};
