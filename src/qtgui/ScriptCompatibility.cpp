#include "ScriptCompatibility.hpp"
#include <QMap>
#include <QSet>
#include <QRegularExpression>
#include <array>
#include <cmath>
#include <limits>
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
QMap<QString,bool> objectFlags(const QJsonValue &value)
{
   if (!value.isObject()) throw std::runtime_error("Per-object display flags must be objects of named boolean values");
   const auto values=value.toObject(); QMap<QString,bool> result;
   const QRegularExpression identifier("^[A-Za-z][A-Za-z0-9_]*$");
   for (auto it=values.begin();it!=values.end();++it) {
      if (!identifier.match(it.key()).hasMatch() || !it.value().isBool()) throw std::runtime_error("Per-object display flags need valid names and booleans");
      result.insert(it.key(),it.value().toBool());
   }
   return result;
}
QJsonObject objectFlagsJson(const QMap<QString,bool> &values)
{
   QJsonObject result; for (auto it=values.cbegin();it!=values.cend();++it) result.insert(it.key(),it.value()); return result;
}
QMap<QString,quint32> objectSizes(const QJsonValue &value)
{
   if (!value.isObject()) throw std::runtime_error("Marker sizes must be objects of named nonnegative integers");
   QMap<QString,quint32> result; const auto values=value.toObject();
   const QRegularExpression identifier("^[A-Za-z][A-Za-z0-9_]*$");
   for (auto it=values.begin();it!=values.end();++it) {
      const double size=it.value().toDouble(-1);
      if (!identifier.match(it.key()).hasMatch() || !it.value().isDouble() || size<0 || size>4294967295.0 || std::floor(size)!=size)
         throw std::runtime_error("Marker sizes need valid names and unsigned integer pixel sizes");
      result.insert(it.key(),static_cast<quint32>(size));
   }
   return result;
}
QJsonObject objectSizesJson(const QMap<QString,quint32> &values)
{
   QJsonObject result; for (auto it=values.cbegin();it!=values.cend();++it) result.insert(it.key(),double(it.value())); return result;
}
QMap<QString,double> objectWidths(const QJsonValue &value)
{
   if (!value.isObject()) throw std::runtime_error("Line widths must be objects of named finite values >= 1");
   QMap<QString,double> result; const auto values=value.toObject();
   const QRegularExpression identifier("^[A-Za-z][A-Za-z0-9_]*$");
   for (auto it=values.begin();it!=values.end();++it) {
      const double width=it.value().toDouble(-1);
      if (!identifier.match(it.key()).hasMatch() || !it.value().isDouble() || !std::isfinite(width) || width<1 || width>std::numeric_limits<float>::max())
         throw std::runtime_error("Line widths need valid names and finite pixel values >= 1 within the renderer's range");
      result.insert(it.key(),width);
   }
   return result;
}
QJsonObject objectWidthsJson(const QMap<QString,double> &values)
{
   QJsonObject result; for (auto it=values.cbegin();it!=values.cend();++it) result.insert(it.key(),it.value()); return result;
}
QString formatVector(const std::array<double,3> &value)
{
   return QString("[%1 %2 %3]").arg(QString::number(value[0],'g',17),QString::number(value[1],'g',17),QString::number(value[2],'g',17));
}
}

QMap<QString,bool> qtObjectFlags(const QString &json)
{
   QJsonParseError error; const auto document=QJsonDocument::fromJson(json.toUtf8(),&error);
   if (error.error!=QJsonParseError::NoError || !document.isObject()) throw std::runtime_error("Invalid per-object display flags: expected named boolean values");
   return objectFlags(document.object());
}
QString qtObjectFlagsJson(const QMap<QString,bool> &flags)
{
   return QString::fromUtf8(QJsonDocument(objectFlagsJson(flags)).toJson(QJsonDocument::Compact));
}

