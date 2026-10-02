#pragma once
#include "PlotModel.hpp"
#include <osg/Vec3d>
#include <optional>

struct OrbitVelocitySegment { osg::Vec3d start,end; QColor color; };
// OF TrajectoryDealer records position + 1000 * view-frame velocity as the
// SegmentArtist endpoint. This is its fixed 1000 s visualization scale, not
// a propagation estimate, normalized arrow, or body-attitude rotation.
inline std::optional<OrbitVelocitySegment> orbitVelocitySegment(const PlotPoint &point)
{
   if (!point.hasVelocity || !PlotModel::usableOrbitPosition(point.x,point.y,point.z)) return std::nullopt;
   for (const auto value:point.viewVelocity) if (!PlotModel::usableValue(value)) return std::nullopt;
   const osg::Vec3d start(point.x,point.y,point.z);
   const auto end=start+osg::Vec3d(point.viewVelocity[0],point.viewVelocity[1],point.viewVelocity[2])*1000;
   if (!PlotModel::usableOrbitPosition(end.x(),end.y(),end.z())) return std::nullopt;
   return OrbitVelocitySegment{start,end,point.color};
}
