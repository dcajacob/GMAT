#include "GuiTest.hpp"
#include "GroundTrack.hpp"
#include "GroundTrackArea.hpp"
#include "GroundTrackCurve.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "PlotInterface.hpp"
#include <vector>
#include <cmath>
#include <limits>
static std::vector<double> received;
static int calls=0,curvePoints=0;
static std::vector<GroundTrackCurve *> curves;
bool PlotInterface::UpdateGroundTrackData(const std::string &,const double,const double *p,const int count)
{received.assign(p,p+2*count);++calls;return true;}
extern "C" void __real__ZN16GroundTrackCurve7AddDataEddd(GroundTrackCurve *,double,double,double);
extern "C" void __wrap__ZN16GroundTrackCurve7AddDataEddd(GroundTrackCurve *c,double x,double y,double t)
{curves.push_back(c);++curvePoints;Check(std::isfinite(x)&&std::isfinite(y),"curve only receives finite positions");__real__ZN16GroundTrackCurve7AddDataEddd(c,x,y,t);}
static bool Near(double a,double b){return std::abs(a-b)<1e-9;}
class Regression : public wxTimer
{
public:
 void Notify() override
 {
   auto m=Moderator::Instance();auto frame=GmatAppData::Instance()->GetMainFrame();
   auto second=m->CreateSpacecraft("Spacecraft","SecondSC");
   GroundTrack track("StateTest");track.SetSolarSystem(m->GetSolarSystemInUse());
   for(const char *n:{"DefaultSC","SecondSC"})
   {track.SetStringParameter("Add",n);track.SetRefObject(m->GetConfiguredObject(n),Gmat::SPACECRAFT,n);}
   Check(track.Initialize(),"two-spacecraft track initializes");
   StringArray labels={"Epoch","DefaultSC.X","DefaultSC.Y","DefaultSC.Z","DefaultSC.Vx","DefaultSC.Vy","DefaultSC.Vz",
      "SecondSC.X","SecondSC.Y","SecondSC.Z","SecondSC.Vx","SecondSC.Vy","SecondSC.Vz"};
   double values[]={21545,7100,0,1300,0,7.35,1,-7000,500,-900,0,-7.4,1};
   track.SetDataLabels(labels);track.Distribute(values,13);auto original=received;
   Check(original.size()==4 && std::isfinite(original[0]) && std::isfinite(original[2]),"complete publication supplies both positions");
   double updated[]={21545,7100,0,1300,0,7.35,1,-6500,1200,-900,0,-7.4,1};
   track.Distribute(updated,13);auto partialExpected=received;
   track.Distribute(values,13);
   StringArray partial={labels[0]};partial.insert(partial.end(),labels.begin()+7,labels.end());
   double secondValues[]={21545,-6500,1200,-900,0,-7.4,1};
   track.SetDataLabels(partial);track.Distribute(secondValues,7);
   Check(received.size()==4 && std::isnan(received[0]) && std::isnan(received[1]),"absent first spacecraft is not assigned the second state");
   Check(received.size()==4 && Near(received[2],partialExpected[2]) && Near(received[3],partialExpected[3]),"second spacecraft retains its own slot");
   StringArray reordered;std::vector<double> reorderedValues;
   for(int i:{0,10,3,9,5,11,2,8,1,12,4,7,6}){reordered.push_back(labels[i]);reorderedValues.push_back(values[i]);}
   track.SetDataLabels(reordered);track.Distribute(reorderedValues.data(),reorderedValues.size());
   bool same=received.size()==original.size();for(size_t i=0;i<received.size()&&same;++i)same &= Near(received[i],original[i]);
   Check(same,"reordered state components produce identical coordinates");
   track.SetDataLabels(labels);calls=0;track.Distribute(values,4);
   Check(calls==0,"truncated publication is not read as a complete state");
   track.Distribute(values,10);
   Check(received.size()==4 && Near(received[0],original[0]) && std::isnan(received[2]),
         "truncated second state does not discard the complete first state");
   auto missing=labels;missing[2]="Unrelated.Y";track.SetDataLabels(missing);track.Distribute(values,13);
   Check(received.size()==4 && std::isnan(received[0]) && Near(received[2],original[2]),
         "a missing component omits only its own spacecraft");
   track.SetDataLabels(partial);calls=0;track.Distribute(secondValues,1);
   Check(calls==0,"epoch-only publication does not emit stale positions");
   track.SetDataLabels(labels);calls=0;track.Distribute(values,13);
   Check(calls==1 && Near(received[0],original[0]) && Near(received[2],original[2]),"full publication recovers after missing data");
   calls=0;track.Distribute(nullptr,13);Check(calls==0,"null publication is ignored safely");
   auto area=new GroundTrackArea(frame,"StateMap");double nan=std::numeric_limits<double>::quiet_NaN();
   double absent[]={nan,nan,10,20};curvePoints=0;curves.clear();area->AddData(21545,absent,2);
   Check(curvePoints==1,"map omits the absent spacecraft without shifting curve slots");
   double both[]={30,40,11,21};area->AddData(21546,both,2);
   Check(curvePoints==3,"both curves accept the next complete publication");
   Check(curves.size()==3 && curves[0]==curves[2] && curves[0]!=curves[1],
         "missing positions never shift spacecraft curve identity");
   area->Destroy();frame->SetAutoExitAfterRun(true);frame->Close();
 }
};
extern "C" int __wrap_main(int argc,char **argv){return RunGuiTest<Regression>(argc,argv);}
