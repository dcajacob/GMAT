#include "ScriptCompatibility.hpp"
#include <QMap>
#include <QSet>
#include <QRegularExpression>
#include <array>
#include <cmath>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <stdexcept>

namespace {
QString codePart(const QString &line)
{
   bool quoted=false;
   for (int i=0;i<line.size();++i) {
      if (line[i]=='\'') {
         if (quoted && i+1<line.size() && line[i+1]=='\'') { ++i; continue; }
         quoted=!quoted;
      } else if (line[i]=='%' && !quoted) return line.left(i).trimmed();
   }
   return line.trimmed();
}
bool vectorValue(QString value,std::array<double,3> &result)
{
   if (!value.startsWith('[') || !value.endsWith(']')) return false;
   const auto fields=value.mid(1,value.size()-2).trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
   if (fields.size()!=3) return false;
   for (int i=0;i<3;++i) { bool ok=false; result[i]=fields[i].toDouble(&ok); if (!ok || !std::isfinite(result[i])) return false; }
   return true;
}
QString formatVector(const std::array<double,3> &value)
{
   return QString("[%1 %2 %3]").arg(QString::number(value[0],'g',17),QString::number(value[1],'g',17),QString::number(value[2],'g',17));
}
}

QtScriptConversion convertOpenFramesViews(const QString &source)
{
   QtScriptConversion result; result.script=source;
   const auto lines=source.split('\n');
   const QRegularExpression declaration("^Create\\s+(OpenFramesInterface|OpenFramesView)\\s+([A-Za-z][A-Za-z0-9_]*)\\s*;?$");
   QMap<QString,QString> types;
   QMap<QString,QMap<QString,QString>> properties;
   for (int i=0;i<lines.size();++i) {
      const auto code=codePart(lines[i]);
      if (!QRegularExpression("^Create\\s+OpenFrames").match(code).hasMatch()) continue;
      const auto match=declaration.match(code);
      if (!match.hasMatch() || types.contains(match.captured(2))) {
         result.error=QString("Line %1: unsupported or duplicate OpenFrames declaration. Convert this definition manually.").arg(i+1); return result;
      }
      types[match.captured(2)]=match.captured(1);
   }
   if (types.isEmpty()) return result;
   const QRegularExpression assignment("^(?:GMAT\\s+)?([A-Za-z][A-Za-z0-9_]*)\\.([A-Za-z][A-Za-z0-9_]*)\\s*=\\s*(.+?)\\s*;?$");
   QMap<int,QPair<QString,QString>> settings;
   bool mission=false;
   for (int i=0;i<lines.size();++i) {
      auto code=codePart(lines[i]);
      if (code.startsWith("BeginMissionSequence")) mission=true;
      if (declaration.match(code).hasMatch()) {
         if (mission) { result.error="OpenFrames declarations inside the mission require manual conversion."; return result; }
         continue;
      }
      const auto match=assignment.match(code);
      if (match.hasMatch() && types.contains(match.captured(1))) {
         if (mission || match.captured(3).contains(';')) {
            result.error=QString("Line %1: dynamic or compound OpenFrames settings require manual conversion.").arg(i+1); return result;
         }
         auto value=match.captured(3).trimmed();
         properties[match.captured(1)][match.captured(2)]=value;
         settings[i]={match.captured(1),match.captured(2)};
      } else {
         // Never remove a view object that is used by a calculation, command or
         // unrecognized continuation. Subscriber Toggle commands remain valid.
         for (auto it=types.cbegin();it!=types.cend();++it) {
            const QRegularExpression reference("\\b"+QRegularExpression::escape(it.key())+"\\b");
            if (reference.match(code).hasMatch() && it.value()=="OpenFramesView") {
               result.error=QString("Line %1: view %2 is referenced outside a supported viewer setting.").arg(i+1).arg(it.key()); return result;
            }
         }
      }
   }
   const QSet<QString> common={"Add","CoordinateSystem","DrawObject","SolverIterations","Maximized","ShowPlot","Size","UpperLeft",
      "RelativeZOrder","Axes","XYPlane","EclipticPlane","EnableStars","StarCount","EnableConstellations",
      "DataCollectFrequency","UpdatePlotFrequency","MaxPlotPoints"};
   QStringList output;
   for (int i=0;i<lines.size();++i) {
      const auto declared=declaration.match(codePart(lines[i]));
      if (declared.hasMatch()) {
         if (declared.captured(1)=="OpenFramesInterface") {
            output.append("Create OrbitView "+declared.captured(2)+"; % Converted from OpenFramesInterface for Qt"); ++result.plots;
         } else output.append("% Qt conversion: "+lines[i]);
         continue;
      }
      if (!settings.contains(i)) { output.append(lines[i]); continue; }
      const auto name=settings[i].first,key=settings[i].second;
      if (types[name]=="OpenFramesInterface" && common.contains(key)) output.append(lines[i]);
      else if (types[name]=="OpenFramesInterface" && (key=="DrawLabel" || key=="DrawGrid")) {
         const auto value=properties[name][key];
         const bool anyTrue=QRegularExpression("\\btrue\\b",QRegularExpression::CaseInsensitiveOption).match(value).hasMatch();
         output.append(QString("GMAT %1.%2 = %3; %% Qt conversion of %4").arg(name,key=="DrawLabel" ? "ShowLabels" : "Grid",anyTrue ? "On" : "Off",key));
         result.notes.append(name+"."+key+": per-object flags combined into one plot setting.");
      } else {
         output.append("% Qt conversion: "+lines[i]);
         if (types[name]=="OpenFramesInterface" && key!="View") result.notes.append(name+"."+key+": retained as a comment; no direct Qt equivalent.");
      }
   }
   QStringList cameras;
   for (auto it=types.cbegin();it!=types.cend();++it) if (it.value()=="OpenFramesInterface") {
      const auto plot=it.key();
      const auto views=properties[plot].value("View");
      const auto match=QRegularExpression("^\\{\\s*([A-Za-z][A-Za-z0-9_]*)(?:\\s*,[^{}]*)?\\s*\\}$").match(views);
      if (!match.hasMatch() || types.value(match.captured(1))!="OpenFramesView") {
         result.notes.append(plot+": no supported view selection; review the default OrbitView camera."); continue;
      }
      const auto view=match.captured(1); const auto values=properties.value(view);
      bool validFov=false;
      const double fov=values.value("FOVy","45").toDouble(&validFov);
      if (!validFov || !std::isfinite(fov) || fov<1 || fov>150) {
         result.error=plot+": field of view must be a finite number from 1 to 150 degrees for Qt conversion."; return result;
      }
      cameras.append(qtCameraDirective(plot,{true,fov}).trimmed());
      const auto frame=values.value("ViewFrame","CoordinateSystem");
      const bool body=frame!="CoordinateSystem";
      auto set=[&](const QString &key,const QString &value) { cameras.append("GMAT "+plot+"."+key+" = "+value+";"); };
      set("ViewPointReference",body ? frame : "[0 0 0]");
      set("ViewDirection",body ? frame : "[0 0 0]");
      const auto prefix=values.value("SetCurrentLocation")=="On" ? QString("Current") : QString("Default");
      const bool stored=values.value("Set"+prefix+"Location")=="On";
      std::array<double,3> eye,center,up;
      if (stored && vectorValue(values.value(prefix+"Eye"),eye)) set("ViewPointVector",formatVector(eye));
      else result.notes.append(plot+": default camera distance used; review Script view or Fit.");
      if (stored && vectorValue(values.value(prefix+"Center"),center)) {
         if (!body) set("ViewDirection",formatVector(center));
         else if (center!=std::array<double,3>{0,0,0}) result.notes.append(plot+": body-relative center offset needs manual adjustment.");
      }
      if (values.contains("LookAtFrame")) set("ViewDirection",values["LookAtFrame"]);
      if (stored && vectorValue(values.value(prefix+"Up"),up)) {
         int axis=0; for (int i=1;i<3;++i) if (std::abs(up[i])>std::abs(up[axis])) axis=i;
         QString signedAxis=up[axis]<0 ? "-" : ""; signedAxis+="XYZ"[axis]; set("ViewUpAxis",signedAxis);
         int nonzero=0; for (double component:up) if (std::abs(component)>1e-12) ++nonzero;
         if (nonzero!=1) result.notes.append(plot+": non-axis up vector approximated by "+signedAxis+"; review camera roll.");
      }
      set("ViewUpCoordinateSystem",properties[plot].value("CoordinateSystem","EarthMJ2000Eq"));
      result.notes.append(plot+": uses first view "+view+" with perspective and its vertical field of view; OFI view switching and trajectory-relative orientation are not imported.");
      if (body) result.notes.append(plot+": object tracking uses plot-frame axes; body-relative view rotation is not imported.");
   }
   int insertion=output.size();
   for (int i=0;i<output.size();++i) if (codePart(output[i]).startsWith("BeginMissionSequence")) { insertion=i; break; }
   for (int i=cameras.size()-1;i>=0;--i) output.insert(insertion,cameras[i]);
   result.notes.removeDuplicates();
   QStringList preamble={"% OpenFrames viewer conversion for Qt. Review these visual differences before saving:"};
   for (const auto &note:result.notes) preamble.append("% "+note);
   result.script=(preamble+output).join('\n');
   return result;
}

