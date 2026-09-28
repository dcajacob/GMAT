#include "GuiTest.hpp"
#include "OrbitViewCanvas.hpp"
#include <algorithm>
#include <array>
#include <vector>

// Expose existing protected data through member pointers for observation only.
// The production class needs no test accessors or altered visibility.
struct Inspect : OrbitViewCanvas
{
   static std::array<int,8> Indices(OrbitViewCanvas *c)
   {
      return {{c->*(&Inspect::mBeginIndex1),c->*(&Inspect::mEndIndex1),
         c->*(&Inspect::mBeginIndex2),c->*(&Inspect::mEndIndex2),
         c->*(&Inspect::mCurrIndex),c->*(&Inspect::mLastIndex),
         c->*(&Inspect::mNumData),c->*(&Inspect::mTotalPoints)}};
   }
   static std::vector<Real> Times(OrbitViewCanvas *c)
   {
      Real *times=c->*(&Inspect::mTime);
      return {times,times+(c->*(&Inspect::mNumData))};
   }
   static Real LastTime(OrbitViewCanvas *c)
   { return (c->*(&Inspect::mTime))[c->*(&Inspect::mLastIndex)]; }
   static bool DisplayIsWithinRecording(OrbitViewCanvas *c)
   {
      (c->*(&Inspect::ComputeActualIndex))();
      int a=c->*(&Inspect::mRealBeginIndex1),b=c->*(&Inspect::mRealEndIndex1);
      int d=c->*(&Inspect::mRealBeginIndex2),e=c->*(&Inspect::mRealEndIndex2);
      auto indices=Indices(c);
      auto valid=[&](int i) { return (i>=indices[0] && i<=indices[1]) ||
         (indices[2]>=0 && i>=indices[2] && i<=indices[3]); };
      for(int i=a;i<=b;++i)if(!valid(i))return false;
      if(d>=0)for(int i=d;i<=e;++i)if(!valid(i))return false;
      return true;
   }
   static int Limit(OrbitViewCanvas *c) { return c->*(&Inspect::maxData); }
};
static OrbitViewCanvas *Find(wxWindow *w)
{
   if(auto c=dynamic_cast<OrbitViewCanvas *>(w))return c;
   for(auto child:w->GetChildren())if(auto c=Find(child))return c;
   return nullptr;
}
class Regression : public wxTimer
{
   OrbitViewCanvas *canvas=nullptr;
   std::vector<Real> frames;
   int step=0;
   bool validDisplay=true;
   void Paint(wxPaintEvent &e)
   {
      if(canvas->IsAnimationRunning())
      {
         frames.push_back(Inspect::LastTime(canvas));
         validDisplay &= Inspect::DisplayIsWithinRecording(canvas);
      }
      e.Skip();
   }
public:
   void Notify() override
   {
      auto frame=GmatAppData::Instance()->GetMainFrame();
      if(step==0)
      {
         const wxString dir=wxString::FromUTF8(std::getenv("GMAT_GUI_TEST_IMAGES"));
         Check(frame->BuildScript(dir+"/animation.script"),"build animation fixture");
         Check(frame->RunCurrentMission()==1,"animation fixture mission completes");
         canvas=Find(frame);Check(canvas!=nullptr,"native canvas exists");
         if(!canvas){frame->SetAutoExitAfterRun(true);frame->Close();return;}
         if(auto child=dynamic_cast<wxMDIChildFrame *>(canvas->GetParent()))child->Activate();
         canvas->Bind(wxEVT_PAINT,&Regression::Paint,this);
         canvas->Refresh();
      }
      else
      {
         auto before=Inspect::Indices(canvas);auto data=Inspect::Times(canvas);
         const bool wrapped=before[7]>Inspect::Limit(canvas);
         std::printf("BUFFER: wrapped=%d count=%d total=%d oldest=%d last=%d\n",wrapped,before[6],before[7],before[0],before[5]);
         std::vector<Real> first,expected;
         for(int i=0;i<before[6];i+=3)expected.push_back(data[(before[0]+i)%Inspect::Limit(canvas)]);
         for(int repeat=0;repeat<2;++repeat)
         {
            validDisplay=true;frames.clear();canvas->ViewAnimation(5,3);canvas->Update();
            auto after=Inspect::Indices(canvas);
            Check(after==before,"replay preserves recording indices and counts");
            Check(Inspect::Times(canvas)==data,"replay preserves recorded epochs");
            Check(frames.size()>=std::min(size_t(3),expected.size()),"replay paints multiple frames while running");
            Check(validDisplay,"partial trails only draw within the selected recording");
            Check(frames==expected,"replay paints the expected recorded epochs");
            Check(std::is_sorted(frames.begin(),frames.end()),"replay epochs advance in chronological order");
            for(Real t:frames)std::printf("FRAME: %.12f\n",t);
            if(repeat==0)first=frames;
            else Check(frames==first,"second replay paints the same chronological sequence");
         }
         class Interrupt : public wxTimer
         {
            OrbitViewCanvas *canvas;
         public:
            bool fired=false;
            explicit Interrupt(OrbitViewCanvas *c):canvas(c) {}
            void Notify() override {fired=true;canvas->SetUserInterrupt();}
         } interrupt(canvas);
         if(before[6]>3)
         {
            interrupt.StartOnce(25);canvas->ViewAnimation(10,1);interrupt.Stop();
            Check(interrupt.fired && !canvas->IsAnimationRunning(),"Stop interrupts playback and returns to idle");
            Check(Inspect::Indices(canvas)==before && Inspect::Times(canvas)==data,
                  "interrupted replay preserves recording state");
            frames.clear();canvas->ViewAnimation(0,3);
            Check(frames==expected,"replay after Stop returns to the original sequence");
         }
         canvas->Unbind(wxEVT_PAINT,&Regression::Paint,this);
         frame->SetAutoExitAfterRun(true);frame->Close();return;
      }
      ++step;StartOnce(200);
   }
};
extern "C" int __wrap_main(int argc,char **argv) {return RunGuiTest<Regression>(argc,argv);}
