#pragma once
#include "PlotModel.hpp"
#include <osg/Vec3d>
#include <osg/Quat>
#include <cmath>
#include <algorithm>

struct OrbitCameraBasis
{
   osg::Vec3d target,right,up,outward;
   double distance,extent;
};
inline OrbitCameraBasis orbitCamera(const PlotModel &model,quint64 frame,double yaw,double pitch,double extent)
{
   double viewExtent=extent*1.1,distance=extent*4;
   osg::Vec3d target;
   osg::Vec3d right(std::cos(yaw),-std::sin(yaw),0);
   osg::Vec3d up(std::cos(pitch)*std::sin(yaw),std::cos(pitch)*std::cos(yaw),-std::sin(pitch));
   osg::Vec3d outward=right^up;
   const PlotCamera *camera=nullptr;
   if (model.scriptedCamera) for (const auto &sample:model.cameras) if (sample.frame<=frame) camera=&sample;
   if (camera) {
      target.set(camera->target[0],camera->target[1],camera->target[2]);
      outward.set(camera->eye[0]-target.x(),camera->eye[1]-target.y(),camera->eye[2]-target.z());
      distance=outward.normalize();
      up.set(camera->up[0],camera->up[1],camera->up[2]); up.normalize();
      right=up^outward;
      if (right.normalize()<1e-9) {
         // A parallel up vector has no unique roll. Choose a stable axis.
         up=std::abs(outward.z())<.9 ? osg::Vec3d(0,0,1) : osg::Vec3d(0,1,0);
         right=up^outward; right.normalize();
      }
      up=outward^right;
      const osg::Quat azimuth(-yaw,up);
      outward=azimuth*outward; right=azimuth*right;
      const osg::Quat elevation(-pitch,right);
      outward=elevation*outward; up=elevation*up;
      viewExtent=model.fitCamera ? (extent+target.length())*1.1 : distance*.4663076581549986;
      // Orthographic controls retain their established feel. Scripted
      // distance sets framing; pan/rotate are offsets from the tracked view.
      if (model.fitCamera) distance=4*(extent+target.length());
   }
   if (model.perspective && model.fitCamera)
      distance=(extent+target.length())*1.1/std::sin(std::clamp(model.fieldOfView,1.0,150.0)*3.14159265358979323846/360);
   return {target,right,up,outward,distance,viewExtent};
}