QString qtCameraDirective(const QString &plot,const QtCameraSetting &setting)
{
   const QJsonObject object{{"plot",plot},{"perspective",setting.perspective},{"fieldOfView",setting.fieldOfView}};
   return "% GMAT-Qt-Camera "+QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact))+"\n";
}
QMap<QString,QtCameraSetting> qtCameraSettings(const QString &source)
{
   QMap<QString,QtCameraSetting> result;
   for (const auto &line:source.split('\n')) {
      const auto text=line.trimmed(); const QString prefix="% GMAT-Qt-Camera ";
      if (!text.startsWith(prefix)) continue;
      QJsonParseError error;
      const auto document=QJsonDocument::fromJson(text.mid(prefix.size()).toUtf8(),&error);
      const auto object=document.object(); const auto name=object.value("plot").toString();
      const auto fov=object.value("fieldOfView").toDouble(-1);
      if (error.error!=QJsonParseError::NoError || !document.isObject() ||
          !QRegularExpression("^[A-Za-z][A-Za-z0-9_]*$").match(name).hasMatch() ||
          !object.value("perspective").isBool() || !std::isfinite(fov) || fov<1 || fov>150 || result.contains(name))
         throw std::runtime_error("Invalid or duplicate GMAT-Qt-Camera comment: expected a plot name, perspective boolean and fieldOfView from 1 to 150 degrees");
      result.insert(name,{object.value("perspective").toBool(),fov});
   }
   return result;
}
QString retainQtCameraSettings(const QString &original,const QString &candidate)
{
   const auto before=qtCameraSettings(original),after=qtCameraSettings(candidate);
   QString result=candidate;
   for (auto it=before.cbegin();it!=before.cend();++it)
      if (!after.contains(it.key())) result.prepend(qtCameraDirective(it.key(),it.value()));
   return result;
}

