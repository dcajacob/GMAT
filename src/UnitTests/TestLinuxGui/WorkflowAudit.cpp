#include "GuiTest.hpp"
#include "EditorPanel.hpp"
#include "OrbitViewCanvas.hpp"
#include "TsPlotCanvas.hpp"
#include "GmatMenuBar.hpp"
#include "SpacecraftPanel.hpp"
#include "BallisticsMassPanel.hpp"
#include <wx/modalhook.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/file.h>
#include <wx/button.h>
#include <wx/log.h>
#include <GL/gl.h>

template<class T> static T *Find(wxWindow *w)
{
   if (auto found=dynamic_cast<T *>(w)) return found;
   for (auto child:w->GetChildren()) if (auto found=Find<T>(child)) return found;
   return nullptr;
}
class Replies : public wxModalDialogHook
{
public:
   int reply = wxID_CANCEL, count = 0;
   wxString path;
   int Enter(wxDialog *dialog) override
   {
      ++count;
      if(auto message=dynamic_cast<wxMessageDialog *>(dialog)) std::printf("MESSAGE: %s\n",message->GetMessage().utf8_str().data());
      std::printf("DIALOG: %s\n",dialog->GetTitle().utf8_str().data());
      if (auto file=dynamic_cast<wxFileDialog *>(dialog))
      { if (!path.empty()) file->SetPath(path); return reply; }
      if (dialog->GetTitle() == "Please Confirm Close") return wxID_NO;
      return wxID_OK;
   }
};
class Stopper : public wxTimer
{
public:
   bool fired=false;
   void Notify() override { fired=true; GmatAppData::Instance()->GetMainFrame()->StopRunningMission(); }
};
class Regression : public wxTimer
{
   int step=0;
   Replies replies;
   wxString dir,script;
   wxLog *oldLog=nullptr;
public:
   void Notify() override
   {
      auto frame=GmatAppData::Instance()->GetMainFrame();
      wxCommandEvent event;
      if(step==0)
      {
         oldLog=wxLog::SetActiveTarget(new wxLogStderr()); replies.Register();
         dir=wxString::FromUTF8(std::getenv("GMAT_GUI_TEST_IMAGES"));
         script=dir+wxString::FromUTF8("/workflow script.script");
         frame->SetSize(1100,800);
         bool built=frame->BuildScript(script);
         Check(built,"build script from a path containing spaces");
         if (!built) { replies.Unregister(); delete wxLog::SetActiveTarget(oldLog); frame->SetAutoExitAfterRun(true); frame->Close(); return; }
         GmatTreeItemData item(script,GmatTree::SCRIPT_FILE);
         auto child=frame->CreateChild(&item,false);
         auto panel=Find<EditorPanel>(child); Check(panel!=nullptr,"open script editor through real MDI child");
         if (!panel) { frame->SetAutoExitAfterRun(true); frame->Close(); return; }
         child->Activate();
         panel->GetEditor()->AppendText("\n% audit edit\n");
         Check(child->IsDirty(),"editing marks the actual script tab dirty");
         int before=replies.count;
         panel->OnSaveAs(event);
         Check(replies.count>before && child->IsDirty(),"cancel Save As preserves unsaved edits");
         before=replies.count; child->Close();
         Check(replies.count>before && !child->CanClose() && !child->IsBeingDeleted(),"decline unsaved close keeps the tab open");
         panel->OnSave(event);
         Check(!child->IsDirty(),"ordinary Save clears actual child dirty state");
         { wxFile f(script);wxString text;f.ReadAll(&text);Check(text.Contains("% audit edit"),"saved script includes the edit"); }
         Check(frame->CloseAllChildren(),"saved editor closes without losing changes");
      }
      else if(step==1)
      {
         Check(frame->RunCurrentMission()==1,"run mission with native/converted orbit, ground track and XY output");
         auto xy=Find<TsPlotCanvas>(frame);Check(xy && xy->GetCurveCount()>0,"XY plot contains a curve");
         if(xy) { xy->SaveImage((dir+"/xy-export.bmp").utf8_str());wxImage image(dir+"/xy-export.bmp");Check(image.IsOk(),"XY plot exports a decodable image"); }
         if(auto orbit=Find<OrbitViewCanvas>(frame))
         {
            if(auto child=dynamic_cast<wxMDIChildFrame *>(orbit->GetParent()))child->Activate();
            orbit->SetGLContext();std::printf("RENDERER: %s\n",glGetString(GL_RENDERER));
            wxCommandEvent play(wxEVT_MENU,GmatMenu::TOOL_ANIMATION_PLAY);
            frame->OnAnimation(play);frame->StopAnimation();
            Check(!frame->IsAnimationRunning(),"native animation returns to stopped state");
         }
         frame->SetSize(800,600);
      }
      else if(step==2)
      {
         Screenshot(frame,"workflow-small");
         Check(frame->RunCurrentMission()==1,"mission reruns with existing plots");
         Check(frame->CloseAllChildren(),"close all plot tabs after repeated execution");
      }
      else if(step==3)
      {
         Check(frame->BuildScript(script),"rebuild after closing plots");
         Check(frame->RunCurrentMission()==1,"plots recreate and mission completes after close/rebuild");
         Check(frame->CloseAllChildren(),"close recreated plot windows");
         Check(frame->BuildScript(dir+"/long.script"),"build long-running mission for Stop check");
         Stopper stop;stop.StartOnce(100);int result=frame->RunCurrentMission();stop.Stop();
         std::printf("STOP RESULT: %d\n",result);
         Check(stop.fired && !frame->IsMissionRunning(),"Stop interrupts mission and returns GUI to idle");
      }
      else if(step==4)
      {
         Check(frame->BuildScript(script),"switch from stopped mission to original script");
         const GmatTree::ItemType types[]={GmatTree::SPACECRAFT,GmatTree::PROPAGATOR,GmatTree::ORBIT_VIEW,GmatTree::XY_PLOT};
         const char *names[]={"BuildTestSat","BuildTestProp","BuildTestView","AuditXY"};
         for(int repeat=0;repeat<2;++repeat)
            for(int i=0;i<4;++i)
            {
               GmatTreeItemData item(names[i],types[i]);auto child=frame->CreateChild(&item,false);
               Check(child!=nullptr,"resource properties panel opens");
               if(repeat==0 && i==0 && child)
               {
                  auto panel=Find<SpacecraftPanel>(child);
                  auto mass=Find<BallisticsMassPanel>(child);
                  auto input=mass ? Find<wxTextCtrl>(mass) : nullptr;
                  Check(panel && input,"locate spacecraft mass input");
                  if(panel && input)
                  {
                     auto object=GmatAppData::Instance()->GetGuiInterpreter()->GetConfiguredObject("BuildTestSat");
                     const double beforeMass=object->GetRealParameter("DryMass");
                     input->SelectAll();input->WriteText("invalid mass");input->MarkDirty();
                     wxCommandEvent changed(wxEVT_TEXT,input->GetId());changed.SetEventObject(input);
                     input->GetEventHandler()->ProcessEvent(changed);
                     int before=replies.count; panel->OnApply(event);
                     Check(replies.count>before && object->GetRealParameter("DryMass")==beforeMass,"invalid mass shows feedback and leaves model unchanged");
                     panel->OnCancel(event);
                  }
               }
               Check(frame->CloseAllChildren(),"unchanged properties panel closes");
            }
         frame->SetSize(1100,800);
         frame->OnNewScript(event);
         Check(Find<EditorPanel>(frame)!=nullptr,"new-script action creates an editor");
         if(auto panel=Find<EditorPanel>(frame))
         { panel->GetEditor()->SetText("% Theme audit\nCreate Spacecraft Example;\nBeginMissionSequence;\n"); panel->GetEditor()->SetSavePoint(); panel->SetEditorModified(false); frame->SetActiveChildDirty(false); }
      }
      else
      {
         Screenshot(frame,"workflow-editor");
         Check(frame->CloseAllChildren(),"unmodified new script closes");
         Check(frame->RunCurrentMission()==1,"mission runs successfully after Stop and panel lifecycle checks");
         replies.Unregister();delete wxLog::SetActiveTarget(oldLog);
         frame->SetAutoExitAfterRun(true);frame->Close();return;
      }
      ++step;StartOnce(250);
   }
};
extern "C" int __wrap_main(int argc,char **argv) { return RunGuiTest<Regression>(argc,argv); }
