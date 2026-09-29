#pragma once
#include <osg/Quat>
#include <osg/Vec3d>
#include <cmath>

// Map local camera vectors into a reference frame whose +Y axis points at
// the target. OF's two-frame view rotates the whole trackball pose, not just
// its center. Keep its singular-case threshold and rotation order.
inline osg::Quat cameraAlignment(const osg::Vec3d &direction,bool shortest)
{
   constexpr double epsilon=1e-6;
   if (direction.length()<=epsilon) return {};
   osg::Quat toCamera;
   if (shortest) {
      toCamera.makeRotate(direction,osg::Vec3d(0,1,0));
      return toCamera.inverse();
   }
   const double horizontal=std::hypot(direction.x(),direction.y());
   osg::Quat azimuth,elevation;
   if (horizontal>epsilon) azimuth.makeRotate(osg::Vec3d(direction.x(),direction.y(),0),osg::Vec3d(0,1,0));
   elevation.makeRotate(osg::Vec3d(0,horizontal,direction.z()),osg::Vec3d(0,1,0));
   // OSG quaternion composition follows its row-vector matrix convention.
   return (azimuth*elevation).inverse();
}
