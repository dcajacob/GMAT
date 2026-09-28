#include "GuiTest.hpp"
#include "GroundTrackPlotPanel.hpp"
#include "GroundTrackPlot.hpp"
#include "GroundTrack.hpp"
#include "GroundTrackWindow.hpp"
#include "OrbitViewCanvas.hpp"
#include "Moderator.hpp"
#include "OutputTree.hpp"
#include <wx/textctrl.h>

template<class T> static T *Find(wxWindow *w)
{
   if(auto found=dynamic_cast<T *>(w))return found;
   for(auto child:w->GetChildren())if(auto found=Find<T>(child))return found;
   return nullptr;
}
static bool Contains(wxTreeCtrl *tree, wxTreeItemId item, const wxString &name)
{
   if(tree->GetItemText(item)==name)return true;
   wxTreeItemIdValue cookie;
   for(auto child=tree->GetFirstChild(item,cookie);child.IsOk();child=tree->GetNextChild(item,cookie))
      if(Contains(tree,child,name))return true;
   return false;
}
struct PanelAccess : GroundTrackPlotPanel
{
   // Access a real panel's control without substituting a test panel or subscriber.
   static wxTextCtrl *Frequency(GroundTrackPlotPanel *panel)
   { return panel->*(&PanelAccess::mDataCollectFreqTextCtrl); }
};
class Regression : public wxTimer
{
public:
   void Notify() override
   {
      auto app=GmatAppData::Instance();auto frame=app->GetMainFrame();
      auto subscriber=Moderator::Instance()->GetConfiguredObject("DefaultGroundTrackPlot");
      Check(dynamic_cast<GroundTrack *>(subscriber)!=nullptr && dynamic_cast<GroundTrackPlot *>(subscriber)==nullptr,
            "default subscriber is GroundTrack, not the legacy concrete GroundTrackPlot");
      GmatTreeItemData item("DefaultGroundTrackPlot",GmatTree::GROUND_TRACK_PLOT);
      auto child=frame->CreateChild(&item,false);
      auto panel=Find<GroundTrackPlotPanel>(child);
      Check(panel!=nullptr,"GroundTrack editor opens through the normal resource path");
      if(panel)
      {
         auto frequency=PanelAccess::Frequency(panel);
         Check(frequency->GetValue()=="1","editor loads the collection frequency");
         frequency->SetValue("5");
         frequency->MarkDirty();
         wxCommandEvent changed(wxEVT_TEXT,frequency->GetId());
         changed.SetEventObject(frequency);
         frequency->GetEventHandler()->ProcessEvent(changed);
         wxCommandEvent apply;panel->OnApply(apply);
         Check(subscriber->GetIntegerParameter("DataCollectFrequency")==5,"Apply updates the real GroundTrack subscriber");
         Check(subscriber->GetGeneratingString().find("DataCollectFrequency = 5")!=std::string::npos,
               "saved script retains the edited setting");
      }
      auto tree=app->GetOutputTree();tree->UpdateOutput(false,true,true);
      Check(Contains(tree,tree->GetRootItem(),"DefaultGroundTrackPlot"),"visible GroundTrack is listed in the Output tree");
      subscriber->SetBooleanParameter("ShowPlot",false);tree->UpdateOutput(false,true,true);
      Check(!Contains(tree,tree->GetRootItem(),"DefaultGroundTrackPlot"),"hidden GroundTrack is omitted from the Output tree");
      subscriber->SetBooleanParameter("ShowPlot",true);
      child->Close();
      Check(frame->RunCurrentMission()==1,"mission runs after editing and closing the GroundTrack panel");
      auto trackWindow=Find<GroundTrackWindow>(frame);
      if(auto orbit=Find<OrbitViewCanvas>(frame))
         if(auto orbitWindow=dynamic_cast<wxMDIChildFrame *>(orbit->GetParent()))orbitWindow->Activate();
      GmatTreeItemData output("DefaultGroundTrackPlot",GmatTree::OUTPUT_GROUND_TRACK_PLOT);
      frame->CreateChild(&output);
      Check(trackWindow && frame->GetActiveChild()==trackWindow,"opening GroundTrack output activates the existing plot");
      frame->SetAutoExitAfterRun(true);frame->Close();
   }
};
extern "C" int __wrap_main(int argc,char **argv)
{ return RunGuiTest<Regression>(argc,argv); }
