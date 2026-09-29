#include "OrbitRenderer.hpp"
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QPainter>
#include <functional>
#include <osgViewer/Viewer>
#include <osgViewer/GraphicsWindow>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/Texture2D>
#include <osg/LineWidth>
#include <osg/Point>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <map>

namespace {
class Overlay final : public QWidget {
public:
   std::function<void(QPainter &)> draw;
   explicit Overlay(QWidget *parent) : QWidget(parent) {
      setAttribute(Qt::WA_TransparentForMouseEvents);
      setAttribute(Qt::WA_NoSystemBackground);
   }
   void paintEvent(QPaintEvent *) override { QPainter painter(this); draw(painter); }
};
osg::Vec4 color(const QColor &c) { return {float(c.redF()),float(c.greenF()),float(c.blueF()),1}; }
osg::ref_ptr<osg::Geode> sphere(double radius,const QString &path,const QColor &fallback)
{
   constexpr int columns=128,rows=64;
   constexpr double pi=3.14159265358979323846;
   auto vertices=new osg::Vec3Array;
   auto normals=new osg::Vec3Array;
   auto uv=new osg::Vec2Array;
   for (int row=0;row<=rows;++row) for (int col=0;col<=columns;++col) {
      const double latitude=pi*(double(row)/rows-.5),longitude=2*pi*(double(col)/columns-.5);
      const osg::Vec3 normal(std::cos(latitude)*std::cos(longitude),std::cos(latitude)*std::sin(longitude),std::sin(latitude));
      normals->push_back(normal); vertices->push_back(normal*radius);
      uv->push_back(osg::Vec2(double(col)/columns,double(row)/rows));
   }
   auto mesh=new osg::Geometry;
   mesh->setVertexArray(vertices); mesh->setNormalArray(normals,osg::Array::BIND_PER_VERTEX);
   mesh->setTexCoordArray(0,uv);
   auto triangles=new osg::DrawElementsUInt(GL_TRIANGLES);
   for (int row=0;row<rows;++row) for (int col=0;col<columns;++col) {
      const unsigned a=row*(columns+1)+col,b=a+columns+1;
      for (const auto index:{a,a+1,b,a+1,b+1,b}) triangles->push_back(index);
   }
   mesh->addPrimitiveSet(triangles);
   auto colors=new osg::Vec4Array;
   // Use Qt's deployed image readers, independent of OSG's optional image plugins.
   const QImage pixels=QImage(path).convertToFormat(QImage::Format_RGBA8888).mirrored();
   osg::ref_ptr<osg::Image> image;
   if (!pixels.isNull()) {
      image=new osg::Image;
      image->allocateImage(pixels.width(),pixels.height(),1,GL_RGBA,GL_UNSIGNED_BYTE);
      std::memcpy(image->data(),pixels.constBits(),pixels.sizeInBytes());
   }
   colors->push_back(image ? osg::Vec4(1,1,1,1) : color(fallback));
   mesh->setColorArray(colors,osg::Array::BIND_OVERALL);
   if (image) {
      auto texture=new osg::Texture2D(image);
      texture->setWrap(osg::Texture::WRAP_S,osg::Texture::REPEAT);
      texture->setWrap(osg::Texture::WRAP_T,osg::Texture::CLAMP_TO_EDGE);
      texture->setFilter(osg::Texture::MIN_FILTER,osg::Texture::LINEAR_MIPMAP_LINEAR);
      mesh->getOrCreateStateSet()->setTextureAttributeAndModes(0,texture,osg::StateAttribute::ON);
   }
   auto node=new osg::Geode; node->addDrawable(mesh); return node;
}
}

