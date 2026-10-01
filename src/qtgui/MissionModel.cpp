#include "MissionModel.hpp"
#include "GmatCommand.hpp"
#include "Moderator.hpp"
#include "ScriptStatements.hpp"
#include <QSet>
#include <QMap>
#include <QRegularExpression>
#include <stdexcept>

namespace {
struct Line { qsizetype start, end; QString normalized; };
QVector<Line> lines(const QString &text)
{
   QVector<Line> result;
   qsizetype offset = 0;
   for (const auto &line : text.split('\n')) {
      if (!line.trimmed().isEmpty()) result.append({offset, std::min(text.size(), offset+line.size()+1), line.trimmed()});
      offset += line.size()+1;
   }
   return result;
}
QPair<qsizetype, qsizetype> locate(const QVector<Line> &document, const QString &statement,
                                qsizetype begin, qsizetype end)
{
   const auto wanted=lines(statement);
   if (wanted.isEmpty()) return {-1,-1};
   for (qsizetype i=0;i<document.size();++i) {
      if (document[i].start<begin || document[i].end>end) continue;
      qsizetype j=0;
      while (j<wanted.size() && i+j<document.size() && document[i+j].end<=end &&
             document[i+j].normalized==wanted[j].normalized) ++j;
      if (j==wanted.size()) return {document[i].start,document[i+j-1].end};
   }
   return {-1,-1};
}
class Builder
{
public:
   MissionSnapshot result;
   QVector<GmatCommand *> commands;
   QVector<Line> document;
   QSet<GmatCommand *> visited;
   bool expandScriptEvents=false;
   void collect(GmatCommand *command, GmatCommand *stop, int parent,
                qsizetype begin, qsizetype end, int depth = 0)
   {
      if (depth>100) throw std::runtime_error("Mission nesting is too deep to display");
      int scriptDepth=0;
      while (command && command!=stop) {
         if (visited.contains(command)) throw std::runtime_error("Unexpected cycle in mission sequence");
         visited.insert(command);
         const auto type=QString::fromStdString(command->GetTypeName());
         if (scriptDepth>0) {
            if (type=="BeginScript") ++scriptDepth;
            if (type=="EndScript") --scriptDepth;
            command=command->GetNext(); continue;
         }
         if (type=="NoOp") { command=command->GetNext(); continue; }
         MissionNode node;
         node.type=type;
         const auto name=QString::fromStdString(command->GetName());
         node.label=name.isEmpty() ? type : type+" — "+name;
         node.statement=QString::fromStdString(command->GetGeneratingString(Gmat::SCRIPTING)).trimmed();
         if (expandScriptEvents && type=="BeginScript") {
            const auto statements=scriptStatements(node.statement);
            if (!statements.isEmpty()) node.statement=statements.first().code.trimmed();
         }
         if (name.isEmpty()) {
            for (auto line : node.statement.split('\n')) {
               line=line.trimmed();
               if (line.isEmpty() || line.startsWith('%')) continue;
               if (line.startsWith("GMAT ")) line.remove(0,5);
               if (line.endsWith(';')) line.chop(1);
               node.label=line;
               break;
            }
         }
         const auto range=locate(document,node.statement,begin,end);
         node.start=range.first; node.end=range.second; node.parent=parent;
         const QSet<QString> structural={"BeginMissionSequence","Else","EndIf","EndFor","EndWhile","EndTarget","EndOptimize","EndScript"};
         node.editable=node.start>=0 && !structural.contains(type);
         const int index=result.nodes.size(); result.nodes.append(node); commands.append(command);
         if (parent<0) result.roots.append(index); else result.nodes[parent].children.append(index);
         if (node.start>=0) {
            qsizetype childStart=node.start;
            for (int branch=0;auto *child=command->GetChildCommand(branch);++branch) {
               const int before=result.nodes.size();
               collect(child,command,index,childStart,node.end,depth+1);
               for (int n=before;n<result.nodes.size();++n) childStart=std::max(childStart,result.nodes[n].end);
            }
            begin=node.end;
         }
         if (type=="BeginScript" && !expandScriptEvents) scriptDepth=1;
         command=command->GetNext();
      }
   }
};

QString key(QString code)
{
   code=code.trimmed(); if (code.endsWith(';')) code.chop(1);
   code.remove(QRegularExpression("^GMAT\\s+"));
   QString result; bool quoted=false;
   static const QRegularExpression number(R"(^(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?)");
   for (qsizetype index=0;index<code.size();++index) {
      const auto ch=code[index];
      if (ch=='\'') quoted=!quoted;
      // A numeric unary plus is omitted by command serialization. Keep binary
      // addition and quoted labels significant while accepting the source's
      // explicit positive initial guess (including scientific notation).
      if (!quoted && ch=='+' && !result.isEmpty() && QString("=([{,:").contains(result.back()) &&
          number.match(code.mid(index+1)).hasMatch()) continue;
      if (!quoted && (ch.isDigit() || ch=='.') && (index==0 || (!code[index-1].isLetterOrNumber() && code[index-1]!='_' && code[index-1]!='.'))) {
         const auto numeric=number.match(code.mid(index));
         if (numeric.hasMatch()) {
            const auto end=index+numeric.capturedLength();
            if (end==code.size() || (!code[end].isLetterOrNumber() && code[end]!='_' && code[end]!='.')) {
               bool ok=false; const auto value=numeric.captured().toDouble(&ok);
               if (ok) { result+=QString::number(value,'g',17); index=end-1; continue; }
            }
         }
      }
      if (quoted || !ch.isSpace()) result+=ch;
   }
   return result;
}
QString propagationKey(const QString &code)
{
   // Propagate stores these flags without quotes, may repeat them in earlier
   // groups, and includes STM automatically with Covariance. Compare the
   // command-wide flags while retaining each ordered propagator/object group.
   static const QRegularExpression prefix(R"(^\s*Propagate\s+(?:'[^'\n]*'\s+)?(?:BackProp\s+)?(?:Synchronized\s+)?)");
   const auto begin=prefix.match(code); if (!begin.hasMatch()) return key(code);
   static const QRegularExpression group(R"(^([A-Za-z][A-Za-z0-9_]*)\s*\(\s*((?:[A-Za-z][A-Za-z0-9_]*|'STM'|'AMatrix'|'Covariance')(?:\s*,\s*(?:[A-Za-z][A-Za-z0-9_]*|'STM'|'AMatrix'|'Covariance'))*)\s*\))");
   QString rest=code.mid(begin.capturedEnd()).trimmed(),header=begin.captured();
   QSet<QString> flags; int count=0;
   while (!rest.isEmpty() && rest.front()!='{' && rest.front()!=';') {
      const auto match=group.match(rest); if (!match.hasMatch()) return key(code);
      QStringList objects;
      for (auto token:match.captured(2).split(',')) {
         token=token.trimmed(); if (token.startsWith('\'')) token=token.mid(1,token.size()-2);
         if (token=="STM" || token=="AMatrix" || token=="Covariance") flags.insert(token);
         else objects.append(token);
      }
      if (objects.isEmpty()) return key(code);
      header+=match.captured(1)+"("+objects.join(",")+") "; ++count;
      rest=rest.mid(match.capturedLength()).trimmed();
   }
   if (!count) return key(code);
   if (flags.contains("Covariance")) flags.remove("STM");
   auto ordered=flags.values(); ordered.sort();
   return key(header+rest)+"|variational="+ordered.join(',');
}
QString pythonCallKey(const QString &code)
{
   // Python serializes a single output with brackets, and an empty input
   // list with parentheses. Both alternatives are accepted script syntax.
   // Retain exact labels, ordered outputs, module/function and argument text.
   static const QRegularExpression call(R"(^((?:'[^'\n]*')?)(?:(?:\[([A-Za-z][A-Za-z0-9_]*(?:,[A-Za-z][A-Za-z0-9_]*)*)\]|([A-Za-z][A-Za-z0-9_]*))=)?(Python\.[A-Za-z_][A-Za-z0-9_]*\.[A-Za-z_][A-Za-z0-9_]*)(\([\s\S]*\))?$)");
   const auto text=key(code); const auto match=call.match(text);
   if (!match.hasMatch()) return text;
   const auto outputs=match.captured(2).isEmpty() ? match.captured(3) : match.captured(2);
   return match.captured(1)+(outputs.isEmpty() ? QString() : "["+outputs+"]=")+match.captured(4)+(match.captured(5).isEmpty() ? "()" : match.captured(5));
}
QString gmatCallKey(const QString &code)
{
   // Normalize only a configured GMAT function's accepted call spelling.
   // Bare zero-input calls and scalar output syntax serialize differently;
   // labels, function identity and ordered arguments must still match.
   static const QRegularExpression call(R"(^((?:'[^'\n]*')?)(?:(?:\[([A-Za-z][A-Za-z0-9_]*(?:,[A-Za-z][A-Za-z0-9_]*)*)\]|([A-Za-z][A-Za-z0-9_]*))=)?([A-Za-z][A-Za-z0-9_]*)(\([\s\S]*\))?$)");
   const auto text=key(code); const auto match=call.match(text);
   if (!match.hasMatch()) return text;
   auto *moderator=Moderator::Instance();
   auto *function=moderator->IsInitialized() ? moderator->GetConfiguredObject(match.captured(4).toStdString()) : nullptr;
   if (!function || !function->IsOfType("GmatFunction")) return text;
   const auto outputs=match.captured(2).isEmpty() ? match.captured(3) : match.captured(2);
   return match.captured(1)+(outputs.isEmpty() ? QString() : "["+outputs+"]=")+match.captured(4)+(match.captured(5).isEmpty() ? "()" : match.captured(5));
}
bool sameStatement(const QString &generated,const QString &original)
{
   auto a=key(generated),b=key(original);
   if (a==b) return true;
   if (gmatCallKey(generated)==gmatCallKey(original)) return true;
   if (a.contains("Python.") && pythonCallKey(generated)==pythonCallKey(original)) return true;
   if (generated.trimmed().startsWith("Propagate ") && propagationKey(generated)==propagationKey(original)) return true;
   // For writes its implicit unit step even when the source uses start:end.
   static const QRegularExpression implicitStep(R"(^(For(?:'[^']*')?[A-Za-z][A-Za-z0-9_]*=[^:]+):([^:]+)$)");
   const auto loop=implicitStep.match(b);
   if (loop.hasMatch() && a==loop.captured(1)+":1:"+loop.captured(2)) return true;
   // These serializers explicitly fill in omitted option dictionaries. Their
   // accepted header/operands identify the same command; retain the source's
   // dictionary and implicit defaults in its editor instead of writing them.
   const auto type=generated.trimmed().section(QRegularExpression("[\\s'(]"),0,0);
   const QSet<QString> dictionaries={"Target","Optimize","Vary","Achieve","FindEvents","Minimize","NonlinearConstraint","Write"};
   if (!dictionaries.contains(type)) return false;
   static const QRegularExpression options(R"(,?\{[^{}]*\})");
   a.remove(options); b.remove(options); return a==b;
}
qsizetype codeStart(const ScriptStatement &statement)
{
   for (qsizetype i=0;i<statement.code.size();++i) if (!statement.code[i].isSpace()) return statement.positions[i];
   return -1;
}
qsizetype codeEnd(const ScriptStatement &statement)
{
   for (qsizetype i=statement.code.size()-1;i>=0;--i) if (!statement.code[i].isSpace()) return statement.positions[i]+1;
   return -1;
}
void retainSource(MissionSnapshot &snapshot)
{
   const auto original=scriptStatements(snapshot.sourceScript);
   auto generated=scriptStatements(snapshot.canonicalScript);
   // The serializer may repeat configuration include directives in its mission
   // tail. These are not executable commands. Leave original source directives
   // in the alignment: commands expanded from a mission include still cannot
   // be mistaken for text owned by this editor.
   for (qsizetype i=generated.size();i-->0;) if (generated[i].code.trimmed().startsWith("#Include")) generated.removeAt(i);
   qsizetype canonicalBegin=-1,sourceBegin=-1;
   for (qsizetype i=0;i<generated.size();++i) if (key(generated[i].code)=="BeginMissionSequence") { canonicalBegin=i; break; }
   const bool canonicalBoundary=canonicalBegin>=0;
   if (!canonicalBoundary) {
      for (const auto &node:snapshot.nodes) if (node.parent<0 && node.start>=0) {
         for (qsizetype i=0;i<generated.size();++i) if (codeStart(generated[i])>=node.start && codeEnd(generated[i])<=node.end) { canonicalBegin=i; break; }
         if (canonicalBegin>=0) break;
      }
   }
   for (qsizetype i=0;i<original.size();++i) if (key(original[i].code)=="BeginMissionSequence") { sourceBegin=i; break; }
   // The engine inserts BeginMissionSequence for an implicit mission. Locate
   // its first actual command, rather than treating configuration assignments
   // as executable source.
   bool implicit=false;
   if (sourceBegin<0 && canonicalBegin>=0 && canonicalBegin+(canonicalBoundary ? 1 : 0)<generated.size()) {
      implicit=true;
      for (qsizetype i=0;i<original.size();++i) if (sameStatement(generated[canonicalBegin+(canonicalBoundary ? 1 : 0)].code,original[i].code)) { sourceBegin=i; break; }
   }
   else if (sourceBegin>=0 && !canonicalBoundary) ++sourceBegin;
   QMap<qsizetype,qsizetype> mapping;
   bool matched=canonicalBegin>=0 && sourceBegin>=0;
   auto sourceIndex=sourceBegin;
   if (matched) for (auto i=canonicalBegin+(implicit && canonicalBoundary ? 1 : 0);i<generated.size();++i,++sourceIndex) {
      if (sourceIndex>=original.size() || !sameStatement(generated[i].code,original[sourceIndex].code)) { matched=false; break; }
      mapping[i]=sourceIndex;
   }
   if (sourceIndex!=original.size()) matched=false;
   // Includes or engine rewrites that cannot be aligned exactly must remain a
   // text edit. Never fall back to regenerating unrelated configurations.
   for (auto &node:snapshot.nodes) {
      const auto canonicalStart=node.start,canonicalEnd=node.end;
      node.start=-1; node.end=-1; node.editable=false;
      if (!matched || canonicalStart<0) continue;
      qsizetype first=-1,last=-1;
      for (auto i=canonicalBegin;i<generated.size();++i) {
         if (codeStart(generated[i])>=canonicalStart && codeEnd(generated[i])<=canonicalEnd && mapping.contains(i)) {
            if (first<0) first=mapping.value(i);
            last=mapping.value(i);
         }
      }
      if (first<0 || last<first) continue;
      auto begin=codeStart(original[first]),end=codeEnd(original[last]);
      const auto line=snapshot.sourceScript.lastIndexOf('\n',begin-1)+1;
      if (snapshot.sourceScript.mid(line,begin-line).trimmed().isEmpty()) begin=line;
      // Keep the engine-associated leading comments, with their original text.
      const auto generatedLines=node.statement.split('\n');
      int comments=0;
      for (const auto &line:generatedLines) {
         if (line.trimmed().startsWith('%')) ++comments;
         else if (!line.trimmed().isEmpty()) break;
      }
      while (comments>0 && begin>0) {
         const auto previousLine=snapshot.sourceScript.lastIndexOf('\n',begin-2)+1;
         const auto text=snapshot.sourceScript.mid(previousLine,begin-previousLine).trimmed();
         if (!text.isEmpty() && !text.startsWith('%')) break;
         begin=previousLine; if (text.startsWith('%')) --comments;
      }
      const auto newline=snapshot.sourceScript.indexOf('\n',end);
      const auto lineEnd=newline<0 ? snapshot.sourceScript.size() : newline+1;
      const auto tail=snapshot.sourceScript.mid(end,lineEnd-end).trimmed();
      if (tail.isEmpty() || tail.startsWith('%')) end=lineEnd;
      node.start=begin; node.end=end; node.statement=snapshot.sourceScript.mid(begin,end-begin).trimmed();
      const QSet<QString> structural={"BeginMissionSequence","Else","EndIf","EndFor","EndWhile","EndTarget","EndOptimize","EndScript"};
      node.editable=!structural.contains(node.type);
   }
}
}
MissionSnapshot snapshotMission(GmatCommand *first,const QString &canonical,const QString &source,QVector<GmatCommand *> *commands,bool expandScriptEvents)
{
   Builder builder; builder.result.sourceScript=source; builder.result.canonicalScript=canonical;
   builder.expandScriptEvents=expandScriptEvents;
   builder.document=lines(canonical); builder.collect(first,nullptr,-1,0,canonical.size());
   retainSource(builder.result);
   if (commands) *commands=builder.commands;
   return builder.result;
}
QString editMission(const MissionSnapshot &snapshot,int index,MissionEdit operation,const QString &replacement)
{
   if (operation!=MissionEdit::Remove && replacement.trimmed().isEmpty()) throw std::runtime_error("Enter a command");
   QString candidate=snapshot.sourceScript;
   if (operation==MissionEdit::Append) {
      if (!candidate.endsWith('\n')) candidate+='\n';
      return candidate+replacement.trimmed()+"\n";
   }
   if (index<0 || index>=snapshot.nodes.size()) throw std::runtime_error("Select a mission command");
   const auto &node=snapshot.nodes[index];
   if (node.start<0 || node.end<=node.start) throw std::runtime_error("This command cannot be located safely; use the script editor");
   if ((operation==MissionEdit::Replace || operation==MissionEdit::Remove) && !node.editable)
      throw std::runtime_error("Edit the enclosing branch to change this structural command");
   if (operation==MissionEdit::InsertBefore && node.type=="BeginMissionSequence")
      throw std::runtime_error("Insert commands after BeginMissionSequence");
   if (operation==MissionEdit::Remove) candidate.remove(node.start,node.end-node.start);
   else if (operation==MissionEdit::Replace) candidate.replace(node.start,node.end-node.start,replacement.trimmed()+"\n");
   else if (operation==MissionEdit::InsertBefore) candidate.insert(node.start,replacement.trimmed()+"\n");
   else if (operation==MissionEdit::InsertAfter) candidate.insert(node.end,replacement.trimmed()+"\n");
   return candidate;
}
