#include "UserParameter.hpp"
#include <QRegularExpression>
#include <QVector>
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace {
struct Statement { QString code; QVector<qsizetype> positions; };
QVector<Statement> statements(const QString &source)
{
   QVector<Statement> result; Statement current;
   bool quoted=false; int depth=0;
   for (qsizetype i=0;i<source.size();++i) {
      const auto ch=source[i];
      if (!quoted && ch=='%') { while (i<source.size() && source[i]!='\n') ++i; if (i==source.size()) break; }
      const auto c=source[i]; current.code+=c; current.positions.append(i);
      if (c=='\'' && !quoted && QRegularExpression("^\\s*(?:GMAT\\s+)?[A-Za-z][A-Za-z0-9_.]*(?:\\([^;]*\\))?\\s*=\\s*$").match(current.code.chopped(1)).hasMatch()) {
         // GMAT string literals preserve inner apostrophes rather than using
         // doubled-quote escaping. Treat the literal as one block through its
         // closing quote before a terminator or trailing comment.
         const auto newline=source.indexOf('\n',i); const auto end=newline<0 ? source.size() : newline;
         qsizetype closing=-1;
         for (auto q=i+1;q<end;++q) if (source[q]=='\'' && QRegularExpression("^[ \\t\\r]*(?:;[ \\t]*)?(?:%[^\\n]*)?$").match(source.mid(q+1,end-q-1)).hasMatch()) { closing=q; break; }
         if (closing>=0) {
            while (i<closing) { current.code+=source[++i]; current.positions.append(i); }
            continue;
         }
      }
      if (c=='\'') {
         if (quoted && i+1<source.size() && source[i+1]=='\'') { current.code+=source[++i]; current.positions.append(i); }
         else quoted=!quoted;
      } else if (!quoted) {
         if (c=='[' || c=='{' || c=='(') ++depth;
         if (c==']' || c=='}' || c==')') --depth;
         if (c==';' && depth==0) { result.append(current); current={}; }
         else if (c=='\n' && depth==0 && !current.code.trimmed().isEmpty()) {
            const auto trimmed=current.code.trimmed();
            if (trimmed.endsWith("...")) {
               const auto dots=current.code.lastIndexOf("..."); current.code.remove(dots,3); current.positions.remove(dots,3);
            } else if (!trimmed.endsWith('=') && !QRegularExpression("^Create\\s+[A-Za-z][A-Za-z0-9_]*$").match(trimmed).hasMatch()) {
               result.append(current); current={};
            }
         }
      }
   }
   if (!current.code.trimmed().isEmpty() && !quoted && depth==0) result.append(current);
   if (quoted || depth!=0)
      throw std::runtime_error("The script contains an incomplete statement. Build it before editing a parameter.");
   return result;
}
}
QString userParameterLiteral(const QString &type,const QString &value)
{
   if (type=="Variable") {
      bool ok=false; const auto number=value.trimmed().toDouble(&ok);
      if (!ok || !std::isfinite(number)) throw std::runtime_error("Enter a finite numeric initial value. Use mission assignments for expressions.");
      return QString::number(number,'g',17);
   }
   if (type!="String") throw std::runtime_error("Initial values require a Variable or String.");
   for (const auto ch:value) if (ch.unicode()<32 || ch==QChar(0x2028) || ch==QChar(0x2029))
      throw std::runtime_error("GMAT string initializers use one single-quoted line. Remove control characters.");
   return "'"+value+"'";
}
QString setUserParameterValue(const QString &source,const QString &name,const QString &type,const QString &value,const QString &firstMissionStatement)
{
   const auto literal=userParameterLiteral(type,value);
   const QRegularExpression assignment("^\\s*(?:GMAT\\s+)?("+QRegularExpression::escape(name)+")\\s*=\\s*(.*?)\\s*;?\\s*$",QRegularExpression::DotMatchesEverythingOption);
   const QRegularExpression declaration("^\\s*Create\\s+"+QRegularExpression::escape(type)+"\\s+([^;]+?);?\\s*$",QRegularExpression::DotMatchesEverythingOption);
   const QRegularExpression begin("^\\s*BeginMissionSequence\\b");
   const auto parsed=statements(source);
   qsizetype rhsStart=-1,rhsEnd=-1,createEnd=-1;
   auto normalized=[](QString code) {
      code=code.trimmed(); if (code.endsWith(';')) code.chop(1); code.remove(QRegularExpression("^GMAT\\s+"));
      QString key; bool quoted=false;
      for (auto ch:code) { if (ch=='\'') quoted=!quoted; if (quoted || !ch.isSpace()) key+=ch; }
      return key;
   };
   QString firstCommand;
   if (!firstMissionStatement.isEmpty()) {
      const auto commands=statements(firstMissionStatement); if (!commands.isEmpty()) firstCommand=normalized(commands.first().code);
   }
   bool locatedBoundary=firstCommand.isEmpty();
   for (const auto &statement:parsed) {
      if (begin.match(statement.code).hasMatch() || (!firstCommand.isEmpty() && normalized(statement.code)==firstCommand)) { locatedBoundary=true; break; }
      const auto create=declaration.match(statement.code);
      if (create.hasMatch() && create.captured(1).split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts).contains(name)) createEnd=statement.positions.last()+1;
      const auto match=assignment.match(statement.code);
      if (!match.hasMatch()) continue;
      rhsStart=statement.positions[match.capturedStart(2)]; rhsEnd=statement.positions[match.capturedEnd(2)-1]+1;
   }
   if (!locatedBoundary) throw std::runtime_error("Cannot locate the mission boundary safely. Add BeginMissionSequence before editing the initializer.");
   if (createEnd<0) throw std::runtime_error("Cannot locate this parameter's declaration safely. Use its script settings.");
   QString candidate=source;
   if (rhsStart>=0) {
      struct Edit { qsizetype start,length; QString text; };
      QVector<Edit> edits{{rhsStart,rhsEnd-rhsStart,literal}};
      // TextParser stops checking for comments after a percent sign inside a
      // literal. Preserve a trailing comment on its own line in this case.
      if (type=="String" && value.contains('%')) {
         const auto trailing=QRegularExpression("^[ \\t]*;?[ \\t]*(%[^\\r\\n]*)").match(source.mid(rhsEnd));
         if (trailing.hasMatch()) {
            const auto comment=trailing.captured(1);
            edits.append({rhsEnd+trailing.capturedStart(1),comment.size(),{}});
            edits.append({source.lastIndexOf('\n',rhsStart-1)+1,0,comment+"\n"});
         }
      }
      std::stable_sort(edits.begin(),edits.end(),[](const Edit &a,const Edit &b) { return a.start>b.start; });
      for (const auto &edit:edits) candidate.replace(edit.start,edit.length,edit.text);
   }
   else {
      // Keep an inline declaration comment attached to its declaration.
      auto after=createEnd; while (after<source.size() && (source[after]==' ' || source[after]=='\t')) ++after;
      if (after<source.size() && source[after]=='%') { const auto end=source.indexOf('\n',after); createEnd=end<0 ? source.size() : end+1; }
      candidate.insert(createEnd,"\nGMAT "+name+" = "+literal+";\n");
   }
   return candidate;
}
