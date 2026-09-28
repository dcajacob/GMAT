#include "GuiTest.hpp"
#include "GroundTrack.hpp"
#include "GroundTrackArea.hpp"
#include "GroundTrackWindow.hpp"
#include "Moderator.hpp"
#include "SubscriberException.hpp"
#include <cmath>
#include <limits>
template<class T>static T*Find(wxWindow*w){if(auto p=dynamic_cast<T*>(w))return p;for(auto c:w->GetChildren())if(auto p=Find<T>(c))return p;return nullptr;}
// Test-only member access: explicit template instantiation permits observing
// the existing private curve storage without changing production layout or API.
struct CurveMember {using type=std::vector<GroundTrackCurve*> GroundTrackArea::*;friend type member(CurveMember);};
template<class Tag,typename Tag::type M>struct MemberAccess {friend typename Tag::type member(Tag){return M;}};
template struct MemberAccess<CurveMember,&GroundTrackArea::data>;
struct Access {static std::vector<GroundTrackCurve*>& Curves(GroundTrackArea*a){return a->*member(CurveMember{});}};
class Regression:public wxTimer{public:void Notify()override{
 auto m=Moderator::Instance();auto frame=GmatAppData::Instance()->GetMainFrame();m->CreateSpacecraft("Spacecraft","SecondSC");GroundTrack track("WindowsStateTest");track.SetSolarSystem(m->GetSolarSystemInUse());
 for(const char*n:{"DefaultSC","SecondSC"}){track.SetStringParameter("Add",n);track.SetRefObject(m->GetConfiguredObject(n),Gmat::SPACECRAFT,n);}
 Check(track.Initialize(),"two spacecraft initialize through real Windows receiver");auto area=Find<GroundTrackArea>(frame);Check(area!=nullptr,"actual GroundTrack area exists");
 if(area){
 StringArray labels={"Epoch","DefaultSC.X","DefaultSC.Y","DefaultSC.Z","DefaultSC.Vx","DefaultSC.Vy","DefaultSC.Vz","SecondSC.X","SecondSC.Y","SecondSC.Z","SecondSC.Vx","SecondSC.Vy","SecondSC.Vz"};
 double values[]={21545,7100,0,1300,0,7.35,1,-7000,500,-900,0,-7.4,1};track.SetDataLabels(labels);track.Distribute(values,13);
 auto &curves=Access::Curves(area);Check(curves.size()==2&&curves[0]->size()==1&&curves[1]->size()==1,"full publication reaches both actual curves");
 if(curves.size()==2){double x0=curves[0]->Xi(0),y0=curves[0]->Yi(0),x1=curves[1]->Xi(0),y1=curves[1]->Yi(0);
 StringArray partial={labels[0]};partial.insert(partial.end(),labels.begin()+7,labels.end());double second[]={21545,-7000,500,-900,0,-7.4,1};track.SetDataLabels(partial);track.Distribute(second,7);
 Check(curves[0]->size()==1&&curves[1]->size()==2,"partial publication retains spacecraft identity");
 Check(std::abs(curves[1]->Xi(1)-x1)<1e-9&&std::abs(curves[1]->Yi(1)-y1)<1e-9,"partial publication preserves second spacecraft coordinates");
 StringArray order;std::vector<double> v;for(int i:{0,10,3,9,5,11,2,8,1,12,4,7,6}){order.push_back(labels[i]);v.push_back(values[i]);}track.SetDataLabels(order);track.Distribute(v.data(),int(v.size()));
 Check(curves[0]->size()==2&&std::abs(curves[0]->Xi(1)-x0)<1e-9&&std::abs(curves[0]->Yi(1)-y0)<1e-9,"reordered components preserve first coordinates");
 track.SetDataLabels(labels);auto count=curves[0]->size();track.Distribute(values,4);track.Distribute(nullptr,13);Check(curves[0]->size()==count,"incomplete and null data ignored");
 track.SetIntegerParameter("DataCollectFrequency",5);track.Initialize();
 for(int i=0;i<12;++i){values[0]=21545+i/86400.0;track.Distribute(values,13);}auto &again=Access::Curves(area);Check(again[0]->size()==3,"frequency five collects three of twelve actual curve points");
 }
 for(const char*n:{"DataCollectFrequency","UpdatePlotFrequency"})for(int value:{0,-1}){bool rejected=false;try{track.SetIntegerParameter(n,value);}catch(const SubscriberException&){rejected=true;}Check(rejected,"invalid frequency rejected");}
 Check(track.SetIntegerParameter("NumPointsToRedraw",0)==0,"zero redraw setting accepted");
 }
 frame->SetAutoExitAfterRun(true);frame->Close();}};
extern "C" int __wrap_main(int argc,char**argv){return RunGuiTest<Regression>(argc,argv);}