QString setQtCameraSetting(const QString &source,const QString &plot,const QtCameraSetting &setting)
{
   qtCameraSettings(source); // Reject ambiguity rather than overwriting malformed settings.
   const auto directive=qtCameraDirective(plot,setting);
   qtCameraSettings(directive);
   const QRegularExpression line("^[ \t]*% GMAT-Qt-Camera [^\n]*(?:\n|$)",QRegularExpression::MultilineOption);
   auto matches=line.globalMatch(source);
   while (matches.hasNext()) {
      const auto match=matches.next();
      if (qtCameraSettings(match.captured()).contains(plot)) {
         auto result=source; result.replace(match.capturedStart(),match.capturedLength(),directive); return result;
      }
   }
   return directive+source;
}

namespace {
QPair<qsizetype,qsizetype> arrayBlock(const QString &source,const QString &name)
{
   const QString prefix="^[ \t]*% GMAT-Qt-Array-Expressions "+QRegularExpression::escape(name);
   const QRegularExpression beginPattern(prefix+" begin[ \t]*(?:\n|$)",QRegularExpression::MultilineOption);
   const QRegularExpression endPattern(prefix+" end[ \t]*(?:\n|$)",QRegularExpression::MultilineOption);
   const auto begin=beginPattern.match(source),end=endPattern.match(source);
   if (!begin.hasMatch() && !end.hasMatch()) return {-1,0};
   if (!begin.hasMatch() || !end.hasMatch() || end.capturedStart()<begin.capturedEnd() ||
       beginPattern.match(source,begin.capturedEnd()).hasMatch() || endPattern.match(source,end.capturedEnd()).hasMatch())
      throw std::runtime_error("Array expression block is incomplete or duplicated; repair its begin/end comments before editing");
   return {begin.capturedStart(),end.capturedEnd()-begin.capturedStart()};
}
}
QString arrayExpressions(const QString &source,const QString &name)
{
   const auto block=arrayBlock(source,name); QJsonArray cells;
   if (block.first<0) return "[]";
   const auto lines=source.mid(block.first,block.second).split('\n');
   const QRegularExpression assignment("^\\s*(?:GMAT\\s+)?"+QRegularExpression::escape(name)+"\\(\\s*([0-9]+)\\s*,\\s*([0-9]+)\\s*\\)\\s*=\\s*([^;\\n]+);\\s*$");
   for (int i=1;i<lines.size();++i) {
      if (lines[i].trimmed()=="% GMAT-Qt-Array-Expressions "+name+" end") break;
      const auto match=assignment.match(lines[i]);
      if (!match.hasMatch()) throw std::runtime_error("Array expression block contains an unsupported statement; edit it in the script");
      cells.append(QJsonObject{{"row",match.captured(1).toInt()},{"column",match.captured(2).toInt()},{"expression",match.captured(3).trimmed()}});
   }
   return QString::fromUtf8(QJsonDocument(cells).toJson(QJsonDocument::Compact));
}
QString setArrayExpressions(const QString &source,const QString &name,const QString &cells,int rows,int columns)
{
   const auto block=arrayBlock(source,name);
   if (block.first>=0) arrayExpressions(source,name); // Never erase unrecognized commands.
   QJsonParseError error; const auto document=QJsonDocument::fromJson(cells.toUtf8(),&error);
   if (error.error!=QJsonParseError::NoError || !document.isArray()) throw std::runtime_error("Invalid array expression grid");
   QMap<QPair<int,int>,QString> expressions;
   for (const auto &value:document.array()) {
      const auto cell=value.toObject(); const auto row=cell.value("row").toInt(-1),col=cell.value("column").toInt(-1);
      const auto expression=cell.value("expression").toString().trimmed();
      if (row<1 || row>rows || col<1 || col>columns || expression.isEmpty() ||
          expression.contains(QRegularExpression("[;\r\n%]")) || expressions.contains({row,col}))
         throw std::runtime_error("Each expression needs a unique in-range cell and a single formula without semicolons or comments");
      expressions.insert({row,col},expression);
   }
   QString replacement;
   if (!expressions.isEmpty()) {
      replacement="% GMAT-Qt-Array-Expressions "+name+" begin\n";
      for (auto it=expressions.cbegin();it!=expressions.cend();++it)
         replacement+=QString("GMAT %1(%2,%3) = %4;\n").arg(name).arg(it.key().first).arg(it.key().second).arg(it.value());
      replacement+="% GMAT-Qt-Array-Expressions "+name+" end\n";
   }
   auto result=source;
   if (block.first>=0) { result.replace(block.first,block.second,replacement); return result; }
   if (replacement.isEmpty()) return result;
   const auto mission=QRegularExpression("^[ \t]*BeginMissionSequence[ \t]*;?[^\n]*(?:\n|$)",QRegularExpression::MultilineOption).match(source);
   if (!mission.hasMatch()) throw std::runtime_error("Build a mission sequence before adding array expressions");
   const auto position=mission.capturedEnd();
   result.insert(position,(position && source[position-1]!='\n' ? "\n" : "")+replacement);
   return result;
}
