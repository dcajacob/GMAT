// Optional qualification probe against a locally installed OpenFrames library.
// This is not a dependency of the Qt application or its normal test suite.
#include "CameraAlignment.hpp"
#include <OpenFrames/View.hpp>
#include <OpenFrames/ReferenceFrame.hpp>
#include <OpenFrames/TransformAccumulator.hpp>
#include <iostream>
#include <vector>
class Probe : public OpenFrames::FollowingTrackball {
public:
   using OpenFrames::FollowingTrackball::computeWorldToViewMatrix;
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
}
