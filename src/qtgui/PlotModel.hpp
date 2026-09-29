#pragma once
#include <QColor>
#include <QImage>
#include <QMap>
#include <QPointF>
#include <QVector>
#include <QString>
#include <deque>
#include <limits>
#include <array>
#include "StarCatalog.hpp"

struct PlotPoint
{
   double x = 0, y = 0, z = 0, epoch = 0;
   quint64 frame = 0;
   QColor color;
   bool connect = false;
   bool solver = false;
   double high = 0, low = 0;
   // Row-major body-fixed to plot-frame rotation, captured while engine objects live.
   std::array<double,9> bodyToView = {1,0,0,0,1,0,0,0,1};
   std::array<double,3> sunPosition = {0,0,0};
   bool hasSun = false;
   std::array<double,9> inertialToView = {1,0,0,0,1,0,0,0,1};
   int marker=-1;
   bool highlighted=false;
};
struct PlotCurve
{
   QString name;
   QColor color = Qt::red;
   std::deque<PlotPoint> points;
   QVector<quint64> breaks;
   bool breakNext = true, visible = true, lines = true, markers = false;
   bool showObject = true;
   double radius = 0;
   QString texturePath;
   QString modelPath;
   std::array<double,3> modelOffset = {0,0,0}, modelRotation = {0,0,0};
   double modelScale = 1;
   int width = 1, markerSize = 4;
   int markerType=0,lineStyle=100;
   bool errorBars=true;
   Qt::PenStyle style = Qt::SolidLine;
};
struct PlotStation
{
   QString name;
   double longitude = 0, latitude = 0;
   QColor color;
};
struct PlotCamera
{
   quint64 frame=0;
   std::array<double,3> eye{},target{},up{0,0,1};
   bool solver=false;
};
struct PlotModel
{
   enum class Kind { Orbit, GroundTrack, XY, Table };
   explicit PlotModel(Kind value) : kind(value) {}
   Kind kind;
   QString title, xLabel, yLabel, coordinates;
   QMap<int, PlotCurve> curves;
   QVector<PlotStation> stations;
   QImage map;
   bool grid = true, legend = true, labels = true, penDown = true, active = true;
   bool axes = true;
   bool xyPlane=false,eclipticPlane=false,wireframe=false,sunLine=false;
   bool footprints=false;
   bool sunlight = true;
   bool starsEnabled = false, starCatalogLoaded = false;
   int starCount = 7000;
   StarCatalog starCatalog;
   bool constellationsEnabled=false,constellationCatalogLoaded=false;
   ConstellationCatalog constellationCatalog;
   bool scriptedCamera=false, fitCamera=false;
   bool perspective=false;
   double fieldOfView=50;
   std::deque<PlotCamera> cameras;
   int maxPoints = 20000, updateFrequency = 1, pendingUpdates = 0;
   int defaultLineWidth = 1;
   int longitudeLines = 12, latitudeLines = 6;
   quint64 frame = 0, historyGeneration = 0;
   double lastEpoch = 0;
   void append(int curve, double x, double y, double z = 0, double epoch = 0,
               bool drawing = true, bool solver = false, double high = 0, double low = 0);
   void clear();
   void breakLines();
   void trim();
   static QVector<QPair<QPointF, QPointF>> groundSegments(const QPointF &a, const QPointF &b);
   static QVector<QPointF> groundFootprint(QPointF center,double radiusDegrees=5);
};
