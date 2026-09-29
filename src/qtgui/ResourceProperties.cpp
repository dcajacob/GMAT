#include "ResourceProperties.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "Moderator.hpp"
#include "Rmatrix.hpp"
#include "Rvector.hpp"
#include <QRegularExpression>
#include <cmath>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace {
GmatBase *forcePropertyOwner(GmatBase &object,const QString &name,QString &leaf)
{
   if (!object.IsOfType("ODEModel")) return nullptr;
   for (int i=0;i<object.GetOwnedObjectCount();++i) {
      auto *force=object.GetOwnedObject(i);
      if (!force) continue;
      const auto prefix=QString::fromStdString(object.BuildPropertyName(force))+".";
      if (name.startsWith(prefix)) { leaf=name.mid(prefix.size()); return force; }
   }
   return nullptr;
}
}

QVector<ResourceProperty> resourceProperties(GmatBase &object)
{
   QVector<ResourceProperty> fields;
   for (Integer id = 0; id < object.GetParameterCount(); ++id) {
      try {
         // Array values are marked read-only for ordinary property syntax;
         // their specialized editor writes the array's indexed initial values.
         const bool arrayValues=object.GetTypeName()=="Array" && object.GetParameterText(id)=="RmatValue";
         if (object.IsParameterReadOnly(id) && !arrayValues) continue;
         ResourceProperty field;
         field.name = QString::fromStdString(object.GetParameterText(id));
         field.unit = QString::fromStdString(object.GetParameterUnit(id));
         switch (object.GetParameterType(id)) {
         case Gmat::RVECTOR_TYPE: {
            const auto &vector=object.GetRvectorParameter(id);
            field.rows=1; field.columns=vector.GetSize();
            QStringList values;
            for (int i=0;i<field.columns;++i) values.append(QString::number(vector[i],'g',17));
            field.value=values.join(" "); break;
         }
         case Gmat::RMATRIX_TYPE: {
            const auto &matrix=object.GetRmatrixParameter(id);
            field.rows=matrix.GetNumRows(); field.columns=matrix.GetNumColumns();
            QStringList rows;
            for (int r=0;r<field.rows;++r) {
               QStringList values;
               for (int c=0;c<field.columns;++c) values.append(QString::number(matrix(r,c),'g',17));
               rows.append(values.join(" "));
            }
            field.value=rows.join("; "); break;
         }
         case Gmat::REAL_TYPE: field.value = QString::number(object.GetRealParameter(id), 'g', 17); break;
         case Gmat::INTEGER_TYPE: field.value = QString::number(object.GetIntegerParameter(id)); break;
         case Gmat::UNSIGNED_INT_TYPE: field.value = QString::number(object.GetUnsignedIntParameter(id)); break;
         case Gmat::BOOLEAN_TYPE:
            field.choices = {"true", "false"}; field.value = object.GetBooleanParameter(id) ? "true" : "false"; break;
         case Gmat::ON_OFF_TYPE:
            field.choices = {"On", "Off"}; field.value = QString::fromStdString(object.GetOnOffParameter(id)); break;
         case Gmat::ENUMERATION_TYPE:
            for (const auto &choice : object.GetPropertyEnumStrings(id)) field.choices.append(QString::fromStdString(choice));
            [[fallthrough]];
         case Gmat::STRING_TYPE:
         case Gmat::FILENAME_TYPE:
         case Gmat::OBJECT_TYPE: field.value = QString::fromStdString(object.GetStringParameter(id)); break;
         case Gmat::STRINGARRAY_TYPE:
         case Gmat::OBJECTARRAY_TYPE:
            if (!isResourceList(object, field.name)) continue;
            for (const auto &entry : object.GetStringArrayParameter(id)) field.choices.append(QString::fromStdString(entry));
            field.value = field.choices.join(", ");
            field.choices.clear(); field.list = true; break;
         default: continue;
         }
         fields.append(field);
      } catch (BaseException &) {
         // Some plugin and computed properties have no scalar editor.
      }
   }
   if (object.IsOfType("Spacecraft")) {
      // Attitude parameters are delegated by name at IDs above the spacecraft
      // parameter table. Enumerate the owned model rather than guessing IDs.
      if (auto *attitude=object.GetOwnedObject(0)) {
         for (const auto &field:resourceProperties(*attitude)) {
            const bool duplicate=std::any_of(fields.cbegin(),fields.cend(),[&](const ResourceProperty &other) { return other.name==field.name; });
            if (!duplicate && !object.IsParameterReadOnly(object.GetParameterID(field.name.toStdString()))) fields.append(field);
         }
      }
   }
   if (object.IsOfType("ODEModel")) {
      // Engine-owned force components have their own parameter tables and
      // canonical prefixes (for example GravityField.Earth.Degree).
      for (int i=0;i<object.GetOwnedObjectCount();++i) {
         auto *force=object.GetOwnedObject(i); if (!force) continue;
         const auto prefix=QString::fromStdString(object.BuildPropertyName(force));
         if (prefix.isEmpty() || prefix=="UnknownForce") continue;
         for (auto field:resourceProperties(*force)) {
            if (field.list) continue;
            field.name=prefix+"."+field.name;
            const bool duplicate=std::any_of(fields.cbegin(),fields.cend(),[&](const ResourceProperty &other) { return other.name==field.name; });
            if (!duplicate) fields.append(field);
         }
      }
   }
   return fields;
}

