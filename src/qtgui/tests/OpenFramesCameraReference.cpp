// Optional qualification probe against a locally installed OpenFrames library.
// This is not a dependency of the Qt application or its normal test suite.
#include "CameraAlignment.hpp"
#include "OrbitCamera.hpp"
#include <OpenFrames/View.hpp>
#include <OpenFrames/ReferenceFrame.hpp>
#include <OpenFrames/TransformAccumulator.hpp>
#include <iostream>
#include <vector>
class Probe : public OpenFrames::FollowingTrackball {
public:
   using OpenFrames::FollowingTrackball::computeWorldToViewMatrix;
};
class BoundedFrame : public OpenFrames::ReferenceFrame {
public:
   BoundedFrame() : OpenFrames::ReferenceFrame("trajectory"),bounds(osg::Vec3(105,210,315),std::sqrt(350.0)) {}
   const osg::BoundingSphere &getBound() const override { return bounds; }
private:
   osg::BoundingSphere bounds;
};
int main()
{
   osg::ref_ptr<OpenFrames::ReferenceFrame> root=new OpenFrames::ReferenceFrame("root");
   osg::ref_ptr<OpenFrames::ReferenceFrame> from=new OpenFrames::ReferenceFrame("from");
   osg::ref_ptr<OpenFrames::ReferenceFrame> to=new OpenFrames::ReferenceFrame("to");
   root->addChild(from); root->addChild(to);
   const osg::Vec3d origin(5,8,-2);
   const osg::Quat body(.7,osg::Vec3d(1,2,3)/std::sqrt(14.0));
   from->setPosition(origin); from->setAttitude(body);
   osg::ref_ptr<OpenFrames::TransformAccumulator> reference=new OpenFrames::TransformAccumulator(root,from);
   osg::ref_ptr<OpenFrames::TransformAccumulator> target=new OpenFrames::TransformAccumulator(root,to);
   osg::ref_ptr<Probe> probe=new Probe;
   double maximum=0; int cases=0;
   const std::vector<osg::Vec3d> directions={{0,0,0},{1e-8,0,0},{1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1},
      {1,2,3},{-3,1,-2},{1e-8,0,10},{1,1,-20},{-4,-2,7}};
   for (bool relative:{false,true}) for (bool shortest:{false,true}) for (const auto &direction:directions) {
      to->setPosition(origin+direction);
      probe->setTransformSources(reference,target,relative ? OpenFrames::View::RELATIVE_FRAME : OpenFrames::View::ABSOLUTE_FRAME,
         shortest ? OpenFrames::View::DIRECT : OpenFrames::View::AZEL);
      osg::Matrixd worldToLocal; probe->computeWorldToViewMatrix(worldToLocal);
      const auto localToWorld=osg::Matrixd::inverse(worldToLocal);
      const auto frame=relative ? body : osg::Quat();
      const auto alignment=cameraAlignment(frame.inverse()*direction,shortest);
      for (const auto &vector:std::vector<osg::Vec3d>{{1,0,0},{0,1,0},{0,0,1},{400,-1200,700}}) {
         const auto actual=origin+frame*(alignment*vector);
         const auto expected=vector*localToWorld;
         const double error=(actual-expected).length(); maximum=std::max(maximum,error);
         if (error>1e-8) { std::cerr<<"FAIL relative="<<relative<<" shortest="<<shortest<<" error="<<error<<'\n'; return 1; }
      }
      ++cases;
   }
   std::cout<<"PASS: "<<cases<<" OpenFrames frame/mode/direction cases; maximum point error="<<maximum<<'\n';
   osg::ref_ptr<BoundedFrame> trajectory=new BoundedFrame; root->addChild(trajectory);
   int automaticCases=0; double maximumAutomaticError=0;
   for (bool shortest:{false,true}) for (const auto &direction:directions) for (double aspect:{.4,1.0,2.5}) {
      to->setPosition(direction);
      osg::ref_ptr<OpenFrames::View> view=new OpenFrames::View(root,trajectory,to,OpenFrames::View::ABSOLUTE_FRAME,
         shortest ? OpenFrames::View::DIRECT : OpenFrames::View::AZEL);
      view->setPerspective(60,aspect); view->resetView();
      PlotModel model(PlotModel::Kind::Orbit); model.automaticTrajectory="Path"; model.perspective=true; model.fieldOfView=60; model.scriptedCamera=true;
      model.curves[0].name="Path"; model.curves[0].points.push_back({100,200,300}); model.curves[0].points.push_back({110,220,330});
      const auto rotation=cameraAlignment(direction,shortest);
      const auto eye=rotation*osg::Vec3d(0,-30000,0),up=rotation*osg::Vec3d(0,0,1);
      model.cameras.push_back({0,{eye.x(),eye.y(),eye.z()},{0,0,0},{up.x(),up.y(),up.z()},false});
      const auto camera=orbitCamera(model,0,0,0,1,aspect);
      const auto actual=osg::Matrixd::lookAt(camera.target+camera.outward*camera.distance,camera.target,camera.up);
      const auto expected=view->getTrackball()->getInverseMatrix();
      for (const auto &point:std::vector<osg::Vec3d>{{0,0,0},{100,200,300},{110,220,330}}) {
         const auto error=(point*actual-point*expected).length(); maximumAutomaticError=std::max(maximumAutomaticError,error);
         if (error>1e-4) { std::cerr<<"FAIL automatic LookAt error="<<error<<'\n'; return 1; }
      }
      ++automaticCases;
   }
   std::cout<<"PASS: "<<automaticCases<<" automatic trajectory LookAt cases; maximum point error="<<maximumAutomaticError<<'\n';
}
