#include "GuiTest.hpp"
#include "EditorPanel.hpp"
#include "ScriptPanel.hpp"
#include <wx/file.h>
#include <wx/log.h>

class ProbePanel : public EditorPanel
{
public:
   ProbePanel(wxWindow *parent, const wxString &path) : EditorPanel(parent,path,false) {}
   void SaveTo(const wxString &path, bool build = false)
   {
      mFilename = path;
      wxCommandEvent event;
      if (build) SaveAndBuildScript(event); else SaveScript();
   }
   bool Modified() const { return mEditorModified; }
   bool Canceled() const { return mSaveCanceled; }
   wxString Path() const { return mScriptFilename; }
};
class LegacyProbe : public ScriptPanel
{
public:
   LegacyProbe(wxWindow *parent, const wxString &path) : ScriptPanel(parent,path,false) {}
   void SaveTo(const wxString &path) { mFilename=path; SaveScript(); }
   bool Modified() const { return mEditorModified; }
   bool Canceled() const { return mSaveCanceled; }
   wxString Path() const { return mScriptFilename; }
};
class Regression : public wxTimer
{
public:
   void Notify() override
   {
      auto frame = GmatAppData::Instance()->GetMainFrame();
      const wxString original = wxString::FromUTF8(std::getenv("GMAT_GUI_TEST_IMAGES"))+wxString::FromUTF8("/mission café.script");
      const wxString missing = original+"/missing.script"; // A file cannot be a directory.
      const wxString text = "% editable mission\nCreate Spacecraft TestSC;\n";
      { wxFile file(original,wxFile::write); Check(file.Write(text),"create isolated Unicode-path script"); }
      auto host = new wxFrame(frame,wxID_ANY,"Editor IO audit");
      auto panel = new ProbePanel(host,original);
      auto editor = panel->GetEditor();
      Check(editor->GetText() == text,"editor opens a script with spaces and Unicode in its path");
      editor->AppendText("% unsaved edits\n");
      panel->SetEditorModified(true);
      const wxString edited = editor->GetText();
      wxLog *oldLog = wxLog::SetActiveTarget(new wxLogStderr());
      const std::string mode=std::getenv("GMAT_GUI_TEST_MODE") ? std::getenv("GMAT_GUI_TEST_MODE") : "all";
      if (mode != "reload")
      {
      panel->SaveTo(missing);
      Check(panel->Canceled(),"failed save is reported to the caller");
      Check(panel->Modified(),"failed save retains panel modified status");
      Check(panel->Path() == original,"failed Save As retains the original script identity");
      Check(editor->GetText() == edited && editor->IsModified(),"failed save retains editor contents and dirty state");
      panel->SaveTo(missing,true);
      Check(panel->Canceled() && panel->Modified(),"failed save-and-build retains dirty state and cancels build");
      panel->SaveTo(original);
      Check(!panel->Canceled() && !panel->Modified(),"retry to writable path succeeds and clears dirty state");
      { wxFile file(original); wxString saved; file.ReadAll(&saved); Check(saved == edited,"successful save persists every edit"); }
      }
      if (mode != "save")
      {
      { wxFile file(original,wxFile::write); file.Write(edited); }
      editor->SetSavePoint();
      editor->AppendText("% still unsaved\n");
      const wxString beforeLoad = editor->GetText();
      const bool undo = editor->CanUndo();
      Check(!editor->LoadFile(missing),"failed load returns false");
      Check(editor->GetText() == beforeLoad && editor->IsModified(),"failed load preserves unsaved text and dirty state");
      Check(editor->CanUndo() == undo,"failed load preserves undo history");
      editor->Undo();
      Check(editor->GetText() == edited,"undo after failed load removes only the last edit");
      editor->Redo();
      Check(editor->GetText() == beforeLoad,"redo after failed load restores the edit");
      Check(editor->LoadFile() && editor->GetText() == edited,"failed load preserves the default reload filename");
      editor->AppendText("% preserve panel on missing file\n"); panel->SetEditorModified(true);
      const wxString panelText=editor->GetText();
      Check(wxRemoveFile(original),"remove isolated file before panel reload");
      panel->ReloadFile();
      Check(editor->GetText()==panelText && panel->Modified(),"panel reload of a disappeared file retains edits and dirty status");
      panel->SaveTo(original);
      Check(!panel->Canceled(),"panel can save after its source file disappears");
      }
      if (mode != "reload")
      {
      auto legacy=new LegacyProbe(host,original);
      legacy->mFileContentsTextCtrl->AppendText("% legacy editor edit\n");legacy->SetEditorModified(true);
      legacy->SaveTo(missing);
      Check(legacy->Canceled() && legacy->Modified() && legacy->Path()==original,"plain text editor also retains identity and edits on save failure");
      legacy->SaveTo(original);
      Check(!legacy->Canceled() && !legacy->Modified(),"plain text editor retries a successful save");
      }
      delete wxLog::SetActiveTarget(oldLog);
      host->Destroy();
      frame->SetAutoExitAfterRun(true); frame->Close();
   }
};
extern "C" int __wrap_main(int argc, char **argv) { return RunGuiTest<Regression>(argc,argv); }