bool isResourceList(GmatBase &object, const QString &name)
{
   // These lists use canonical {...} syntax. Other compound lists can carry
   // additional positional settings and need their own replacement handling.
   QString leaf;
   if (forcePropertyOwner(object,name,leaf)) return false;
   const auto id=object.GetParameterID(name.toStdString());
   const auto type=QString::fromStdString(object.GetTypeName());
   const bool supported=(name=="Add" && (type=="OrbitView" || type=="GroundTrack" || type=="GroundTrackPlot" || type=="ReportFile")) ||
      (name=="YVariables" && type=="XYPlot") ||
      (object.IsOfType("Spacecraft") && (name=="Tanks" || name=="Thrusters" || name=="AddHardware" || name=="AddPlates")) ||
      (object.IsOfType("Thruster") && name=="Tank") ||
      (type=="FiniteBurn" && name=="Thrusters") ||
      ((type=="ForceModel" || type=="ODEModel") && (name=="PrimaryBodies" || name=="PointMasses")) ||
      (object.IsOfType("EventLocator") && (name=="OccultingBodies" || name=="Observers" || name=="EclipseTypes"));
   const auto parameterType=object.GetParameterType(id);
   return supported && (parameterType==Gmat::OBJECTARRAY_TYPE || parameterType==Gmat::STRINGARRAY_TYPE) && !object.IsParameterReadOnly(id);
}

