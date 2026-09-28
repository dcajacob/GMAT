#include "GuiTest.hpp"
#include "TimeDilator.hpp"
#include <cstring>
#include <new>

class Regression : public wxTimer
{
   TimeDilator *slider=nullptr;
   int step=0;
public:
   void Notify() override
   {
      auto frame=GmatAppData::Instance()->GetMainFrame();
      if(step==0)
      {
         wxSetAssertHandler([](const wxString &, int, const wxString &,
                              const wxString &condition, const wxString &message) {
            std::printf("FAIL: wx assertion: %s %s\n",condition.utf8_str().data(),message.utf8_str().data());
            ++failures;
         });
         // Deterministically expose use of uninitialized constructor members.
         void *storage=::operator new(sizeof(TimeDilator));
         std::memset(storage,0xa5,sizeof(TimeDilator));
         slider=new(storage) TimeDilator();
         Check(slider->GetCurrent()==1.5,"uncreated time control initializes without native-widget calls");
         Check(slider->Create(frame,wxID_ANY,wxPoint(10,100),wxSize(400,60)),"time control creates successfully");
         slider->SetLimits(100,200);slider->SetCurrent(150);
         Check(slider->GetToolTip()!=nullptr,"created time control has its tooltip");
      }
      else if(step==1)slider->SetSize(0,0);
      else if(step==2)slider->SetSize(400,60);
      else if(step==3)slider->SetSize(1,1);
      else if(step==4)slider->SetSize(300,45);
      else
      {
         Check(slider->GetCurrent()==150,"time value survives zero, tiny and restored sizes");
         Check(slider->GetClientSize().x>1 && slider->GetClientSize().y>1,"time control restores usable dimensions");
         slider->Destroy();frame->SetAutoExitAfterRun(true);frame->Close();return;
      }
      ++step;StartOnce(150);
   }
};
extern "C" int __wrap_main(int argc,char **argv)
{ return RunGuiTest<Regression>(argc,argv); }
