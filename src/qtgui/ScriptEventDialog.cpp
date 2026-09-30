#include "ScriptEventDialog.hpp"
#include "ScriptEditor.hpp"
#include <QPlainTextEdit>
#include <QSplitter>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QRegularExpression>
#include <stdexcept>

namespace {
struct Parts { QString prefix,header,body,footer,suffix; bool valid=false; };
Parts splitEvent(const QString &text)
{
   const QString label="(?:[ \\t]+'(?:''|[^'])*')?";
   const QRegularExpression begin("^[ \\t]*BeginScript"+label+"[ \\t]*;?[ \\t]*(?:%[^\\n]*)?$"),end("^[ \\t]*EndScript"+label+"[ \\t]*;?[ \\t]*(?:%[^\\n]*)?$");
   qsizetype offset=0,headerStart=-1,headerEnd=-1,footerStart=-1,footerEnd=-1; int depth=0;
   for (const auto &line:text.split('\n')) {
      const auto next=std::min(text.size(),offset+line.size()+1); const auto trimmed=line.trimmed();
      if (headerStart<0) {
         if (!trimmed.isEmpty() && !trimmed.startsWith('%')) {
            if (!begin.match(line).hasMatch()) return {};
            headerStart=offset; headerEnd=next; depth=1;
         }
      } else if (footerStart<0) {
         if (begin.match(line).hasMatch()) ++depth;
         if (end.match(line).hasMatch() && --depth==0) { footerStart=offset; footerEnd=next; }
      } else if (!trimmed.isEmpty() && !trimmed.startsWith('%')) return {};
      offset=next;
   }
   if (headerStart<0 || footerStart<headerEnd || depth!=0) return {};
   return {text.left(headerStart),text.mid(headerStart,headerEnd-headerStart),text.mid(headerEnd,footerStart-headerEnd),text.mid(footerStart,footerEnd-footerStart),text.mid(footerEnd),true};
}
QString commentText(const QString &prefix)
{
   QStringList lines;
   for (auto line:prefix.split('\n')) {
      const auto marker=line.indexOf('%');
      if (marker>=0) { line=line.mid(marker+1); if (line.startsWith(' ')) line.remove(0,1); }
      lines.append(line);
   }
   if (prefix.endsWith('\n')) lines.removeLast();
   return lines.join('\n');
}
}
bool ScriptEventDialog::supports(const QString &statement) { return splitEvent(statement).valid; }
ScriptEventDialog::ScriptEventDialog(const QString &statement,QWidget *parent) : QDialog(parent)
{
   const auto parts=splitEvent(statement); if (!parts.valid) throw std::runtime_error("Select a complete BeginScript/EndScript event.");
   prefix=parts.prefix; header=parts.header; footer=parts.footer; suffix=parts.suffix; originalBody=parts.body; originalComments=commentText(prefix);
   setObjectName("scriptEventDialog"); setWindowTitle("Script event"); resize(780,600);
   auto *layout=new QVBoxLayout(this); auto *help=new QLabel("Edit the event's comments and commands. OK keeps changes pending until command Apply.",this); help->setWordWrap(true); layout->addWidget(help);
   auto *splitter=new QSplitter(Qt::Vertical,this); splitter->setChildrenCollapsible(false); layout->addWidget(splitter,1);
   auto *commentArea=new QWidget(splitter); auto *commentLayout=new QVBoxLayout(commentArea); commentLayout->setContentsMargins(0,0,0,0); commentLayout->addWidget(new QLabel("Comments",commentArea));
   comments=new QPlainTextEdit(originalComments,commentArea); comments->setObjectName("scriptEventComments"); comments->setLineWrapMode(QPlainTextEdit::NoWrap); comments->setMinimumHeight(55); commentLayout->addWidget(comments);
   auto *scriptArea=new QWidget(splitter); auto *scriptLayout=new QVBoxLayout(scriptArea); scriptLayout->setContentsMargins(0,0,0,0);
   auto boundary=[&](const QString &text,const QString &name) { auto *label=new QLabel(text.trimmed(),scriptArea); label->setTextFormat(Qt::PlainText); label->setObjectName(name); label->setWordWrap(true); label->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont)); auto colors=label->palette(); colors.setColor(QPalette::WindowText,colors.color(QPalette::Link)); label->setPalette(colors); scriptLayout->addWidget(label); };
   boundary(header,"scriptEventBegin"); body=new ScriptEditor(scriptArea); body->setObjectName("scriptEventBody"); body->setKeywords({"BeginScript","EndScript","Propagate","Report","Maneuver","Target","EndTarget","Optimize","EndOptimize"}); body->setPlainText(originalBody); scriptLayout->addWidget(body,1); boundary(footer,"scriptEventEnd"); splitter->setSizes({110,380});
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons); connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
}
QString ScriptEventDialog::statement() const
{
   QString leading=prefix;
   if (comments->toPlainText()!=originalComments) {
      leading.clear();
      if (!comments->toPlainText().isEmpty()) for (const auto &line:comments->toPlainText().split('\n')) leading+="% "+line+"\n";
   }
   auto commands=body->toPlainText(); if (!commands.isEmpty() && !commands.endsWith('\n')) commands+='\n';
   return leading+header+commands+footer+suffix;
}
