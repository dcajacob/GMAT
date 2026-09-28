#include "GuiTest.hpp"
#include "OrbitViewCanvas.hpp"
#include <cmath>
template<class T>T* Find(wxWindow*w){if(auto p=dynamic_cast<T*>(w))return p;for(auto c:w->GetChildren())if(auto p=Find<T>(c))return p;return nullptr;}
struct CameraMember {using type=Camera OrbitViewCanvas::*;friend type member(CameraMember);};
template<class Tag,typename Tag::type M>struct MemberAccess {friend typename Tag::type member(Tag){return M;}};
template struct MemberAccess<CameraMember,&OrbitViewCanvas::mCamera>;
struct Access {static Camera& Get(OrbitViewCanvas*c){return c->*member(CameraMember{});}};
class Regression:public wxTimer {int step=0;OrbitViewCanvas*canvas=nullptr;Camera before;std::string mode=getenv("GMAT_WHEEL_MODE");
 void Wheel(int rotation){wxMouseEvent e(wxEVT_MOUSEWHEEL);e.m_wheelRotation=rotation;e.m_wheelDelta=120;e.SetShiftDown(mode=="shift");e.SetEventObject(canvas);canvas->GetEventHandler()->ProcessEvent(e);canvas->Update();}
public:void Notify()override{auto frame=GmatAppData::Instance()->GetMainFrame();
 if(step==0){Check(frame->RunCurrentMission()==1,"default mission completes");canvas=Find<OrbitViewCanvas>(frame);Check(canvas!=nullptr,"native orbit canvas exists");if(!canvas){frame->SetAutoExitAfterRun(true);frame->Close();return;}if(mode=="free"||mode=="astronaut"){wxKeyEvent e(wxEVT_KEY_DOWN);e.m_keyCode='Z';e.SetShiftDown(mode=="astronaut");canvas->GetEventHandler()->ProcessEvent(e);}before=Access::Get(canvas);Wheel(120);}
 else if(step==1){const auto &after=Access::Get(canvas);double d=(before.view_center-before.position).GetMagnitude();
 if(mode=="shift")Check(after.fovDeg<before.fovDeg,"shift wheel narrows camera field of view");
 else if(mode=="astronaut"){auto move=after.position-before.position;Check(move.GetMagnitude()>0&&((after.view_center-before.view_center)-move).GetMagnitude()<1e-8,"astronaut wheel moves eye and center together");}
 else{Check((after.view_center-after.position).GetMagnitude()<d,"wheel moves camera closer to view center");Check((after.view_center-before.view_center).GetMagnitude()<1e-8,"zoom keeps view center fixed");}
 Check(canvas->SetGLContext(),"camera changes retain a usable GL context");Wheel(-120);}
 else{const auto &after=Access::Get(canvas);Check((after.position-before.position).GetMagnitude()<1e-8&&std::abs(after.fovDeg-before.fovDeg)<1e-8,"opposite wheel restores eye and field of view");Check((after.view_center-before.view_center).GetMagnitude()<1e-8,"opposite wheel restores view center");frame->SetAutoExitAfterRun(true);frame->Close();return;}++step;StartOnce(300);}};
extern "C" int __wrap_main(int argc,char**argv){return RunGuiTest<Regression>(argc,argv);}
