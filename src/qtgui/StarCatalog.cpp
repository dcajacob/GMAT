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

ConstellationCatalog ConstellationCatalog::read(const QString &path)
{
   ConstellationCatalog result;
   QFile file(path);
   if (!file.open(QIODevice::ReadOnly|QIODevice::Text)) { result.error=file.errorString(); return result; }
   QString name;
   constexpr double radians=3.14159265358979323846/180;
   auto direction=[&](double dec,double hours) {
      const double ra=hours*15*radians; dec*=radians;
      return std::array<double,3>{std::cos(ra)*std::cos(dec),std::sin(ra)*std::cos(dec),std::sin(dec)};
   };
   while (!file.atEnd()) {
      const auto line=file.readLine().trimmed();
      if (line.isEmpty() || line.startsWith('#') || line.startsWith('%')) continue;
      if (line.startsWith("N ")) { name=QString::fromUtf8(line.mid(2)).trimmed(); continue; }
      std::istringstream input(line.toStdString()); input.imbue(std::locale::classic());
      double dec1,ra1,dec2,ra2;
      if (!(input>>dec1>>ra1>>dec2>>ra2) || !std::isfinite(dec1) || !std::isfinite(dec2) ||
          !std::isfinite(ra1) || !std::isfinite(ra2) || std::abs(dec1)>90 || std::abs(dec2)>90 ||
          ra1<0 || ra1>24 || ra2<0 || ra2>24) { ++result.rejectedLines; continue; }
      result.segments.append({name,direction(dec1,ra1),direction(dec2,ra2)});
   }
   if (result.segments.isEmpty()) result.error="No valid constellation lines in catalog";
   return result;
}
