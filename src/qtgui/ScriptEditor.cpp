#include "ScriptEditor.hpp"
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QSet>
#include <QPainter>
#include <QPaintEvent>
#include <QTextBlock>
#include <QFontDatabase>

class ScriptHighlighter final : public QSyntaxHighlighter
{
public:
   explicit ScriptHighlighter(ScriptEditor *owner) : QSyntaxHighlighter(owner->document()),editor(owner) {}
   QSet<QString> keywords;
protected:
   void highlightBlock(const QString &text) override
   {
      const bool dark=editor->palette().color(QPalette::Base).lightness()<128;
      const QColor command=dark ? QColor("#82b7ff") : QColor("#164a9c");
      const QColor comment=dark ? QColor("#9bbd91") : QColor("#386638");
      const QColor literal=dark ? QColor("#e7b980") : QColor("#8b410c");
      static const QRegularExpression tokens(
         "(%.*)|('(?:''|[^'])*'?)|((?<![\\w.])(?:[0-9]+(?:\\.[0-9]*)?|\\.[0-9]+)(?:[eE][+-]?[0-9]+)?(?![\\w.]))|([A-Za-z_][A-Za-z_0-9]*)");
      auto matches=tokens.globalMatch(text);
      while (matches.hasNext()) {
         const auto match=matches.next();
         QTextCharFormat format;
         if (match.capturedStart(1)>=0) format.setForeground(comment);
         else if (match.capturedStart(2)>=0 || match.capturedStart(3)>=0) format.setForeground(literal);
         else if (keywords.contains(match.captured(4)) &&
                  (match.capturedStart()==0 || text[match.capturedStart()-1]!='.')) {
            format.setForeground(command); format.setFontWeight(QFont::Bold);
         } else continue;
         setFormat(match.capturedStart(),match.capturedLength(),format);
      }
   }
private:
   ScriptEditor *editor;
};

ScriptEditor::ScriptEditor(QWidget *parent) : QPlainTextEdit(parent),numbers(new QWidget(this)),highlighter(new ScriptHighlighter(this))
{
   setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
   setLineWrapMode(QPlainTextEdit::NoWrap);
   numbers->setObjectName("lineNumbers");
   numbers->installEventFilter(this);
   connect(this,&QPlainTextEdit::blockCountChanged,this,[this] { updateMargin(); });
   connect(this,&QPlainTextEdit::updateRequest,this,[this](const QRect &rect,int delta) {
      if (delta) numbers->scroll(0,delta);
      else numbers->update(0,rect.y(),numbers->width(),rect.height());
   });
   setKeywords({"GMAT","Create","Global","BeginMissionSequence","If","Else","EndIf","For","EndFor","While","EndWhile"});
   updateMargin();
}
void ScriptEditor::setKeywords(const QStringList &words)
{
   for (const auto &word:words) highlighter->keywords.insert(word);
   highlighter->rehighlight();
}
void ScriptEditor::updateMargin()
{
   const int width=12+fontMetrics().horizontalAdvance('9')*QString::number(blockCount()).size();
   setViewportMargins(width,0,0,0);
   const auto area=contentsRect();
   numbers->setGeometry(area.x(),area.y(),width,area.height());
   numbers->update();
}
void ScriptEditor::resizeEvent(QResizeEvent *event)
{
   QPlainTextEdit::resizeEvent(event); updateMargin();
}
void ScriptEditor::changeEvent(QEvent *event)
{
   QPlainTextEdit::changeEvent(event);
   if (event->type()==QEvent::FontChange || event->type()==QEvent::PaletteChange) {
      updateMargin(); highlighter->rehighlight();
   }
}
bool ScriptEditor::eventFilter(QObject *object,QEvent *event)
{
   if (object!=numbers || event->type()!=QEvent::Paint) return QPlainTextEdit::eventFilter(object,event);
   const auto *paint=static_cast<QPaintEvent *>(event);
   QPainter painter(numbers);
   painter.fillRect(paint->rect(),palette().color(QPalette::AlternateBase));
   painter.setPen(palette().color(QPalette::Text));
   painter.setOpacity(0.65);
   painter.setFont(font());
   auto block=firstVisibleBlock();
   int y=qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
   while (block.isValid() && y<=paint->rect().bottom()) {
      const int height=qRound(blockBoundingRect(block).height());
      if (block.isVisible() && y+height>=paint->rect().top())
         painter.drawText(0,y,numbers->width()-6,fontMetrics().height(),Qt::AlignRight,QString::number(block.blockNumber()+1));
      y+=height; block=block.next();
   }
   return true;
}