QMap<QString,quint32> qtObjectSizes(const QString &json)
{
   QJsonParseError error; const auto document=QJsonDocument::fromJson(json.toUtf8(),&error);
   if (error.error!=QJsonParseError::NoError || !document.isObject()) throw std::runtime_error("Invalid marker sizes: expected named unsigned integers");
   return objectSizes(document.object());
}
QString qtObjectSizesJson(const QMap<QString,quint32> &sizes)
{
   return QString::fromUtf8(QJsonDocument(objectSizesJson(sizes)).toJson(QJsonDocument::Compact));
}

QMap<QString,double> qtObjectWidths(const QString &json)
{
   QJsonParseError error; const auto document=QJsonDocument::fromJson(json.toUtf8(),&error);
   if (error.error!=QJsonParseError::NoError || !document.isObject()) throw std::runtime_error("Invalid line widths: expected named finite pixel values >= 1");
   return objectWidths(document.object());
}
QString qtObjectWidthsJson(const QMap<QString,double> &widths)
{
   return QString::fromUtf8(QJsonDocument(objectWidthsJson(widths)).toJson(QJsonDocument::Compact));
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
   for (auto it=types.cbegin();it!=types.cend();++it) if (it.value()=="OpenFramesView") {
      const auto values=properties.value(it.key());
      if (values.value("LookAtFrame").contains('.')) {
         result.error=it.key()+".LookAtFrame: segment-relative LookAt targets are not supported by the OpenFrames object lookup. The original script is unchanged."; return result;
      }
      if (values.value("ViewFrame").contains('.') &&
          !QRegularExpression("^[A-Za-z][A-Za-z0-9_]*\\.[A-Za-z][A-Za-z0-9_]*$").match(values.value("ViewFrame")).hasMatch()) {
         result.error=it.key()+".ViewFrame: a segment frame must be Object.NamedPropagate. The original script is unchanged."; return result;
      }
      for (const auto *key:{"SetDefaultLocation","SetCurrentLocation","InertialFrame","ViewTrajectory","ShortestAngle"})
         if (values.contains(key) && values.value(key)!="On" && values.value(key)!="Off") {
            result.error=it.key()+"."+key+": expected On or Off."; return result;
         }
   }
   QMap<QString,QtCameraSetting> displaySettings;
   for (auto it=types.cbegin();it!=types.cend();++it) if (it.value()=="OpenFramesInterface") {
      QStringList names; auto &display=displaySettings[it.key()];
      for (auto line=settings.cbegin();line!=settings.cend();++line) {
         if (line->first!=it.key()) continue;
         const auto property=line->second,value=assignment.match(codePart(lines[line.key()])).captured(3).trimmed();
         if (property=="Add") {
            const auto list=QRegularExpression("^\\{\\s*([A-Za-z][A-Za-z0-9_]*(?:[\\s,]+[A-Za-z][A-Za-z0-9_]*)*)?\\s*\\}$").match(value);
            if (!list.hasMatch()) { result.error=it.key()+": invalid Add list for per-object display settings."; return result; }
            names=list.captured(1).split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts); names.removeDuplicates();
            // OF Add resets the ordered object list and its display defaults.
            display.objectLabels.clear(); display.objectTrajectories.clear(); display.objectCenters.clear(); display.objectEndpoints.clear(); display.objectMarkerSizes.clear(); display.objectLineWidths.clear();
            for (const auto &name:names) { display.objectLabels[name]=true; display.objectTrajectories[name]=true; display.objectCenters[name]=true; display.objectEndpoints[name]=true; display.objectMarkerSizes[name]=10; display.objectLineWidths[name]=2; }
         } else if (property=="DrawLabel" || property=="DrawTrajectory" || property=="DrawCenterPoint" || property=="DrawEndPoints") {
            if (!value.startsWith('[') || !value.endsWith(']')) { result.error=it.key()+"."+property+": expected a bracketed true/false array."; return result; }
            const auto flags=value.mid(1,value.size()-2).trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
            auto &destination=property=="DrawLabel" ? display.objectLabels : property=="DrawTrajectory" ? display.objectTrajectories : property=="DrawCenterPoint" ? display.objectCenters : display.objectEndpoints;
            for (int i=0;i<flags.size();++i) {
               if (flags[i]!="true" && flags[i]!="false") { result.error=it.key()+"."+property+": expected true or false."; return result; }
               // OF applies the available prefix and leaves omitted objects at defaults.
               if (i<names.size()) destination[names[i]]=flags[i]=="true";
            }
         } else if (property=="DrawLineWidth") {
            if (!value.startsWith('[') || !value.endsWith(']')) { result.error=it.key()+".DrawLineWidth: expected a bracketed real array."; return result; }
            const auto widths=value.mid(1,value.size()-2).trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
            for (int i=0;i<widths.size();++i) {
               bool ok=false; const double width=widths[i].toDouble(&ok);
               if (!ok || !std::isfinite(width) || (i<names.size() && (width<1 || width>std::numeric_limits<float>::max()))) { result.error=it.key()+".DrawLineWidth: selected objects need finite widths >= 1 within the renderer's range."; return result; }
               // OF's bulk real setter validates/applies only the selected prefix.
               if (i<names.size()) display.objectLineWidths[names[i]]=width;
            }
         } else if (property=="DrawMarkerSize") {
            if (!value.startsWith('[') || !value.endsWith(']')) { result.error=it.key()+".DrawMarkerSize: expected a bracketed unsigned integer array."; return result; }
            const auto sizes=value.mid(1,value.size()-2).trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
            if (sizes.size()>names.size()) { result.error=it.key()+".DrawMarkerSize: a size index is outside the selected objects."; return result; }
            for (int i=0;i<sizes.size();++i) {
               bool ok=false; const auto size=sizes[i].toULongLong(&ok);
               if (!ok || size>4294967295ULL || !QRegularExpression("^[0-9]+$").match(sizes[i]).hasMatch()) { result.error=it.key()+".DrawMarkerSize: expected unsigned integer pixel sizes."; return result; }
               if (i<names.size()) display.objectMarkerSizes[names[i]]=static_cast<quint32>(size);
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
      else if (types[name]=="OpenFramesInterface" && (key=="DrawLabel" || key=="DrawTrajectory" || key=="DrawCenterPoint" || key=="DrawEndPoints" || key=="DrawMarkerSize" || key=="DrawLineWidth")) {
         output.append("% Qt conversion: "+lines[i]);
         result.notes.append(name+"."+key+": independent per-object drawing retained in Qt metadata.");
      } else if (types[name]=="OpenFramesInterface" && key=="DrawGrid") {
         const auto value=properties[name][key];
         const bool anyTrue=QRegularExpression("\\btrue\\b",QRegularExpression::CaseInsensitiveOption).match(value).hasMatch();
         output.append(QString("GMAT %1.Grid = %2; %% Qt conversion of DrawGrid").arg(name,anyTrue ? "On" : "Off"));
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
         result.notes.append(plot+": no supported view selection; review the default OrbitView camera.");
         cameras.append(qtCameraDirective(plot,displaySettings.value(plot)).trimmed()); continue;
      }
      const auto viewNames=match.captured(1).split(QRegularExpression("\\s*,\\s*"));
      QSet<QString> seen;
      for (const auto &viewName:viewNames) {
         if (types.value(viewName)!="OpenFramesView" || seen.contains(viewName)) {
            result.error=plot+": unknown or duplicate camera view "+viewName+"."; return result;
         }
         const auto viewSettings=properties.value(viewName);
         if (viewSettings.value("ViewTrajectory")=="On" && viewSettings.value("ViewFrame","CoordinateSystem")!="CoordinateSystem") {
            auto objects=properties[plot].value("Add"); objects.remove('{'); objects.remove('}');
            if (!objects.split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts).contains(viewSettings.value("ViewFrame").section('.',0,0))) {
               result.error=viewName+": trajectory frame must name an object in "+plot+".Add or CoordinateSystem."; return result;
            }
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
      cameraSetting.objectLabels=displaySettings.value(plot).objectLabels;
      cameraSetting.objectTrajectories=displaySettings.value(plot).objectTrajectories;
      cameraSetting.objectCenters=displaySettings.value(plot).objectCenters;
      cameraSetting.objectEndpoints=displaySettings.value(plot).objectEndpoints;
      cameraSetting.objectMarkerSizes=displaySettings.value(plot).objectMarkerSizes;
      cameraSetting.objectLineWidths=displaySettings.value(plot).objectLineWidths;
      const auto frame=values.value("ViewFrame","CoordinateSystem");
      const bool trajectory=values.value("ViewTrajectory")=="On";
      const bool segment=frame.contains('.');
      const auto frameObject=frame.section('.',0,0);
      const bool body=frame!="CoordinateSystem" && (!trajectory || segment);
      if (segment) cameraSetting.segmentFrame=frame;
      cameraSetting.bodyRelative=body && values.value("InertialFrame","Off")=="Off";
      auto set=[&](const QString &key,const QString &value) { cameras.append("GMAT "+plot+"."+key+" = "+value+";"); };
      set("ViewPointReference",body ? frameObject : "[0 0 0]");
      set("ViewDirection",body ? frameObject : "[0 0 0]");
      const auto prefix=values.value("SetCurrentLocation")=="On" ? QString("Current") : QString("Default");
      const bool stored=values.value("Set"+prefix+"Location")=="On";
      if (((!body && trajectory) || frame=="CoordinateSystem") && !stored) {
         cameraSetting.automaticTrajectory=frame;
         set("ViewPointVector","[0 -30000 0]"); cameraSetting.up=std::array<double,3>{0,0,1};
      }
      // OF returns the moving empty segment frame for either ViewTrajectory mode.
      if (body && !stored) {
         cameraSetting.automaticBody=frameObject;
         set("ViewPointVector","[0 -30000 0]"); cameraSetting.up=std::array<double,3>{0,0,1};
      }
      std::array<double,3> eye{0,-1,0},center{},up{0,0,1};
      if (stored) for (auto component:{qMakePair(QString("Eye"),&eye),qMakePair(QString("Center"),&center),qMakePair(QString("Up"),&up)}) {
         if (values.contains(prefix+component.first) && !vectorValue(values.value(prefix+component.first),*component.second)) {
            result.error=plot+": invalid stored camera "+component.first+" vector."; return result;
         }
      }
      if (stored) set("ViewPointVector",formatVector(eye));
      else if (!cameraSetting.automaticBody.isEmpty()) result.notes.append(plot+(segment ? ": automatic segment camera uses empty frame bounds." : ": automatic body camera uses the rendered body/model bounds."));
      if (stored) {
         if (!body) set("ViewDirection",formatVector(center));
         else cameraSetting.centerOffset=center;
      }
      if (values.contains("LookAtFrame")) {
         set("ViewDirection",values["LookAtFrame"]=="CoordinateSystem" ? "[0 0 0]" : values["LookAtFrame"]);
         cameraSetting.lookAtRotation=true; cameraSetting.shortestAngle=values.value("ShortestAngle","Off")=="On";
         cameraSetting.centerOffset=center;
      }
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
         const bool trajectory=extra.value("ViewTrajectory")=="On";
         const bool segment=extraFrame.contains('.');
         const auto extraObject=extraFrame.section('.',0,0);
         if (segment) preset.segmentFrame=extraFrame;
         if (extraFrame!="CoordinateSystem" && (!trajectory || segment)) preset.reference=extraObject;
         preset.target=extra.value("LookAtFrame");
         preset.lookAtRotation=!preset.target.isEmpty(); preset.shortestAngle=extra.value("ShortestAngle","Off")=="On";
         preset.bodyRelative=!preset.reference.isEmpty() && extra.value("InertialFrame","Off")=="Off";
         const auto location=extra.value("SetCurrentLocation")=="On" ? QString("Current") : QString("Default");
         if (extra.value("Set"+location+"Location")=="On") {
            // OF defaults for omitted stored components.
            preset.eye={0,-1,0};
            for (auto component:{qMakePair(QString("Eye"),&preset.eye),qMakePair(QString("Center"),&preset.center),qMakePair(QString("Up"),&preset.up)}) {
               if (extra.contains(location+component.first) && !vectorValue(extra.value(location+component.first),*component.second)) {
                  result.error=preset.name+": invalid stored camera "+component.first+" vector."; return result;
               }
            }
         } else if ((!segment && trajectory) || extraFrame=="CoordinateSystem") preset.automaticTrajectory=extraFrame;
         else { preset.automaticBody=extraObject; result.notes.append(preset.name+(segment ? ": automatic segment camera uses empty frame bounds." : ": automatic body camera uses the rendered body/model bounds.")); }
         cameraSetting.views.append(preset);
         if (segment) result.notes.append(preset.name+": segment camera follows the first retained named arc and clamps to its endpoint; automatic framing uses the empty segment-frame bounds.");
         if (preset.automaticTrajectory=="CoordinateSystem") result.notes.append(preset.name+": automatic origin framing retains the OpenFrames default radius and viewport-aware distance.");
         if (!preset.reference.isEmpty()) result.notes.append(preset.name+(preset.bodyRelative ? ": camera follows the object position and orientation." : ": camera follows the object position using plot-frame axes."));
         if (!preset.target.isEmpty()) result.notes.append(preset.name+": two-frame look-at orientation is retained, including ShortestAngle/AZEL rotation.");
         if (trajectory && !segment) result.notes.append(preset.name+(preset.automaticTrajectory.isEmpty() ? ": stored whole-trajectory camera retained in the plot frame; it does not follow the moving object." : ": automatic whole-trajectory camera uses retained trajectory bounds, refreshed as points arrive."));
      }
      if (!cameraSetting.segmentFrame.isEmpty()) result.notes.append(plot+": segment camera follows the first retained named arc and clamps to its endpoint; automatic framing uses the empty segment-frame bounds.");
      try { qtCameraSettings(qtCameraDirective(plot,cameraSetting)); }
      catch (const std::exception &error) { result.error=QString::fromUtf8(error.what()); return result; }
      cameras.append(qtCameraDirective(plot,cameraSetting).trimmed());
      if (cameraSetting.automaticTrajectory=="CoordinateSystem") result.notes.append(plot+": automatic origin framing retains the OpenFrames default radius and viewport-aware distance.");
      set("ViewUpCoordinateSystem",properties[plot].value("CoordinateSystem","EarthMJ2000Eq"));
      result.notes.append(plot+": camera selector retains "+QString::number(viewNames.size())+" named views.");
      if (trajectory && !segment) result.notes.append(plot+(stored ? ": stored whole-trajectory camera retained in the plot frame; it does not follow the moving object." : ": automatic whole-trajectory camera uses retained trajectory bounds, refreshed as points arrive."));
      if (body) result.notes.append(plot+(cameraSetting.bodyRelative ? ": camera follows the object position and orientation." : ": camera follows the object position using plot-frame axes."));
      if (values.contains("LookAtFrame")) result.notes.append(plot+": two-frame look-at orientation is retained, including ShortestAngle/AZEL rotation.");
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
   if (setting.lookAtRotation) { object.insert("lookAtRotation",true); object.insert("shortestAngle",setting.shortestAngle); }
   if (setting.centerOffset) object.insert("centerOffset",QJsonArray{(*setting.centerOffset)[0],(*setting.centerOffset)[1],(*setting.centerOffset)[2]});
   if (!setting.primaryName.isEmpty()) object.insert("primaryName",setting.primaryName);
   if (!setting.automaticTrajectory.isEmpty()) object.insert("automaticTrajectory",setting.automaticTrajectory);
   if (!setting.automaticBody.isEmpty()) object.insert("automaticBody",setting.automaticBody);
   if (!setting.segmentFrame.isEmpty()) object.insert("segmentFrame",setting.segmentFrame);
   if (!setting.objectLabels.isEmpty()) object.insert("objectLabels",objectFlagsJson(setting.objectLabels));
   if (!setting.objectTrajectories.isEmpty()) object.insert("objectTrajectories",objectFlagsJson(setting.objectTrajectories));
   if (!setting.objectCenters.isEmpty()) object.insert("objectCenters",objectFlagsJson(setting.objectCenters));
   if (!setting.objectEndpoints.isEmpty()) object.insert("objectEndpoints",objectFlagsJson(setting.objectEndpoints));
   if (!setting.objectMarkerSizes.isEmpty()) object.insert("objectMarkerSizes",objectSizesJson(setting.objectMarkerSizes));
   if (!setting.objectLineWidths.isEmpty()) object.insert("objectLineWidths",objectWidthsJson(setting.objectLineWidths));
   if (!setting.views.isEmpty()) {
      QJsonArray views;
      auto vector=[](const std::array<double,3> &v) { return QJsonArray{v[0],v[1],v[2]}; };
      for (const auto &view:setting.views) {
         QJsonObject named{{"name",view.name},{"reference",view.reference},{"target",view.target},
            {"eye",vector(view.eye)},{"center",vector(view.center)},{"up",vector(view.up)},{"automaticTrajectory",view.automaticTrajectory},{"automaticBody",view.automaticBody},
            {"perspective",view.perspective},{"fieldOfView",view.fieldOfView},{"bodyRelative",view.bodyRelative},{"lookAtRotation",view.lookAtRotation},{"shortestAngle",view.shortestAngle}};
         if (!view.segmentFrame.isEmpty()) named.insert("segmentFrame",view.segmentFrame);
         views.append(named);
      }
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
      for (const auto *key:{"lookAtRotation","shortestAngle"})
         if (object.contains(key) && !object.value(key).isBool()) throw std::runtime_error("Camera alignment modes must be booleans");
      setting.lookAtRotation=object.value("lookAtRotation").toBool(); setting.shortestAngle=object.value("shortestAngle").toBool();
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
      if (object.contains("objectLabels")) setting.objectLabels=objectFlags(object.value("objectLabels"));
      if (object.contains("objectTrajectories")) setting.objectTrajectories=objectFlags(object.value("objectTrajectories"));
      if (object.contains("objectCenters")) setting.objectCenters=objectFlags(object.value("objectCenters"));
      if (object.contains("objectEndpoints")) setting.objectEndpoints=objectFlags(object.value("objectEndpoints"));
      if (object.contains("objectMarkerSizes")) setting.objectMarkerSizes=objectSizes(object.value("objectMarkerSizes"));
      if (object.contains("objectLineWidths")) setting.objectLineWidths=objectWidths(object.value("objectLineWidths"));
      if (object.contains("segmentFrame")) {
         setting.segmentFrame=object.value("segmentFrame").toString();
         if (!QRegularExpression("^[A-Za-z][A-Za-z0-9_]*\\.[A-Za-z][A-Za-z0-9_]*$").match(setting.segmentFrame).hasMatch() || object.contains("automaticTrajectory"))
            throw std::runtime_error("Segment camera frame must be Object.NamedPropagate and cannot frame a whole trajectory");
      }
      if (object.contains("automaticBody")) {
         setting.automaticBody=object.value("automaticBody").toString();
         if (!identifier.match(setting.automaticBody).hasMatch() || setting.automaticBody=="CoordinateSystem" || object.contains("automaticTrajectory"))
            throw std::runtime_error("Automatic body camera needs one object name and cannot also frame a trajectory");
      }
      if (object.contains("automaticTrajectory")) {
         setting.automaticTrajectory=object.value("automaticTrajectory").toString();
         if (!identifier.match(setting.automaticTrajectory).hasMatch() || setting.bodyRelative)
            throw std::runtime_error("Automatic trajectory camera needs an object name without body-relative rotation");
      }
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
            view.automaticTrajectory=value.value("automaticTrajectory").toString();
            view.automaticBody=value.value("automaticBody").toString();
            if (value.contains("segmentFrame")) {
               view.segmentFrame=value.value("segmentFrame").toString();
               if (!QRegularExpression("^[A-Za-z][A-Za-z0-9_]*\\.[A-Za-z][A-Za-z0-9_]*$").match(view.segmentFrame).hasMatch() ||
                   view.reference!=view.segmentFrame.section('.',0,0) || !view.automaticTrajectory.isEmpty())
                  throw std::runtime_error("Named segment camera must reference Object.NamedPropagate and cannot frame a whole trajectory");
            }
            if (value.contains("automaticBody") && (!value.value("automaticBody").isString() || (!view.automaticBody.isEmpty() &&
                (!identifier.match(view.automaticBody).hasMatch() || view.automaticBody=="CoordinateSystem" || !view.automaticTrajectory.isEmpty() || view.automaticBody!=view.reference))))
               throw std::runtime_error("Automatic body camera must match its object reference and cannot also frame a trajectory");
            if (value.contains("automaticTrajectory") && (!value.value("automaticTrajectory").isString() ||
                (!view.automaticTrajectory.isEmpty() && !identifier.match(view.automaticTrajectory).hasMatch())))
               throw std::runtime_error("Invalid automatic trajectory object name");
            view.fieldOfView=value.value("fieldOfView").toDouble(-1); view.perspective=value.value("perspective").toBool();
            if (!identifier.match(view.name).hasMatch() || names.contains(view.name) ||
                !value.value("reference").isString() || (!view.reference.isEmpty() && !identifier.match(view.reference).hasMatch()) ||
                !value.value("target").isString() || (!view.target.isEmpty() && !identifier.match(view.target).hasMatch()) ||
                !value.value("perspective").isBool() || !std::isfinite(view.fieldOfView) || view.fieldOfView<1 || view.fieldOfView>150)
               throw std::runtime_error("Invalid or duplicate named camera: check names, reference objects, projection and field of view");
            if (value.contains("bodyRelative") && !value.value("bodyRelative").isBool()) throw std::runtime_error("Named camera bodyRelative must be a boolean");
            view.bodyRelative=value.value("bodyRelative").toBool();
            for (const auto *key:{"lookAtRotation","shortestAngle"})
               if (value.contains(key) && !value.value(key).isBool()) throw std::runtime_error("Named camera alignment modes must be booleans");
            view.lookAtRotation=value.value("lookAtRotation").toBool(); view.shortestAngle=value.value("shortestAngle").toBool();
            if (!view.automaticTrajectory.isEmpty() && (view.bodyRelative || !view.reference.isEmpty() || (!view.target.isEmpty() && !view.lookAtRotation)))
               throw std::runtime_error("Automatic trajectory camera needs a plot-frame reference and LookAt rotation for any target");
            if (view.lookAtRotation && view.target.isEmpty()) throw std::runtime_error("Aligned camera needs a target");
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
QString retainQtCameraSettings(const QString &original,const QString &candidate,const QString &removedPlot)
{
   const auto before=qtCameraSettings(original),after=qtCameraSettings(candidate);
   QString result=candidate;
   for (auto it=before.cbegin();it!=before.cend();++it)
      if (it.key()!=removedPlot && !after.contains(it.key())) result.prepend(qtCameraDirective(it.key(),it.value()));
   return result;
}

QString removeQtCameraSetting(const QString &source,const QString &plot)
{
   qtCameraSettings(source); // Retain the existing ambiguity/format validation.
   const QRegularExpression line("^[ \\t]*% GMAT-Qt-Camera [^\\n]*(?:\\n|$)",QRegularExpression::MultilineOption);
   auto matches=line.globalMatch(source);
   while (matches.hasNext()) {
      const auto match=matches.next();
      if (qtCameraSettings(match.captured()).contains(plot)) {
         auto result=source; result.remove(match.capturedStart(),match.capturedLength()); return result;
      }
   }
   return source;
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