struct OrbitRenderer::Scene
{
   struct Curve {
      osg::ref_ptr<osg::Group> root=new osg::Group;
      osg::ref_ptr<osg::Geode> track=new osg::Geode;
      osg::ref_ptr<osg::MatrixTransform> body=new osg::MatrixTransform;
      QString texture;
      double radius=-1;
      Curve() { root->addChild(track); root->addChild(body); }
   };
   std::shared_ptr<PlotModel> model;
   osgViewer::Viewer viewer;
   osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> context;
   osg::ref_ptr<osg::Group> root=new osg::Group;
   osg::ref_ptr<osg::Geode> guides=new osg::Geode;
   std::map<int,Curve> curves;
   double zoom=1,yaw=.55,pitch=.45;
   QPointF pan;
   quint64 frame=std::numeric_limits<quint64>::max();
   explicit Scene(std::shared_ptr<PlotModel> value) : model(std::move(value)) {
      viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
      viewer.getCamera()->setClearColor(osg::Vec4(.015,.025,.045,1));
      viewer.getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
      // Point markers have a zero-size world-space bound but a visible pixel size.
      viewer.getCamera()->setSmallFeatureCullingPixelSize(-1);
      root->addChild(guides);
      viewer.setSceneData(root);
   }
   void synchronize(int width,int height,double pixelRatio) {
      double extent=1;
      for (const auto &curve:model->curves) if (curve.visible)
         for (const auto &p:curve.points) extent=std::max(extent,std::hypot(std::hypot(p.x,p.y),p.z)+curve.radius);
      // Keep the previous orthographic orbit controls and a stable replay fit.
      const double aspect=double(width)/height;
      const double halfHeight=extent*1.1/zoom/std::min(1.0,aspect),halfWidth=halfHeight*aspect;
      const osg::Vec3d right(std::cos(yaw),-std::sin(yaw),0);
      const osg::Vec3d up(std::cos(pitch)*std::sin(yaw),std::cos(pitch)*std::cos(yaw),-std::sin(pitch));
      const osg::Vec3d outward=right^up;
      const osg::Vec3d center=right*(-pan.x()*2*halfWidth/width)+up*(pan.y()*2*halfHeight/height);
      viewer.getCamera()->setViewMatrixAsLookAt(center+outward*(extent*4),center,up);
      viewer.getCamera()->setProjectionMatrixAsOrtho(-halfWidth,halfWidth,-halfHeight,halfHeight,extent*.01,extent*10);
      auto guideGeometry=new osg::Geometry;
      auto guidePositions=new osg::Vec3Array; auto guideColors=new osg::Vec4Array;
      auto line=[&](const osg::Vec3d &a,const osg::Vec3d &b,const osg::Vec4 &c) {
         guidePositions->push_back(a); guidePositions->push_back(b);
         guideColors->push_back(c); guideColors->push_back(c);
      };
      if (model->axes) {
         line({0,0,0},{extent,0,0},{.92f,.35f,.31f,1});
         line({0,0,0},{0,extent,0},{.39f,.84f,.53f,1});
         line({0,0,0},{0,0,extent},{.41f,.59f,1,1});
      }
      if (model->grid) for (int i=0;i<=5;++i) {
         const double x=halfWidth*(-1+2.0*i/5),y=halfHeight*(-1+2.0*i/5);
         const auto back=center-outward*extent*2;
         line(back+right*x-up*halfHeight,back+right*x+up*halfHeight,{.11f,.14f,.19f,1});
         line(back+up*y-right*halfWidth,back+up*y+right*halfWidth,{.11f,.14f,.19f,1});
      }
      guideGeometry->setVertexArray(guidePositions);
      guideGeometry->setColorArray(guideColors,osg::Array::BIND_PER_VERTEX);
      guideGeometry->addPrimitiveSet(new osg::DrawArrays(GL_LINES,0,guidePositions->size()));
      guideGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
      guides->removeDrawables(0,guides->getNumDrawables()); guides->addDrawable(guideGeometry);
      for (auto it=curves.begin();it!=curves.end();) {
         if (!model->curves.contains(it->first)) { root->removeChild(it->second.root); it=curves.erase(it); }
         else ++it;
      }
      for (auto it=model->curves.cbegin();it!=model->curves.cend();++it) {
         const auto &source=it.value();
         auto added=curves.try_emplace(it.key()); auto &curve=added.first->second;
         if (added.second) root->addChild(curve.root);
         curve.root->setNodeMask(source.visible ? ~0u : 0);
         if (!source.visible) continue;
         if (curve.radius!=source.radius || curve.texture!=source.texturePath) {
            curve.body->removeChildren(0,curve.body->getNumChildren());
            if (source.radius>0) curve.body->addChild(sphere(source.radius,source.texturePath,source.color));
            curve.radius=source.radius; curve.texture=source.texturePath;
         }
         auto geometry=new osg::Geometry;
         auto positions=new osg::Vec3Array; auto colors=new osg::Vec4Array;
         const PlotPoint *last=nullptr;
         unsigned start=0;
         for (const auto &p:source.points) {
            if (p.frame>frame) continue;
            if (!p.connect && positions->size()>start) {
               if (source.lines) geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP,start,positions->size()-start));
               start=positions->size();
            }
            positions->push_back(osg::Vec3(p.x,p.y,p.z)); colors->push_back(color(p.color)); last=&p;
         }
         if (source.lines && positions->size()>start) geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP,start,positions->size()-start));
         geometry->setVertexArray(positions); geometry->setColorArray(colors,osg::Array::BIND_PER_VERTEX);
         geometry->getOrCreateStateSet()->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
         geometry->getOrCreateStateSet()->setAttributeAndModes(new osg::LineWidth(source.width*pixelRatio));
         if (last && source.showObject && source.radius==0) {
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS,positions->size()-1,1));
            geometry->getOrCreateStateSet()->setAttributeAndModes(new osg::Point(7*pixelRatio));
         }
         curve.track->removeDrawables(0,curve.track->getNumDrawables()); curve.track->addDrawable(geometry);
         curve.body->setNodeMask(last && source.showObject ? ~0u : 0);
         if (last) {
            osg::Matrixd transform;
            for (int row=0;row<3;++row) for (int col=0;col<3;++col) transform(col,row)=last->bodyToView[row*3+col];
            transform.setTrans(last->x,last->y,last->z); curve.body->setMatrix(transform);

         }
      }
   }
};

