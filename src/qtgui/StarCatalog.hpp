#pragma once
#include <QString>
#include <QVector>
#include <array>

struct CatalogStar {
   std::array<double,3> direction;
   double magnitude;
};
struct StarCatalog {
   QVector<CatalogStar> stars;
   QString error;
   int rejectedLines=0;
   static StarCatalog read(const QString &path);
};
struct ConstellationSegment {
   QString name;
   std::array<double,3> first,second;
};
struct ConstellationCatalog {
   QVector<ConstellationSegment> segments;
   QString error;
   int rejectedLines=0;
   static ConstellationCatalog read(const QString &path);
};
