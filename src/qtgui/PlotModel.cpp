#include "PlotModel.hpp"
#include <algorithm>
#include <cmath>

void PlotModel::append(int index, double x, double y, double z, double epoch,
                      bool drawing, bool solver, double high, double low)
{
   auto &curve = curves[index];
   if (!active) { curve.breakNext = true; return; }
   if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(epoch)) {
      curve.breakNext = true; return;
   }
   bool connect = penDown && drawing && !curve.breakNext && !curve.points.empty();
   // Solver iterations and backward propagation must not join unrelated arcs.
   if (!curve.points.empty() && curve.points.back().solver != solver) connect = false;
   high = std::isfinite(high) && high >= 0 ? high : 0;
   low = std::isfinite(low) && low >= 0 ? low : 0;
   curve.points.push_back({x, y, z, epoch, frame, curve.color, connect, solver, high, low});
   curve.breakNext = !penDown || !drawing;
   while (curve.points.size() > static_cast<size_t>(maxPoints)) curve.points.pop_front();
   if (!curve.points.empty()) curve.points.front().connect = false;
   if (!curve.points.empty()) curve.breaks.erase(std::remove_if(curve.breaks.begin(),curve.breaks.end(),
      [&](quint64 point) { return point < curve.points.front().frame; }),curve.breaks.end());
   lastEpoch = epoch;
}
void PlotModel::clear()
{
   for (auto &curve : curves) { curve.points.clear(); curve.breaks.clear(); curve.breakNext = true; }
   frame = 0; pendingUpdates = 0;
}
void PlotModel::breakLines()
{
   for (auto &curve : curves) curve.breakNext = true;
}
void PlotModel::trim()
{
   maxPoints = std::max(2, maxPoints);
   for (auto &curve : curves) {
      while (curve.points.size() > static_cast<size_t>(maxPoints)) curve.points.pop_front();
      if (!curve.points.empty()) curve.points.front().connect = false;
   }
}
QVector<QPair<QPointF, QPointF>> PlotModel::groundSegments(const QPointF &a, const QPointF &b)
{
   if (std::abs(b.x() - a.x()) <= 180.0) return {{a, b}};
   const double adjusted = b.x() + (b.x() > a.x() ? -360.0 : 360.0);
   const double edge = adjusted > a.x() ? 180.0 : -180.0;
   const double fraction = (edge - a.x()) / (adjusted - a.x());
   const double latitude = a.y() + fraction * (b.y() - a.y());
   return {{a, {edge, latitude}}, {{-edge, latitude}, b}};
}
