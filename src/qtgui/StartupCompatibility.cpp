#include "StartupCompatibility.hpp"
#include <QFile>
#include <QRegularExpression>

QString qtStartupCompatibilityError(const QString &path)
{
   QFile file(path);
   if (!file.open(QIODevice::ReadOnly))
      return QString("Cannot read startup file %1: %2").arg(path, file.errorString());
   const QRegularExpression assignment("^\\s*PLUGIN\\s+=\\s*(.*)$");
   const QRegularExpression wxProvider(
      "^(?:lib)?(?:OpenFramesInterface|OVtoOFI)d?(?:\\.(?:dll|dylib|so(?:\\.[0-9]+)*))?$",
      QRegularExpression::CaseInsensitiveOption);
   int number=0;
   while (!file.atEnd()) {
      ++number;
      const auto match=assignment.match(QString::fromUtf8(file.readLine()));
      if (!match.hasMatch()) continue;
      const auto plugin=match.captured(1).trimmed();
      auto portable=plugin;
      portable.replace('\\','/');
      if (wxProvider.match(portable.section('/',-1)).hasMatch())
         return QString("Startup line %1 enables %2, a wxWidgets-only plugin. "
                        "Use gmat_startup_qt.txt or remove this PLUGIN entry for the Qt GUI.")
            .arg(number).arg(plugin);
   }
   return {};
}
