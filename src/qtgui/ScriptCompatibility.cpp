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
      const auto match=QRegularExpression("^\\{\\s*([A-Za-z][A-Za-z0-9_]*(?:\\s*,\\s*[A-Za-z][A-Za-z0-9_]*)*)\\s*\\}$").match(views);
      if (!match.hasMatch()) {
         if (!views.isEmpty()) { result.error=plot+": invalid camera view list."; return result; }
         result.notes.append(plot+": no supported view selection; review the default OrbitView camera."); continue;
      }
      const auto viewNames=match.captured(1).split(QRegularExpression("\\s*,\\s*"));
      QSet<QString> seen;
      for (const auto &viewName:viewNames) {
         if (types.value(viewName)!="OpenFramesView" || seen.contains(viewName)) {
            result.error=plot+": unknown or duplicate camera view "+viewName+"."; return result;
         }
         seen.insert(viewName);
      }
      const auto view=viewNames.first(); const auto values=properties.value(view);
      bool validFov=false;
      const double fov=values.value("FOVy","45").toDouble(&validFov);
      if (!validFov || !std::isfinite(fov) || fov<1 || fov>150) {
         result.error=plot+": field of view must be a finite number from 1 to 150 degrees for Qt conversion."; return result;
      }
      QtCameraSetting cameraSetting{true,fov};
      const auto frame=values.value("ViewFrame","CoordinateSystem");
      const bool body=frame!="CoordinateSystem";
      cameraSetting.bodyRelative=body && values.value("InertialFrame","Off")=="Off" && values.value("ViewTrajectory","Off")=="Off";
      auto set=[&](const QString &key,const QString &value) { cameras.append("GMAT "+plot+"."+key+" = "+value+";"); };
      set("ViewPointReference",body ? frame : "[0 0 0]");
      set("ViewDirection",body ? frame : "[0 0 0]");
      const auto prefix=values.value("SetCurrentLocation")=="On" ? QString("Current") : QString("Default");
      const bool stored=values.value("Set"+prefix+"Location")=="On";
      std::array<double,3> eye{0,-1,0},center{},up{0,0,1};
      if (stored) for (auto component:{qMakePair(QString("Eye"),&eye),qMakePair(QString("Center"),&center),qMakePair(QString("Up"),&up)}) {
         if (values.contains(prefix+component.first) && !vectorValue(values.value(prefix+component.first),*component.second)) {
            result.error=plot+": invalid stored camera "+component.first+" vector."; return result;
         }
      }
      if (stored) set("ViewPointVector",formatVector(eye));
      else result.notes.append(plot+": default camera distance used; review Script view or Fit.");
      if (stored) {
         if (!body) set("ViewDirection",formatVector(center));
         else cameraSetting.centerOffset=center;
      }
      if (values.contains("LookAtFrame")) set("ViewDirection",values["LookAtFrame"]);
      if (stored) {
         int axis=0; for (int i=1;i<3;++i) if (std::abs(up[i])>std::abs(up[axis])) axis=i;
         QString signedAxis=up[axis]<0 ? "-" : ""; signedAxis+="XYZ"[axis]; set("ViewUpAxis",signedAxis);
         if (!std::isfinite(std::hypot(up[0],up[1],up[2])) || std::hypot(up[0],up[1],up[2])<1e-12) {
            result.error=plot+": camera up vector must be nonzero."; return result;
         }
         cameraSetting.up=up;
         result.notes.append(plot+": exact camera up vector retained in Qt metadata; the base viewer uses "+signedAxis+".");
      }
      cameraSetting.primaryName=view;
      for (int index=1;index<viewNames.size();++index) {
         QtCameraPreset preset; preset.name=viewNames[index];
         const auto extra=properties.value(preset.name);
         bool valid=false; preset.fieldOfView=extra.value("FOVy","45").toDouble(&valid);
         if (!valid || !std::isfinite(preset.fieldOfView) || preset.fieldOfView<1 || preset.fieldOfView>150) {
            result.error=preset.name+": field of view must be a finite number from 1 to 150 degrees."; return result;
         }
         const auto extraFrame=extra.value("ViewFrame","CoordinateSystem");
         if (extraFrame!="CoordinateSystem") preset.reference=extraFrame;
         preset.target=extra.value("LookAtFrame");
         preset.bodyRelative=!preset.reference.isEmpty() && extra.value("InertialFrame","Off")=="Off" && extra.value("ViewTrajectory","Off")=="Off";
         const auto location=extra.value("SetCurrentLocation")=="On" ? QString("Current") : QString("Default");
         if (extra.value("Set"+location+"Location")=="On") {
            // OF defaults for omitted stored components.
            preset.eye={0,-1,0};
            for (auto component:{qMakePair(QString("Eye"),&preset.eye),qMakePair(QString("Center"),&preset.center),qMakePair(QString("Up"),&preset.up)}) {
               if (extra.contains(location+component.first) && !vectorValue(extra.value(location+component.first),*component.second)) {
                  result.error=preset.name+": invalid stored camera "+component.first+" vector."; return result;
               }
            }
         } else result.notes.append(preset.name+": automatic OF distance is replaced by 30000 km; use Fit or zoom to adjust.");
         cameraSetting.views.append(preset);
         if (!preset.reference.isEmpty()) result.notes.append(preset.name+(preset.bodyRelative ? ": camera follows the object position and orientation." : ": camera follows the object position using plot-frame axes."));
         if (!preset.target.isEmpty()) result.notes.append(preset.name+": look-at target is tracked directly; OF ShortestAngle/AZEL orientation is not imported.");
         if (extra.value("ViewTrajectory")=="On") result.notes.append(preset.name+": trajectory-relative orientation is not imported.");
      }
      try { qtCameraSettings(qtCameraDirective(plot,cameraSetting)); }
      catch (const std::exception &error) { result.error=QString::fromUtf8(error.what()); return result; }
      cameras.append(qtCameraDirective(plot,cameraSetting).trimmed());
      set("ViewUpCoordinateSystem",properties[plot].value("CoordinateSystem","EarthMJ2000Eq"));
      result.notes.append(plot+": camera selector retains "+QString::number(viewNames.size())+" named views; trajectory-relative orientation is not imported.");
      if (body) result.notes.append(plot+(cameraSetting.bodyRelative ? ": camera follows the object position and orientation." : ": camera follows the object position using plot-frame axes."));
      if (values.contains("LookAtFrame")) result.notes.append(plot+": look-at target is tracked directly; OF ShortestAngle/AZEL orientation is not imported.");
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
   QJsonObject object{{"plot",plot},{"perspective",setting.perspective},{"fieldOfView",setting.fieldOfView}};
   if (setting.up) object.insert("up",QJsonArray{(*setting.up)[0],(*setting.up)[1],(*setting.up)[2]});
   if (setting.bodyRelative) object.insert("bodyRelative",true);
   if (setting.centerOffset) object.insert("centerOffset",QJsonArray{(*setting.centerOffset)[0],(*setting.centerOffset)[1],(*setting.centerOffset)[2]});
   if (!setting.primaryName.isEmpty()) object.insert("primaryName",setting.primaryName);
   if (!setting.views.isEmpty()) {
      QJsonArray views;
      auto vector=[](const std::array<double,3> &v) { return QJsonArray{v[0],v[1],v[2]}; };
      for (const auto &view:setting.views) views.append(QJsonObject{{"name",view.name},{"reference",view.reference},{"target",view.target},
         {"eye",vector(view.eye)},{"center",vector(view.center)},{"up",vector(view.up)},
         {"perspective",view.perspective},{"fieldOfView",view.fieldOfView},{"bodyRelative",view.bodyRelative}});
      object.insert("views",views);
   }
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
      QtCameraSetting setting{object.value("perspective").toBool(),fov};
      if (object.contains("up")) {
         const auto array=object.value("up").toArray();
         std::array<double,3> up{};
         bool valid=array.size()==3;
         for (int i=0;valid && i<3;++i) {
            up[i]=array[i].toDouble(); valid=array[i].isDouble() && std::isfinite(up[i]);
         }
         if (!valid || !std::isfinite(std::hypot(up[0],up[1],up[2])) || std::hypot(up[0],up[1],up[2])<1e-12)
            throw std::runtime_error("Invalid GMAT-Qt-Camera up vector: expected three finite numbers and a nonzero length");
         setting.up=up;
      }
      if (object.contains("bodyRelative") && !object.value("bodyRelative").isBool()) throw std::runtime_error("Camera bodyRelative must be a boolean");
      setting.bodyRelative=object.value("bodyRelative").toBool();
      if (object.contains("centerOffset")) {
         const auto array=object.value("centerOffset").toArray(); std::array<double,3> center{};
         if (array.size()!=3) throw std::runtime_error("Camera centerOffset must have three numbers");
         for (int i=0;i<3;++i) {
            if (!array[i].isDouble() || !std::isfinite(array[i].toDouble())) throw std::runtime_error("Camera centerOffset must have finite numbers");
            center[i]=array[i].toDouble();
         }
         setting.centerOffset=center;
      }
      const QRegularExpression identifier("^[A-Za-z][A-Za-z0-9_]*$");
      if (object.contains("primaryName")) {
         setting.primaryName=object.value("primaryName").toString();
         if (!identifier.match(setting.primaryName).hasMatch()) throw std::runtime_error("Invalid primary camera name");
      }
      if (object.contains("views")) {
         if (!object.value("views").isArray()) throw std::runtime_error("Camera views must be an array");
         QSet<QString> names; names.insert(setting.primaryName);
         auto vector=[](const QJsonValue &value) {
            const auto array=value.toArray(); std::array<double,3> result{};
            if (array.size()!=3) throw std::runtime_error("Camera vector must have three numbers");
            for (int i=0;i<3;++i) {
               if (!array[i].isDouble() || !std::isfinite(array[i].toDouble())) throw std::runtime_error("Camera vector must have finite numbers");
               result[i]=array[i].toDouble();
            }
            return result;
         };
         for (const auto &item:object.value("views").toArray()) {
            const auto value=item.toObject(); QtCameraPreset view;
            view.name=value.value("name").toString(); view.reference=value.value("reference").toString(); view.target=value.value("target").toString();
            view.fieldOfView=value.value("fieldOfView").toDouble(-1); view.perspective=value.value("perspective").toBool();
            if (!identifier.match(view.name).hasMatch() || names.contains(view.name) ||
                !value.value("reference").isString() || (!view.reference.isEmpty() && !identifier.match(view.reference).hasMatch()) ||
                !value.value("target").isString() || (!view.target.isEmpty() && !identifier.match(view.target).hasMatch()) ||
                !value.value("perspective").isBool() || !std::isfinite(view.fieldOfView) || view.fieldOfView<1 || view.fieldOfView>150)
               throw std::runtime_error("Invalid or duplicate named camera: check names, reference objects, projection and field of view");
            if (value.contains("bodyRelative") && !value.value("bodyRelative").isBool()) throw std::runtime_error("Named camera bodyRelative must be a boolean");
            view.bodyRelative=value.value("bodyRelative").toBool();
            if (view.bodyRelative && (view.reference.isEmpty() || view.reference=="CoordinateSystem")) throw std::runtime_error("Body-relative camera needs an object reference");
            view.eye=vector(value.value("eye")); view.center=vector(value.value("center")); view.up=vector(value.value("up"));
            const auto upLength=std::hypot(view.up[0],view.up[1],view.up[2]);
            const auto distance=std::hypot(view.eye[0]-view.center[0],view.eye[1]-view.center[1],view.eye[2]-view.center[2]);
            if (!std::isfinite(upLength) || upLength<1e-12 || !std::isfinite(distance) || (view.target.isEmpty() && distance<1e-9))
               throw std::runtime_error("Named camera needs a nonzero up vector and distinct eye and center");
            names.insert(view.name); setting.views.append(view);
         }
      }
      result.insert(name,setting);
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
