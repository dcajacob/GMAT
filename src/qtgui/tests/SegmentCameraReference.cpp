// Optional segment-camera geometry probe; no OpenFrames application dependency.
#include "CameraAlignment.hpp"
#include "OrbitCamera.hpp"
#include <OpenFrames/View.hpp>
#include <OpenFrames/ReferenceFrame.hpp>
#include <iostream>
int main()
{
   osg::ref_ptr<OpenFrames::ReferenceFrame> root=new OpenFrames::ReferenceFrame("root"),segment=new OpenFrames::ReferenceFrame("FirstArc"),target=new OpenFrames::ReferenceFrame("Earth");
   root->addChild(segment); root->addChild(target);
   for (auto *frame:{root.get(),segment.get(),target.get()}) { frame->showAxes(OpenFrames::ReferenceFrame::NO_AXES); frame->showAxesLabels(OpenFrames::ReferenceFrame::NO_AXES); frame->showNameLabel(false); }
   const osg::Vec3d origin(6995,455,100),localEye(0,-10000,1000),localCenter(1,2,3),localUp(1,0,1);
   const osg::Quat attitude(.7,osg::Vec3d(1,2,3)/std::sqrt(14.0)); segment->setPosition(origin); segment->setAttitude(attitude); target->setPosition(osg::Vec3d());
   int cases=0; double maximum=0;
   for (bool relative:{false,true}) for (bool aligned:{false,true}) for (bool automatic:{false,true}) for (double aspect:{.4,1.0,2.5}) {
      osg::ref_ptr<OpenFrames::View> view=aligned ? new OpenFrames::View(root,segment,target,relative ? OpenFrames::View::RELATIVE_FRAME : OpenFrames::View::ABSOLUTE_FRAME,OpenFrames::View::DIRECT) : new OpenFrames::View(root,segment,relative ? OpenFrames::View::RELATIVE_FRAME : OpenFrames::View::ABSOLUTE_FRAME);
      view->setPerspective(45,aspect);
      if (automatic) view->resetView(); else view->getTrackball()->setTransformation(localEye,localCenter,localUp);
      const auto frame=relative ? attitude : osg::Quat(); const auto alignment=aligned ? cameraAlignment(frame.inverse()*(-origin),true) : osg::Quat();
      auto world=[&](const osg::Vec3d &value) { return frame*(alignment*value); };
      PlotModel model(PlotModel::Kind::Orbit); model.scriptedCamera=true; model.perspective=true; model.fieldOfView=45; model.segmentFrame="Sat.FirstArc";
      if (automatic) { model.automaticBody="Sat"; model.automaticRadius=1; }
      const auto eye=origin+world(automatic ? osg::Vec3d(0,-30000,0) : localEye),center=origin+world(automatic ? osg::Vec3d() : localCenter),up=world(automatic ? osg::Vec3d(0,0,1) : localUp);
      model.cameras.push_back({1,{eye.x(),eye.y(),eye.z()},{center.x(),center.y(),center.z()},{up.x(),up.y(),up.z()},false});
      // A segment's empty frame must not inherit the current spacecraft/model bounds.
      const QMap<QString,OrbitObjectBounds> objects{{"Sat",{osg::Vec3d(50,100,200),5000}}};
      const auto camera=orbitCamera(model,1,0,0,7000,aspect,nullptr,&objects);
      const auto actual=osg::Matrixd::lookAt(camera.target+camera.outward*camera.distance,camera.target,camera.up),expected=view->getTrackball()->getInverseMatrix();
      for (const auto &point:{osg::Vec3d(),origin,origin+osg::Vec3d(100,200,300)}) {
         const auto error=(point*actual-point*expected).length(); maximum=std::max(maximum,error);
         if (error>1e-4) { std::cerr<<"FAIL relative="<<relative<<" aligned="<<aligned<<" automatic="<<automatic<<" aspect="<<aspect<<" error="<<error<<'\n'; return 1; }
      }
      ++cases;
   }
   std::cout<<"PASS "<<cases<<" actual OpenFrames empty-segment-frame comparisons: stored/automatic, body/inertial, LookAt and three aspects; maximum point error="<<maximum<<" (1e-4 tolerance for OF float home-eye). No old whole-trajectory/origin matrices repeated.\n";
}