OrbitRenderer::OrbitRenderer(std::shared_ptr<PlotModel> data,QWidget *parent) : QOpenGLWidget(parent),scene(new Scene(std::move(data)))
{
   QSurfaceFormat format; format.setVersion(2,1); format.setDepthBufferSize(24); format.setSamples(4); setFormat(format);
   setAttribute(Qt::WA_TransparentForMouseEvents);
   setObjectName("orbitRenderer");
   auto *overlay=new Overlay(this);
   overlay->draw=[this](QPainter &painter) { drawOverlay(painter); };
   overlay->setObjectName("orbitOverlay");
}
OrbitRenderer::~OrbitRenderer() { if (context()) disconnect(context(),nullptr,this,nullptr); releaseGraphics(); }
void OrbitRenderer::releaseGraphics()
{
   if (!context() || !scene->context) return;
   makeCurrent(); scene->root->releaseGLObjects(scene->context->getState());
   scene->viewer.setDone(true); scene->viewer.getCamera()->setGraphicsContext(nullptr);
   scene->context=nullptr; doneCurrent();
}
void OrbitRenderer::initializeGL()
{
   scene->context=new osgViewer::GraphicsWindowEmbedded(0,0,width(),height());
   scene->viewer.getCamera()->setGraphicsContext(scene->context);
   scene->viewer.getCamera()->setDrawBuffer(GL_COLOR_ATTACHMENT0);
   scene->viewer.getCamera()->setReadBuffer(GL_COLOR_ATTACHMENT0);
   scene->viewer.setDone(false); scene->viewer.realize();
   connect(context(),&QOpenGLContext::aboutToBeDestroyed,this,[this] { releaseGraphics(); });
}
void OrbitRenderer::resizeGL(int width,int height)
{
   const int w=qRound(width*devicePixelRatioF()),h=qRound(height*devicePixelRatioF());
   if (scene->context) scene->context->resized(0,0,w,h);
   scene->viewer.getCamera()->setViewport(0,0,w,h);
   findChild<QWidget *>("orbitOverlay")->setGeometry(rect());
}
void OrbitRenderer::setView(double zoom,double yaw,double pitch,QPointF pan,quint64 frame)
{
   scene->zoom=zoom; scene->yaw=yaw; scene->pitch=pitch; scene->pan=pan; scene->frame=frame; update();
}
void OrbitRenderer::paintGL()
{
   if (!scene->context || width()<=0 || height()<=0) return;
   scene->context->setDefaultFboId(defaultFramebufferObject());
   scene->synchronize(width(),height(),devicePixelRatioF()); scene->viewer.frame();
   findChild<QWidget *>("orbitOverlay")->update();
}
QImage OrbitRenderer::captureImage()
{
   auto image=grabFramebuffer();
   image.setDevicePixelRatio(devicePixelRatioF());
   QPainter painter(&image); drawOverlay(painter);
   return image;
}
void OrbitRenderer::drawOverlay(QPainter &painter)
{
   painter.setPen(QColor(220,225,235));
   painter.drawText(QRectF(10,height()-28,width()-20,22),Qt::AlignCenter,scene->model->coordinates+" · km");
   if (scene->model->labels) {
      const auto projection=scene->viewer.getCamera()->getViewMatrix()*scene->viewer.getCamera()->getProjectionMatrix();
      for (const auto &curve:scene->model->curves) {
         if (!curve.visible || !curve.showObject) continue;
         const PlotPoint *last=nullptr;
         for (const auto &point:curve.points) if (point.frame<=scene->frame) last=&point;
         if (!last) continue;
         const auto ndc=osg::Vec3d(last->x,last->y,last->z)*projection;
         painter.drawText(QPointF((ndc.x()+1)*width()/2+7,(1-ndc.y())*height()/2-7),curve.name);
      }
   }
   if (scene->model->legend) {
      int x=12;
      for (const auto &curve:scene->model->curves) {
         if (!curve.visible) continue;
         painter.setPen(QPen(curve.color,2)); painter.drawLine(x,20,x+15,20);
         painter.setPen(QColor(220,225,235)); painter.drawText(x+20,25,curve.name);
         x+=34+painter.fontMetrics().horizontalAdvance(curve.name);
      }
   }
}
