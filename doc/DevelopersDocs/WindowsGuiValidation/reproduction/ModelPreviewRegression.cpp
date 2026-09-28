#include "GuiTest.hpp"
#include "VisualModelCanvas.hpp"
#include "OrbitViewCanvas.hpp"
#include "Moderator.hpp"
#include <wx/sizer.h>
#include <GL/gl.h>
#include <cmath>
template<class T>T* Find(wxWindow*w){if(auto p=dynamic_cast<T*>(w))return p;for(auto c:w->GetChildren())if(auto p=Find<T>(c))return p;return nullptr;}
class Regression:public wxTimer {int step=0;wxFrame*window=nullptr;VisualModelCanvas*preview=nullptr;OrbitViewCanvas*orbit=nullptr;
public:void Notify()override{auto frame=GmatAppData::Instance()->GetMainFrame();
 if(step==0){Check(frame->RunCurrentMission()==1,"default mission completes");orbit=Find<OrbitViewCanvas>(frame);Check(orbit&&orbit->SetGLContext(),"orbit context is current before preview creation");if(!orbit){frame->SetAutoExitAfterRun(true);frame->Close();return;}}
 if(step%2==0&&step<4){window=new wxFrame(frame,wxID_ANY,"Model preview regression",wxDefaultPosition,wxSize(500,400));orbit->SetGLContext();glClearColor(0.125f,0.25f,0.5f,0.75f);GLfloat before[4];glGetFloatv(GL_COLOR_CLEAR_VALUE,before);printf("PREVIEW_BEFORE %f %f %f context=%p\n",before[0],before[1],before[2],wglGetCurrentContext());auto spacecraft=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetConfiguredObject("DefaultSC"));preview=new VisualModelCanvas(window,spacecraft);printf("PREVIEW_CONSTRUCTED context=%p\n",wglGetCurrentContext());Check(orbit->SetGLContext(),"restore orbit context after wxGLCanvas creation");GLfloat color[4];glGetFloatv(GL_COLOR_CLEAR_VALUE,color);printf("PREVIEW_AFTER %f %f %f context=%p\n",color[0],color[1],color[2],wglGetCurrentContext());Check(std::abs(color[0]-.125)<1e-6&&std::abs(color[1]-.25)<1e-6&&std::abs(color[2]-.5)<1e-6,"preview constructor does not mutate the active orbit context");auto sizer=new wxBoxSizer(wxVERTICAL);sizer->Add(preview,1,wxEXPAND);window->SetSizer(sizer);window->Show();Check(preview->LoadModel(wxString::FromUTF8(getenv("GMAT_MODEL_FILE"))),"preview accepts bundled model");preview->Refresh();}
 else if(step<4){preview->Update();Check(preview->loadedModel!=nullptr,"preview paint loads model with a current context");Check(glGetString(GL_VERSION)!=nullptr,"preview has a usable OpenGL context");Screenshot(frame,"model-preview");window->Destroy();window=nullptr;}
 else{Check(orbit->SetGLContext(),"orbit context remains usable after repeated preview close");orbit->Refresh();orbit->Update();frame->SetAutoExitAfterRun(true);frame->Close();return;}++step;StartOnce(300);}};
extern "C" int __wrap_main(int argc,char**argv){return RunGuiTest<Regression>(argc,argv);}
