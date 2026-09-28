#include "GuiTest.hpp"
#include "StructureReader3ds.hpp"
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osg/Texture>
#include <osg/Geode>
#include <fstream>
class Images : public osg::NodeVisitor
{
public:
   int textures=0;
   Images():osg::NodeVisitor(TRAVERSE_ALL_CHILDREN){}
   void State(osg::StateSet *state)
   {
      if (!state) return;
      for(unsigned int unit=0;unit<state->getTextureAttributeList().size();++unit)
         if(auto texture=dynamic_cast<osg::Texture *>(state->getTextureAttribute(unit,osg::StateAttribute::TEXTURE)))
         {
            ++textures;
            auto image=texture->getImage(0);
            Check(image && image->valid(),"OSG material has a valid texture image");
            if(image) Check(image->s()==1024 && image->t()==1024,"bundled texture retains original dimensions");
         }
   }
   void apply(osg::Node &node) override {State(node.getStateSet());traverse(node);}
   void apply(osg::Geode &node) override
   {
      State(node.getStateSet());
      for(unsigned int i=0;i<node.getNumDrawables();++i)State(node.getDrawable(i)->getStateSet());
      traverse(node);
   }
};
class Regression : public wxTimer
{
public:
   void Notify() override
   {
      std::string dir=std::getenv("GMAT_GUI_TEST_IMAGES");
      std::string model=dir+"/aura.3ds";
      StructureReader3ds native(model);native.Execute();
      Check(native.TheStructure!=nullptr,"native model loader accepts Aura");
      if(native.TheStructure)
      {
         std::ofstream summary(dir+"/native-summary.txt");native.TheStructure->WriteSummary(summary,0);
         int textures=0;
         for(int i=0;i<native.TheStructure->Materials.Size();++i)
         {
            const auto &name=native.TheStructure->Materials[i]->TextureMap;
            if(name.empty())continue;
            ++textures;wxImage image(wxString::FromUTF8(name));
            Check(image.IsOk(),"native diffuse material resolves its bundled image");
         }
         Check(textures>0,"native Aura contains textured materials");
      }
      auto node=osgDB::readRefNodeFile(model);Check(node.valid(),"OpenFrames OSG loader accepts Aura");
      if(node)
      {
         Images images;node->accept(images);Check(images.textures>0,"OSG Aura contains textured materials");
         Check(osgDB::writeNodeFile(*node,dir+"/scene.osgt"),"serialize complete OSG scene for baseline comparison");
      }
      auto frame=GmatAppData::Instance()->GetMainFrame();frame->SetAutoExitAfterRun(true);frame->Close();
   }
};
extern "C" int __wrap_main(int argc,char **argv){return RunGuiTest<Regression>(argc,argv);}
