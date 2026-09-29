#pragma once
#include <QPlainTextEdit>
#include <QStringList>
class ScriptHighlighter;

class ScriptEditor final : public QPlainTextEdit
{
public:
   explicit ScriptEditor(QWidget *parent=nullptr);
   void setKeywords(const QStringList &words);
protected:
   void resizeEvent(QResizeEvent *event) override;
   void changeEvent(QEvent *event) override;
   bool eventFilter(QObject *object,QEvent *event) override;
private:
   void updateMargin();
   QWidget *numbers;
   ScriptHighlighter *highlighter;
};