QString replaceResourceList(GmatBase &object, const QString &block, const QString &name, const QString &value)
{
   const auto id=object.GetParameterID(name.toStdString());
   if (!isResourceList(object,name) || object.IsParameterReadOnly(id)) throw std::runtime_error("This list cannot be edited here");
   // Accept resource/parameter names only. Candidate interpretation resolves
   // references and verifies subscriber-specific parameter restrictions.
   static const QRegularExpression reference("^[A-Za-z_][A-Za-z0-9_]*(?:\\.[A-Za-z_][A-Za-z0-9_]*)*$");
   QStringList entries;
   if (!value.trimmed().isEmpty()) {
      for (const auto &part : value.split(QRegularExpression("[,\\n]"))) {
         const auto entry=part.trimmed();
         if (!reference.match(entry).hasMatch()) throw std::runtime_error("Enter comma-separated resource or parameter names");
         if (name=="Add" && (object.GetTypeName()=="OrbitView" || object.GetTypeName()=="GroundTrack" || object.GetTypeName()=="GroundTrackPlot")) {
            auto *target=Moderator::Instance()->GetConfiguredObject(entry.toStdString());
            if (!target || !target->IsOfType(Gmat::SPACE_POINT))
               throw std::runtime_error(("Unknown space point: "+entry).toStdString());
            if (target->GetName()!=entry.toStdString())
               throw std::runtime_error("Use the resource name "+target->GetName()+" instead of "+entry.toStdString());
         }
         if (entries.contains(entry)) throw std::runtime_error("Each list entry must be unique");
         entries.append(entry);
      }
   }
   const QString key=QString::fromStdString(object.GetName())+"."+name;
   const QRegularExpression assignment("^[ \\t]*(?:GMAT[ \\t]+)?"+QRegularExpression::escape(key)+
      "[ \\t]*=[ \\t]*\\{[^;]*?\\}[ \\t]*;",QRegularExpression::MultilineOption);
   auto matches=assignment.globalMatch(block);
   QStringList serialized=entries;
   if (object.GetParameterType(id)==Gmat::STRINGARRAY_TYPE)
      for (auto &entry:serialized) entry="'"+entry+"'";
   const QString replacement="GMAT "+key+" = {"+serialized.join(", ")+"};";
   QString result=block;
   if (!matches.hasNext()) {
      if (object.GetStringArrayParameter(id).empty())
         result+=(block.endsWith('\n') ? "" : "\n")+replacement+"\n";
      else throw std::runtime_error("Cannot locate the list safely in this resource's script");
   } else {
      const auto match=matches.next();
      if (matches.hasNext()) throw std::runtime_error("Multiple list assignments require the script editor");
      if (entries.isEmpty() && object.GetTypeName()=="ReportFile") {
         result.remove(match.capturedStart(),match.capturedLength());
         return result;
      }
      result.replace(match.capturedStart(),match.capturedLength(),replacement);
   }
   if (name=="Tank" && object.IsOfType("Thruster")) {
      // Mixture ratios belong to tanks, not list positions. New tanks start
      // with equal weighting and can be adjusted in the numeric cell editor.
      const auto oldNames=object.GetStringArrayParameter(id);
      const auto &ratios=object.GetRvectorParameter("MixRatio");
      QStringList values;
      for (const auto &entry:entries) {
         const auto found=std::find(oldNames.begin(),oldNames.end(),entry.toStdString());
         const auto position=std::distance(oldNames.begin(),found);
         values.append(QString::number(position<ratios.GetSize() ? ratios[position] : 1.0,'g',17));
      }
      const QString ratioKey=QString::fromStdString(object.GetName())+".MixRatio";
      const QRegularExpression ratioAssignment("^[ \\t]*(?:GMAT[ \\t]+)?"+QRegularExpression::escape(ratioKey)+
         "[ \\t]*=[ \\t]*\\[[^;]*?\\][ \\t]*;",QRegularExpression::MultilineOption);
      const QString ratioValue="GMAT "+ratioKey+" = ["+values.join(" ")+"];";
      const auto ratio=ratioAssignment.match(result);
      if (ratio.hasMatch()) result.replace(ratio.capturedStart(),ratio.capturedLength(),ratioValue);
      else if (!entries.isEmpty()) result+=(result.endsWith('\n') ? "" : "\n")+ratioValue+"\n";
   }
   if (name=="Add" && (object.GetTypeName()=="OrbitView" || object.GetTypeName()=="GroundTrack" || object.GetTypeName()=="GroundTrackPlot")) {
      // DrawObject is positional in scripts. Preserve visibility by name when
      // entries move or disappear, and show newly added objects.
      const auto oldNames=object.GetStringArrayParameter(id);
      const auto flags=object.GetBooleanArrayParameter("DrawObject");
      QStringList visibility;
      for (const auto &entry:entries) {
         const auto found=std::find(oldNames.begin(),oldNames.end(),entry.toStdString());
         const auto position=static_cast<size_t>(std::distance(oldNames.begin(),found));
         visibility.append(position<flags.size() && !flags[position] ? "false" : "true");
      }
      const QString drawKey=QString::fromStdString(object.GetName())+".DrawObject";
      const QRegularExpression drawAssignment("^[ \\t]*(?:GMAT[ \\t]+)?"+QRegularExpression::escape(drawKey)+
         "[ \\t]*=[ \\t]*\\[[^;]*?\\][ \\t]*;",QRegularExpression::MultilineOption);
      const QString drawValue="GMAT "+drawKey+" = ["+visibility.join(" ")+"];";
      const auto draw=drawAssignment.match(result);
      if (draw.hasMatch()) result.replace(draw.capturedStart(),draw.capturedLength(),drawValue);
      else result+=(result.endsWith('\n') ? "" : "\n")+drawValue+"\n";
   }
   return result;
}

