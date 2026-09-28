#include "GuiTest.hpp"
#include "OrbitViewCanvas.hpp"
#include "ModelObject.hpp"
#include <GL/gl.h>
#include <GL/glu.h>
#include <vector>
#include <fstream>
#include <iterator>
template<class T>T* Find(wxWindow*w){if(auto p=dynamic_cast<T*>(w))return p;for(auto c:w->GetChildren())if(auto p=Find<T>(c))return p;return nullptr;}
class Access:public OrbitViewCanvas{public:static bool Load(ViewCanvas& c,const std::string&p){return (c.*(&Access::LoadImage))(p,0);}};
static void Unpack(int a,int row=0,int y=0,int x=0){glPixelStorei(GL_UNPACK_ALIGNMENT,a);glPixelStorei(GL_UNPACK_ROW_LENGTH,row);glPixelStorei(GL_UNPACK_SKIP_ROWS,y);glPixelStorei(GL_UNPACK_SKIP_PIXELS,x);}
static std::vector<unsigned char> Pixels(int level,int &w,int &h){glGetTexLevelParameteriv(GL_TEXTURE_2D,level,GL_TEXTURE_WIDTH,&w);glGetTexLevelParameteriv(GL_TEXTURE_2D,level,GL_TEXTURE_HEIGHT,&h);std::vector<unsigned char> p(w*h*3);if(!p.empty())glGetTexImage(GL_TEXTURE_2D,level,GL_RGB,GL_UNSIGNED_BYTE,p.data());return p;}
class Regression:public wxTimer{public:void Notify()override{
 auto frame=GmatAppData::Instance()->GetMainFrame();Check(frame->RunCurrentMission()==1,"native mission completes");auto canvas=Find<OrbitViewCanvas>(frame);Check(canvas&&canvas->SetGLContext(),"native OpenGL context exists");
 if(canvas){printf("RENDERER: %s / %s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));std::string dir=getenv("GMAT_GUI_TEST_IMAGES");glPixelStorei(GL_PACK_ALIGNMENT,1);
 for(int width:{17,32})for(const char* ext:{"png","bmp","jpg"}){
 wxImage image(width,19);for(int y=0;y<19;++y)for(int x=0;x<width;++x)image.SetRGB(x,y,(x*13+y*7)%256,(y*17+x*3)%256,(x*5+y*11)%256);
 std::string texture=std::string("textureX.")+ext,path=dir+"/"+texture;image.SaveFile(wxString::FromUTF8(path));wxImage decoded(wxString::FromUTF8(path));
 std::ifstream in(dir+"/source.3ds",std::ios::binary);std::string b((std::istreambuf_iterator<char>(in)),{});size_t at=0;while((at=b.find("aura_map.jpg",at))!=std::string::npos){b.replace(at,12,texture);at+=12;}std::ofstream out(dir+"/model.3ds",std::ios::binary);out.write(b.data(),b.size());out.close();
 for(bool model:{false,true}){
 GLuint reference;glGenTextures(1,&reference);glBindTexture(GL_TEXTURE_2D,reference);Unpack(1);auto expected=model?decoded:decoded.Mirror(false);
 Check(gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGB,width,19,GL_RGB,GL_UNSIGNED_BYTE,expected.GetData())==0,"reference GLU mipmap upload succeeds");
 std::vector<std::vector<unsigned char>> levels;std::vector<int> widths,heights;for(int l=0;l<8;++l){int w,h;auto p=Pixels(l,w,h);if(!w||!h)break;levels.push_back(p);widths.push_back(w);heights.push_back(h);}glDeleteTextures(1,&reference);
 for(int alignment:{1,4,8}){
 GLuint id;glGenTextures(1,&id);glBindTexture(GL_TEXTURE_2D,id);Unpack(alignment,41,2,3);ModelObject object;
 if(model)Check(object.Load(dir+"/model.3ds")!=0,"model upload succeeds");else Check(Access::Load(*canvas,path),"plot upload succeeds");
 GLint a,r,y,x;glGetIntegerv(GL_UNPACK_ALIGNMENT,&a);glGetIntegerv(GL_UNPACK_ROW_LENGTH,&r);glGetIntegerv(GL_UNPACK_SKIP_ROWS,&y);glGetIntegerv(GL_UNPACK_SKIP_PIXELS,&x);
 Check(a==alignment&&r==41&&y==2&&x==3,"caller pixel-store state restored");
 printf("CASE: %s %dx19 %s alignment=%d\n",model?"model":"plot",width,ext,alignment);
 for(size_t l=0;l<levels.size();++l){int w,h;auto actual=Pixels(int(l),w,h);Check(w==widths[l]&&h==heights[l]&&actual==levels[l],"Windows mip level matches tightly packed GLU reference");}
 glDeleteTextures(1,&id);
 }
 }
 }
 }
 frame->SetAutoExitAfterRun(true);frame->Close();}};
extern "C" int __wrap_main(int argc,char**argv){return RunGuiTest<Regression>(argc,argv);}
