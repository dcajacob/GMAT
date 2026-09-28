#include "GuiTest.hpp"
#include "OFWindowProxyPanel.hpp"
#include "OFScene.hpp"
#include "OFConfigurationPanel.hpp"
#include "GuiInterpreter.hpp"
#include "GmatWidget.hpp"
#include "GmatMdiChildFrame.hpp"
#include "GmatMenuBar.hpp"
#include <osgDB/Registry>
#include <osgDB/FileNameUtils>
#include <osgText/Text>
#include <osgViewer/CompositeViewer>
#include <osg/Geode>
class FontFilter:public osgDB::ReadFileCallback
{
public:
 osgDB::ReaderWriter::ReadResult readObject(const std::string &file,const osgDB::Options *options) override
 {
  auto ext=osgDB::getLowerCaseFileExtension(file);
  if(ext=="ttf" || ext=="otf"){
   auto mode=std::getenv("GMAT_FONT_TEST");
   if(mode && (std::string(mode)=="none" || file.find("cour")!=std::string::npos))return osgDB::ReaderWriter::ReadResult::FILE_NOT_FOUND;
  }
  return osgDB::Registry::instance()->readObjectImplementation(file,options);
 }
};
class HUD:public osg::NodeVisitor
{
public:
 int count=0;HUD():osg::NodeVisitor(TRAVERSE_ALL_CHILDREN){}
 void apply(osg::Geode &g) override{
  for(unsigned int i=0;i<g.getNumDrawables();++i)if(auto text=dynamic_cast<osgText::Text*>(g.getDrawable(i))){
   if(text->getCharacterSizeMode()!=osgText::Text::SCREEN_COORDS)continue;
   ++count;auto font=text->getFont();std::printf("HUD FONT: %s\n",font?font->getFileName().c_str():"built-in fallback");
   Check(text->getBoundingBox().valid() && text->getBoundingBox().xMax()>text->getBoundingBox().xMin(),"HUD text retains nonempty glyph layout");
   auto mode=std::getenv("GMAT_FONT_TEST");
   if(mode && std::string(mode)=="none")Check(font==nullptr,"missing optional fonts use OSG built-in fallback");
   else if(mode)Check(font && font->getFileName().find("Liberation")!=std::string::npos,"HUD selects Liberation without Microsoft fonts");
  }traverse(g);
 }
};

template<class T> T *Find(wxWindow *w){if(auto p=dynamic_cast<T*>(w))return p;for(auto c:w->GetChildren())if(auto p=Find<T>(c))return p;return nullptr;}
class PanelAccess:public OFConfigurationPanel {public:
 static void Dirty(OFConfigurationPanel &p,bool value){(p.*(&PanelAccess::EnableUpdate))(value);}
 static void Cancel(OFConfigurationPanel &p){wxCommandEvent e;(p.*(&PanelAccess::OnCancel))(e);}
 static void OK(OFConfigurationPanel &p){wxCommandEvent e;(p.*(&PanelAccess::OnOK))(e);}
};
class Regression:public wxTimer
{
 OFConfigurationPanel *config=nullptr;
 int step=0;OFWindowProxyPanel *panel=nullptr;double before=0;
public:
 void Notify() override
 {
  auto frame=GmatAppData::Instance()->GetMainFrame();
  if(step==0){osgDB::Registry::instance()->setReadFileCallback(new FontFilter);Check(frame->RunCurrentMission()==1,"OpenFrames default mission completes");panel=Find<OFWindowProxyPanel>(frame);Check(panel!=nullptr,"actual OpenFrames panel exists");if(!panel){frame->SetAutoExitAfterRun(true);frame->Close();return;}}
  else if(step==1){
   auto parent=dynamic_cast<GmatMdiChildFrame*>(panel->GetParent()->GetParent());
   Check(parent!=nullptr,"stable plot hierarchy matches existing two-parent lookup");
   if(parent){parent->Activate();std::printf("CHILD TYPE: %d\n",int(parent->GetItemType()));}
   auto proxy=panel->GetScene().GetWinProxy();
   HUD hud;proxy->getGridPosition(0U,0U)->getHUD()->accept(hud);Check(hud.count>0,"inspect actual OpenFrames HUD text");
   Check(proxy && proxy->isAnimating(),"renderer remains running after mission");
   if(proxy){proxy->pauseTime(true);panel->GetScene().SetTimeInWindowProxy(0);before=proxy->getTime();panel->GetScene().TogglePlayback();Check(!proxy->isTimePaused(),"local playback resumes after mission");}
  }
  else if(step==2){
   auto proxy=panel->GetScene().GetWinProxy();Check(proxy->getTime()!=before,"local playback advances time");panel->GetScene().TogglePlayback();Check(proxy->isTimePaused(),"local playback pauses");
   double scale=panel->GetScene().GetTimeScaleFromWindowProxy();panel->IncreaseScale();Check(panel->GetScene().GetTimeScaleFromWindowProxy()>scale,"local faster changes playback rate");panel->DecreaseScale();Check(panel->GetScene().GetTimeScaleFromWindowProxy()==scale,"local slower restores rate");
   Check(!frame->GetToolBar()->GetToolEnabled(GmatMenu::TOOL_ANIMATION_PLAY),"main animation toolbar is disabled for the plugin tab");
   wxCommandEvent play(wxEVT_MENU,GmatMenu::TOOL_ANIMATION_PLAY);frame->OnAnimation(play);Check(proxy->isTimePaused() && !frame->IsAnimationRunning(),"unsupported main-toolbar command does not disturb plugin playback state");frame->StopAnimation();
   Check(frame->CloseAllChildren(),"OpenFrames plot closes after playback");
  }
  else if(step==3 || step==5){
   auto object=GmatAppData::Instance()->GetGuiInterpreter()->GetConfiguredObject("DefaultOrbitView");
   GmatWidget *widget=nullptr;auto child=frame->CreatePluginChild("DefaultOrbitView","DefaultOrbitView","Configuration",GmatTree::USER_DEFINED_OBJECT,object,&widget);
   config=child?Find<OFConfigurationPanel>(child):nullptr;Check(config!=nullptr,"configuration panel created by real plugin factory");
   if(config){Check(dynamic_cast<GmatMdiChildFrame*>(config->GetParent()->GetParent())==child,"configuration parent lookup identifies actual child");PanelAccess::Dirty(*config,true);Check(child->IsDirty(),"configuration dirty state reaches actual child");if(step==3)PanelAccess::Cancel(*config);else {PanelAccess::Dirty(*config,false);PanelAccess::OK(*config);}}
  }
  else if(step==4 || step==6){Check(Find<OFConfigurationPanel>(frame)==nullptr,"configuration Cancel/OK destroys correct child");}
  else {frame->SetAutoExitAfterRun(true);frame->Close();return;}
  ++step;StartOnce(250);
 }
};
extern "C" int __wrap_main(int argc,char **argv){return RunGuiTest<Regression>(argc,argv);}
