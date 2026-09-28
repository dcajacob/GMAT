#include "GuiTest.hpp"
#include "ResourceTree.hpp"
#include "SpacecraftPanel.hpp"
#include "BallisticsMassPanel.hpp"
#include "Moderator.hpp"
#include <wx/modalhook.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include <wx/uiaction.h>
#include <wx/textctrl.h>

struct TreeAccess : ResourceTree
{
   static void Menu(ResourceTree *t,wxTreeItemId id) { (t->*(&TreeAccess::ShowMenu))(id,wxPoint(50,50)); }
   static void Delete(ResourceTree *t) { wxCommandEvent e;(t->*(&TreeAccess::OnDelete))(e); }
};
static wxTreeItemId Item(wxTreeCtrl *t,wxTreeItemId id,const wxString &name)
{
   if(t->GetItemText(id)==name)return id;
   wxTreeItemIdValue cookie;
   for(auto c=t->GetFirstChild(id,cookie);c.IsOk();c=t->GetNextChild(id,cookie))
   {
      auto found=Item(t,c,name);
      if(found.IsOk())return found;
   }
   return {};
}
template<class T> static T *Find(wxWindow *w)
{
   if(auto found=dynamic_cast<T *>(w))return found;
   for(auto c:w->GetChildren())if(auto found=Find<T>(c))return found;
   return nullptr;
}
class Replies : public wxModalDialogHook
{
public:
   int answer=wxID_YES;
   int confirmations=0;
   int Enter(wxDialog *d) override
   {
      if(auto m=dynamic_cast<wxMessageDialog *>(d))
      {
         std::printf("MESSAGE: %s\n",m->GetMessage().utf8_str().data());
         if(m->GetMessage().StartsWith("Are you sure you want to delete")) {++confirmations;return answer;}
      }
      return wxID_OK;
   }
};
class Regression : public wxTimer
{
   Replies replies;
   bool menuSeen=false,deleteVisible=false,renameVisible=false;
   void MenuOpen(wxMenuEvent &e)
   {
      if(auto menu=e.GetMenu())
      {
         menuSeen=true;
         deleteVisible=menu->FindItem("Delete")!=wxNOT_FOUND;
         renameVisible=menu->FindItem("Rename")!=wxNOT_FOUND;
         wxTheApp->CallAfter([]{wxUIActionSimulator input;input.Char(WXK_ESCAPE);});
      }
      e.Skip();
   }
public:
   void Notify() override
   {
      auto app=GmatAppData::Instance();auto frame=app->GetMainFrame();
      const wxString dir=wxString::FromUTF8(std::getenv("GMAT_GUI_TEST_IMAGES"));
      Check(frame->BuildScript(dir+"/deletion.script"),"build deletion fixture");
      replies.Register();auto oldLog=wxLog::SetActiveTarget(new wxLogStderr());
      auto tree=app->GetResourceTree();
      GmatTreeItemData selected("BuildTestSat",GmatTree::SPACECRAFT);
      auto child=frame->CreateChild(&selected,false);
      auto panel=Find<SpacecraftPanel>(child);auto mass=Find<BallisticsMassPanel>(child);
      auto input=mass?Find<wxTextCtrl>(mass):nullptr;
      Check(panel && input,"unrelated spacecraft properties panel opens");
      if(input)
      {
         input->SelectAll();input->WriteText("1234");input->MarkDirty();
         wxCommandEvent changed(wxEVT_TEXT,input->GetId());changed.SetEventObject(input);
         input->GetEventHandler()->ProcessEvent(changed);
      }
      auto unused=Item(tree,tree->GetRootItem(),"UnusedValue");
      Check(unused.IsOk(),"unused variable appears in the real resource tree");
      if(unused.IsOk())
      {
         tree->SelectItem(unused);
         tree->Bind(wxEVT_MENU_OPEN,&Regression::MenuOpen,this);
         TreeAccess::Menu(tree,unused);
         tree->Unbind(wxEVT_MENU_OPEN,&Regression::MenuOpen,this);
         Check(menuSeen && deleteVisible,"Delete remains available with an unrelated editor open");
         Check(menuSeen && !renameVisible,"Rename retains its existing open-editor restriction");
         replies.answer=wxID_NO;TreeAccess::Delete(tree);
         Check(Moderator::Instance()->GetConfiguredObject("UnusedValue")!=nullptr,"cancelled deletion preserves resource");
         replies.answer=wxID_YES;TreeAccess::Delete(tree);
         Check(Moderator::Instance()->GetConfiguredObject("UnusedValue")==nullptr,"confirmed deletion removes unused resource");
         Check(!Item(tree,tree->GetRootItem(),"UnusedValue").IsOk(),"deleted resource leaves the tree");
      }
      Check(frame->IsChildOpen(&selected) && input && input->GetValue()=="1234" && child->IsDirty(),
            "unrelated editor and its unsaved value survive deletion");
      for(const char *name:{"BuildTestSat","BuildTestProp","UsedTank","UsedSolver"})
      {
         auto id=Item(tree,tree->GetRootItem(),name);Check(id.IsOk(),"protected resource is present");
         int confirmations=replies.confirmations;
         if(id.IsOk()) {tree->SelectItem(id);TreeAccess::Delete(tree);}
         if(std::string(name)=="BuildTestSat")
            Check(replies.confirmations==confirmations,"selected resource's open editor blocks deletion before confirmation");
         Check(Moderator::Instance()->GetConfiguredObject(name)!=nullptr,"open editor or dependency prevents deletion");
      }
      wxCommandEvent cancel;if(panel)panel->OnCancel(cancel);
      replies.Unregister();delete wxLog::SetActiveTarget(oldLog);
      frame->SetAutoExitAfterRun(true);frame->Close();
   }
};
extern "C" int __wrap_main(int argc,char **argv) {return RunGuiTest<Regression>(argc,argv);}
