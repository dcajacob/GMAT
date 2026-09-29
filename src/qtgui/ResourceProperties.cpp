#include "ResourceProperties.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "Moderator.hpp"
#include <QRegularExpression>
#include <cmath>
#include <algorithm>
#include <limits>
#include <stdexcept>

QVector<ResourceProperty> resourceProperties(GmatBase &object)
{
   QVector<ResourceProperty> fields;
   for (Integer id = 0; id < object.GetParameterCount(); ++id) {
      try {
         if (object.IsParameterReadOnly(id)) continue;
         ResourceProperty field;
         field.name = QString::fromStdString(object.GetParameterText(id));
         field.unit = QString::fromStdString(object.GetParameterUnit(id));
         switch (object.GetParameterType(id)) {
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
   return fields;
}

bool isResourceList(GmatBase &object, const QString &name)
{
   const auto type=QString::fromStdString(object.GetTypeName());
   return (name=="Add" && (type=="OrbitView" || type=="GroundTrack" || type=="GroundTrackPlot" || type=="ReportFile")) ||
          (name=="YVariables" && type=="XYPlot");
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
         if (name=="Add" && object.GetTypeName()!="ReportFile") {
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
   const QString replacement="GMAT "+key+" = {"+entries.join(", ")+"};";
   if (!matches.hasNext()) {
      if (object.GetStringArrayParameter(id).empty())
         return block+(block.endsWith('\n') ? "" : "\n")+replacement+"\n";
      throw std::runtime_error("Cannot locate the list safely in this resource's script");
   }
   const auto match=matches.next();
   if (matches.hasNext()) throw std::runtime_error("Multiple list assignments require the script editor");
   QString result=block;
   if (entries.isEmpty() && object.GetTypeName()=="ReportFile") {
      result.remove(match.capturedStart(),match.capturedLength());
      return result;
   }
   result.replace(match.capturedStart(),match.capturedLength(),replacement);
   if (name=="Add" && object.GetTypeName()!="ReportFile") {
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
   const auto id = object.GetParameterID(name.toStdString());
   if (object.IsParameterReadOnly(id)) throw std::runtime_error("Property is read-only");
   bool valid = false;
   switch (object.GetParameterType(id)) {
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