void setResourceProperty(GmatBase &object, const QString &name, const QString &value)
{
   QString leaf;
   if (auto *force=forcePropertyOwner(object,name,leaf)) { setResourceProperty(*force,leaf,value); return; }
   const auto id = object.GetParameterID(name.toStdString());
   const bool arrayValues=object.GetTypeName()=="Array" && name=="RmatValue";
   if (object.IsParameterReadOnly(id) && !arrayValues) throw std::runtime_error("Property is read-only");
   bool valid = false;
   switch (object.GetParameterType(id)) {
   case Gmat::RVECTOR_TYPE:
   case Gmat::RMATRIX_TYPE: {
      const bool vector=object.GetParameterType(id)==Gmat::RVECTOR_TYPE;
      const int rows=vector ? 1 : object.GetRmatrixParameter(id).GetNumRows();
      const int columns=vector ? object.GetRvectorParameter(id).GetSize() : object.GetRmatrixParameter(id).GetNumColumns();
      const auto inputRows=value.trimmed().split(';');
      if (inputRows.size()!=rows) throw std::runtime_error("Keep the existing number of rows; separate rows with semicolons");
      Rmatrix matrix(rows,columns);
      for (int r=0;r<rows;++r) {
         const auto cells=inputRows[r].trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
         if (cells.size()!=columns) throw std::runtime_error("Keep the existing number of columns");
         for (int c=0;c<columns;++c) {
            const double number=cells[c].toDouble(&valid);
            if (!valid || !std::isfinite(number)) throw std::runtime_error("Every cell must contain a finite number");
            matrix(r,c)=number;
         }
      }
      if (vector) {
         Rvector values(columns);
         for (int c=0;c<columns;++c) values[c]=matrix(0,c);
         object.SetRvectorParameter(id,values);
      } else object.SetRmatrixParameter(id,matrix);
      break;
   }
   case Gmat::REAL_TYPE: {
      const double number = value.toDouble(&valid);
      if (!valid || !std::isfinite(number)) throw std::runtime_error("Enter a finite number");
      object.SetRealParameter(id, number); break;
   }
   case Gmat::INTEGER_TYPE: {
      const auto number = value.toLongLong(&valid);
      if (!valid || number < std::numeric_limits<Integer>::min() || number > std::numeric_limits<Integer>::max())
         throw std::runtime_error("Enter an integer in range");
      object.SetIntegerParameter(id, static_cast<Integer>(number)); break;
   }
   case Gmat::UNSIGNED_INT_TYPE: {
      const auto number = value.toULongLong(&valid);
      if (!valid || value.trimmed().startsWith('-') || number > std::numeric_limits<UnsignedInt>::max())
         throw std::runtime_error("Enter a non-negative integer in range");
      object.SetUnsignedIntParameter(id, static_cast<UnsignedInt>(number)); break;
   }
   case Gmat::BOOLEAN_TYPE:
      if (value != "true" && value != "false") throw std::runtime_error("Select true or false");
      object.SetBooleanParameter(id, value == "true"); break;
   case Gmat::ON_OFF_TYPE:
      if (value != "On" && value != "Off") throw std::runtime_error("Select On or Off");
      if (!object.SetOnOffParameter(id, value.toStdString())) throw std::runtime_error("Property rejected value");
      break;
   case Gmat::STRING_TYPE:
   case Gmat::FILENAME_TYPE:
   case Gmat::OBJECT_TYPE:
   case Gmat::ENUMERATION_TYPE:
      if (!object.SetStringParameter(id, value.toStdString())) throw std::runtime_error("Property rejected value");
      break;
   default: throw std::runtime_error("This property requires a specialized editor");
   }
}
