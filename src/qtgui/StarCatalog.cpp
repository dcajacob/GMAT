#include "StarCatalog.hpp"
#include <QFile>
#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>

StarCatalog StarCatalog::read(const QString &path)
{
   StarCatalog result;
   if (path.isEmpty()) { result.error="Star catalog path is empty"; return result; }
   QFile file(path);
   if (!file.open(QIODevice::ReadOnly|QIODevice::Text)) { result.error=file.errorString(); return result; }
   constexpr double radians=3.14159265358979323846/180;
   while (!file.atEnd()) {
      const auto line=file.readLine().trimmed();
      if (line.isEmpty() || line.startsWith('#') || line.startsWith('%')) continue;
      std::istringstream input(line.toStdString()); input.imbue(std::locale::classic());
      double ra,dec,magnitude;
      if (!(input>>ra>>dec>>magnitude) || !std::isfinite(ra) || !std::isfinite(dec) || !std::isfinite(magnitude) ||
          ra<0 || ra>360 || dec < -90 || dec>90) { ++result.rejectedLines; continue; }
      result.stars.push_back({{std::cos(ra*radians)*std::cos(dec*radians),
         std::sin(ra*radians)*std::cos(dec*radians),std::sin(dec*radians)},magnitude});
   }
   std::stable_sort(result.stars.begin(),result.stars.end(),[](const auto &a,const auto &b) { return a.magnitude<b.magnitude; });
   if (result.stars.isEmpty()) result.error="No valid stars in catalog";
   return result;
}
