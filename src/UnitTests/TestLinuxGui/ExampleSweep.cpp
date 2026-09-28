#include "GuiTest.hpp"
#include "ViewCanvas.hpp"
#include <wx/modalhook.h>
#include <wx/msgdlg.h>
#include <wx/weakref.h>
#include <wx/log.h>
#include <GL/gl.h>
#include <chrono>
#include <vector>

class Dialogs : public wxModalDialogHook
{
   int Enter(wxDialog *dialog) override
   {
      wxString message;
      if (auto box=dynamic_cast<wxMessageDialog *>(dialog)) message=box->GetMessage();
      message.Replace("\n"," | ");
      std::printf("AUDIT_DIALOG title=%s message=%s\n",dialog->GetTitle().utf8_str().data(),message.utf8_str().data());
      if (auto box=dynamic_cast<wxMessageDialog *>(dialog))
      {
         if (box->GetMessageDialogStyle() & wxYES_NO) return wxID_NO;
         if (box->GetMessageDialogStyle() & wxCANCEL) return wxID_CANCEL;
      }
      return wxID_OK;
   }
};
class Pulse : public wxTimer
{
   int seconds=0;
public:
   void Notify() override
   {
      ++seconds;
      if (seconds%10==0) std::printf("AUDIT_HEARTBEAT seconds=%d\n",seconds);
      const char *limit=std::getenv("GMAT_EXAMPLE_LIMIT");
      if (limit && seconds==std::atoi(limit))
      {
         std::printf("AUDIT_SOFT_TIMEOUT stop_requested\n");
         GmatAppData::Instance()->GetMainFrame()->StopRunningMission();
      }
   }
};
template<class T> static T *Find(wxWindow *w)
{
   if(auto result=dynamic_cast<T *>(w))return result;
   for(auto child:w->GetChildren())if(auto result=Find<T>(child))return result;
   return nullptr;
}
class Sweep : public wxTimer
{
   int step=0,plot=0,phase=0;
   Dialogs dialogs;
   Pulse pulse;
   std::vector<wxWeakRef<GmatMdiChildFrame>> plots;
public:
   void Notify() override
   {
      auto frame=GmatAppData::Instance()->GetMainFrame();
      if(step==0)
      {
         dialogs.Register();
         frame->SetSize(1100,800);
         const wxString script=wxString::FromUTF8(std::getenv("GMAT_EXAMPLE_SCRIPT"));
         std::printf("AUDIT_BUILD_BEGIN %s\n",script.utf8_str().data());
         const bool built=frame->BuildScript(script);
         std::printf("AUDIT_BUILD_END success=%d\n",built);
         if(!built){Finish(frame);return;}
      }
      else if(step==1)
      {
         std::printf("AUDIT_RUN_BEGIN\n");
         pulse.Start(1000);
         int result=frame->RunCurrentMission();
         pulse.Stop();
         std::printf("AUDIT_RUN_END result=%d active_plots=%d\n",result,frame->GetNumberOfActivePlots());
         for(auto node=frame->GetListOfMdiChildren()->GetFirst();node;node=node->GetNext())
         {
            auto child=static_cast<GmatMdiChildFrame *>(node->GetData());
            if(child->GetItemType()!=GmatTree::SCRIPT_FILE)plots.emplace_back(child);
         }
      }
      else if(plot<static_cast<int>(plots.size()))
      {
         auto child=plots[plot].get();
         if(!child){++plot;phase=0;StartOnce(100);return;}
         if(phase==0)
         {
            frame->SetSize(1100,800);child->Activate();child->Refresh();frame->Update();phase=1;
         }
         else
         {
            std::printf("AUDIT_ACTIVE match=%d shown=%d\n",frame->GetActiveChild()==child,child->IsShownOnScreen());
            wxSize size=child->GetClientSize();
            std::printf("AUDIT_WINDOW index=%d phase=%d type=%d title=%s size=%dx%d\n",plot,phase,child->GetItemType(),child->GetTitle().utf8_str().data(),size.x,size.y);
            if(auto canvas=Find<ViewCanvas>(child))
            {
               GLint viewport[4]={0};
               if(canvas->IsShownOnScreen() && canvas->SetGLContext())
               {
                  glGetIntegerv(GL_VIEWPORT,viewport);auto logical=canvas->GetClientSize();double scale=canvas->GetContentScaleFactor();
                  bool ok=viewport[2]==wxRound(logical.x*scale) && viewport[3]==wxRound(logical.y*scale);
                  std::printf("AUDIT_VIEWPORT match=%d actual=%dx%d expected=%dx%d\n",ok,viewport[2],viewport[3],wxRound(logical.x*scale),wxRound(logical.y*scale));
               }
            }
            const std::string name="plot-"+std::to_string(plot)+"-"+std::to_string(phase);
            if(std::getenv("GMAT_EXAMPLE_EXTERNAL_CAPTURE"))
            {
               wxString path=wxString::FromUTF8(std::getenv("GMAT_GUI_TEST_IMAGES"))+"/"+name+"-external.png";
               const wxChar *argv[]={wxT("import"),wxT("-window"),wxT("root"),path.c_str(),nullptr};
               long result=wxExecute(argv,wxEXEC_SYNC | wxEXEC_NOEVENTS);
               std::printf("AUDIT_SCREENSHOT result=%ld\n",result);
            }
            else Screenshot(frame,name.c_str());
            if(phase==1){frame->SetSize(800,600);child->Activate();child->Refresh();frame->Update();phase=2;}
            else{++plot;phase=0;}
         }
         StartOnce(200);return;
      }
      else {Finish(frame);return;}
      ++step;StartOnce(200);
   }
   void Finish(GmatMainFrame *frame)
   {
      std::printf("AUDIT_CLOSE_BEGIN\n");
      frame->SetAutoExitAfterRun(true);
      frame->Close();
      std::printf("AUDIT_CLOSE_RETURN\n");
   }
};
extern "C" int __wrap_main(int argc,char **argv)
{
   std::setvbuf(stdout,nullptr,_IONBF,0);
   wxLog::SetActiveTarget(new wxLogStderr());
   return RunGuiTest<Sweep>(argc,argv);
}
