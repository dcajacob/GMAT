#include "PlotWidget.hpp"
#include "OrbitRenderer.hpp"
#include "OrbitCamera.hpp"
#include <QGuiApplication>
#include <QResizeEvent>
#include <QAction>
#include <QFileDialog>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSaveFile>
#include <QMessageBox>
#include <QSlider>
#include <QStyle>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

namespace {
void drawMarker(QPainter &painter,QPointF p,int type,double size,const QColor &color,bool highlight)
{
   painter.save(); painter.setPen(QPen(color,1)); painter.setBrush(Qt::NoBrush);
   const double r=size/2;
   auto cross=[&] { painter.drawLine(p+QPointF(-r,-r),p+QPointF(r,r)); painter.drawLine(p+QPointF(-r,r),p+QPointF(r,-r)); };
   auto plus=[&] { painter.drawLine(p+QPointF(-r,0),p+QPointF(r,0)); painter.drawLine(p+QPointF(0,-r),p+QPointF(0,r)); };
   switch (type) {
   case 0: cross(); break;
   case 1: painter.drawEllipse(p,r,r); break;
   case 2: plus(); break;
   case 3: cross(); plus(); break;
   case 4: painter.drawRect(QRectF(p-QPointF(r,r),QSizeF(size,size))); break;
   case 5: painter.drawPolygon(QPolygonF({p+QPointF(0,-r),p+QPointF(r,0),p+QPointF(0,r),p+QPointF(-r,0)})); break;
   case 6: cross(); painter.drawRect(QRectF(p-QPointF(r,r),QSizeF(size,size))); break;
   case 7: case 8: {
      const double sign=type==7 ? 1 : -1;
      painter.drawPolygon(QPolygonF({p+QPointF(0,-r*sign),p+QPointF(r,r*sign),p+QPointF(-r,r*sign)})); break;
   }
   case 9: painter.drawEllipse(p,r,r); plus(); break;
   }
   if (highlight) painter.drawEllipse(p,r+3,r+3);
   painter.restore();
}
QPen curvePen(const PlotCurve &curve,const QColor &color)
{
   QPen pen(color,curve.width,curve.style);
   switch (curve.lineStyle) {
   case 101: pen.setStyle(Qt::DotLine); break;
   case 102: pen.setDashPattern({8,4}); break;
   case 103: pen.setDashPattern({4,2}); break;
   case 104: pen.setStyle(Qt::DashDotLine); break;
   case 106: pen.setStyle(Qt::NoPen); break;
   default: break;
   }
   return pen;
}
}

PlotCanvas::PlotCanvas(std::shared_ptr<PlotModel> model, QWidget *parent) : QWidget(parent), data(std::move(model))
{
   setMinimumSize(200, 140);
   setMouseTracking(true);
   setToolTip("Wheel: zoom · Drag: rotate orbit or pan chart · Shift-drag: pan orbit · Double-click: fit");
   if (data->kind==PlotModel::Kind::Orbit && QGuiApplication::platformName()!="offscreen" && QGuiApplication::platformName()!="minimal")
      renderer=new OrbitRenderer(data,this);
   if (data->scriptedCamera) scriptView();
}
void PlotCanvas::refresh() { if (renderer) renderer->setView(zoom,yaw,pitch,pan,visibleFrame); else update(); }
QImage PlotCanvas::captureImage() { return renderer ? renderer->captureImage() : grab().toImage(); }
void PlotCanvas::resizeEvent(QResizeEvent *event) { QWidget::resizeEvent(event); if (renderer) renderer->setGeometry(rect()); }
void PlotCanvas::fit() { zoom = 1; pan = {}; data->fitCamera=true; refresh(); }
void PlotCanvas::scriptView() {
   if (!data->scriptedCamera) return;
   zoom=1; pan={}; yaw=0; pitch=0; data->fitCamera=false; refresh();
}
void PlotCanvas::setFrame(quint64 value) { visibleFrame = value; refresh(); }
void PlotCanvas::zoomBy(double steps) { zoom = std::clamp(zoom * std::pow(1.15, steps), 0.02, 200.0); refresh(); }
void PlotCanvas::setViewAngles(double azimuth, double elevation) { yaw = azimuth; pitch = elevation; refresh(); }
void PlotCanvas::mousePressEvent(QMouseEvent *event) { lastMouse = event->position(); }
void PlotCanvas::mouseMoveEvent(QMouseEvent *event)
{
   const auto delta = event->position() - lastMouse; lastMouse = event->position();
   if (!(event->buttons() & Qt::LeftButton)) return;
   if (data->kind == PlotModel::Kind::Orbit && !(event->modifiers() & Qt::ShiftModifier)) {
      yaw = std::remainder(yaw + delta.x() * 0.008, 6.283185307179586);
      pitch = std::remainder(pitch + delta.y() * 0.008, 6.283185307179586);
   } else pan += delta;
   refresh();
}
void PlotCanvas::mouseDoubleClickEvent(QMouseEvent *) { fit(); }
void PlotCanvas::wheelEvent(QWheelEvent *event) { zoomBy(event->angleDelta().y() / 120.0); event->accept(); }
void PlotCanvas::paintEvent(QPaintEvent *)
{
   if (renderer) return;
   QPainter painter(this);
   painter.setRenderHint(QPainter::Antialiasing);
   const bool orbit = data->kind == PlotModel::Kind::Orbit;
   const bool ground = data->kind == PlotModel::Kind::GroundTrack;
   const QColor background = orbit ? QColor(12, 18, 30) : QColor(Qt::white);
   const QColor foreground = orbit ? QColor(226, 235, 248) : QColor(25, 35, 45);
   painter.fillRect(rect(), background);
   painter.setPen(foreground);
   painter.drawText(QRectF(10, 4, width()-20, 22), Qt::AlignCenter, data->title);
   QRectF area(orbit ? 35 : 74, 54, width()-(orbit ? 70 : 100), height()-110);
   if (ground && area.width() > 2 * area.height()) { const double w = area.height()*2; area.setLeft(area.center().x()-w/2); area.setWidth(w); }
   if (ground && area.height() > area.width()/2) { const double h = area.width()/2; area.setTop(area.center().y()-h/2); area.setHeight(h); }
   if (area.width() <= 0 || area.height() <= 0) return;
   double xmin = ground ? -180 : std::numeric_limits<double>::infinity();
   double xmax = ground ? 180 : -std::numeric_limits<double>::infinity();
   double ymin = ground ? -90 : std::numeric_limits<double>::infinity();
   double ymax = ground ? 90 : -std::numeric_limits<double>::infinity();
   double extent = 1;
   for (const auto &curve:data->curves) if (curve.visible)
      for (const auto &point:curve.points) extent=std::max(extent,std::hypot(point.x,point.y,point.z)+curve.radius);
   const auto camera=orbitCamera(*data,visibleFrame,yaw,pitch,extent);
   auto project = [&](const PlotPoint &point) {
      if (!orbit) return QPointF(point.x, point.y);
      const osg::Vec3d relative=osg::Vec3d(point.x,point.y,point.z)-camera.target;
      return QPointF(relative*camera.right,relative*camera.up);
   };
   if (!ground) for (const auto &curve : data->curves) {
      if (!curve.visible) continue;
      for (const auto &point : curve.points) {
         // Stable fit across replay: frame selection does not change bounds.
         const auto p = project(point);
         xmin = std::min(xmin, p.x()-curve.radius); xmax = std::max(xmax, p.x()+curve.radius);
         ymin = std::min(ymin, p.y()-curve.radius-point.low); ymax = std::max(ymax, p.y()+curve.radius+point.high);
         extent = std::max(extent, std::hypot(std::hypot(point.x, point.y), point.z)+curve.radius);
      }
   }
   if (orbit) {
      xmin = -camera.extent; xmax = camera.extent; ymin = xmin; ymax = xmax;
      const double aspect = area.width()/area.height();
      if (aspect > 1) { xmin *= aspect; xmax *= aspect; } else { ymin /= aspect; ymax /= aspect; }
   } else if (!ground) {
      if (!std::isfinite(xmin)) { xmin=0; xmax=1; ymin=0; ymax=1; }
      const double dx = xmax == xmin ? std::max(1.0, std::abs(xmin)*0.05) : (xmax-xmin)*0.05;
      const double dy = ymax == ymin ? std::max(1.0, std::abs(ymin)*0.05) : (ymax-ymin)*0.08;
      xmin -= dx; xmax += dx; ymin -= dy; ymax += dy;
   }
   const double cx = (xmin+xmax)/2, cy = (ymin+ymax)/2;
   const double dx = (xmax-xmin)/zoom, dy = (ymax-ymin)/zoom;
   xmin = cx-dx/2 - pan.x()*dx/area.width(); xmax = xmin+dx;
   ymin = cy-dy/2 + pan.y()*dy/area.height(); ymax = ymin+dy;
   auto screen = [&](const QPointF &p) { return QPointF(area.left()+(p.x()-xmin)/(xmax-xmin)*area.width(), area.bottom()-(p.y()-ymin)/(ymax-ymin)*area.height()); };
   painter.save(); painter.setClipRect(area);
   if (ground && !data->map.isNull()) {
      const QPointF a = screen({-180,90}), b = screen({180,-90});
      painter.drawImage(QRectF(a,b), data->map);
   }
   if (data->grid) {
      painter.setPen(QPen(ground ? QColor(255,255,255,100) : orbit ? QColor(90,110,140,75) : QColor(215,222,231), 1));
      const int nx = ground ? data->longitudeLines : 5, ny = ground ? data->latitudeLines : 5;
      for (int i=0;i<=nx;++i) {
         const double x = ground ? -180.0+360.0*i/nx : xmin+(xmax-xmin)*i/nx;
         painter.drawLine(screen({x,ymin}), screen({x,ymax}));
      }
      for (int i=0;i<=ny;++i) {
         const double y = ground ? -90.0+180.0*i/ny : ymin+(ymax-ymin)*i/ny;
         painter.drawLine(screen({xmin,y}), screen({xmax,y}));
      }
   }
   if (orbit && data->axes) {
      const QColor colors[] = {QColor(235,90,80),QColor(100,215,135),QColor(105,150,255)};
      const char *names[] = {"X","Y","Z"};
      for (int i=0;i<3;++i) {
         PlotPoint p; if (i==0) p.x=extent; if (i==1) p.y=extent; if (i==2) p.z=extent;
         const auto end=screen(project(p)); painter.setPen(colors[i]); painter.drawLine(screen(project(PlotPoint{})),end); painter.drawText(end+QPointF(4,-4),names[i]);
      }
   }
   if (orbit) {
      // Paint from far to near so foreground trajectory segments remain visible
      // over the central body, while the far side is occluded by its disk.
      struct Primitive { double depth; const PlotCurve *curve; const PlotPoint *a; const PlotPoint *b; };
      QVector<Primitive> objects;
      auto depth=[&](const PlotPoint &p) { return osg::Vec3d(p.x,p.y,p.z)*camera.outward; };
      for (const auto &curve:data->curves) {
         if (!curve.visible) continue;
         const PlotPoint *previous=nullptr,*last=nullptr;
         for (const auto &point:curve.points) {
            if (point.frame>visibleFrame) continue;
            if (curve.lines && previous && point.connect) objects.append({(depth(*previous)+depth(point))/2,&curve,previous,&point});
            previous=&point; last=&point;
         }
         if (last && curve.showObject) objects.append({depth(*last),&curve,last,nullptr});
      }
      std::stable_sort(objects.begin(),objects.end(),[](const Primitive &a,const Primitive &b) { return a.depth<b.depth; });
      for (const auto &object:objects) {
         const auto &curve=*object.curve;
         painter.setPen(QPen(object.a->color,curve.width,curve.style));
         const auto pixel=screen(project(*object.a));
         if (object.b) painter.drawLine(pixel,screen(project(*object.b)));
         else if (curve.radius>0) {
            const double radius=curve.radius/(xmax-xmin)*area.width();
            QRadialGradient gradient(pixel-QPointF(radius*.3,radius*.3),radius*1.4);
            gradient.setColorAt(0,curve.color.lighter(145)); gradient.setColorAt(1,curve.color.darker(240));
            painter.setPen(curve.color); painter.setBrush(gradient); painter.drawEllipse(pixel,radius,radius);
         } else { painter.setBrush(object.a->color); painter.drawEllipse(pixel,3.5,3.5); }
      }
      if (data->labels) for (const auto &object:objects) if (!object.b) {
         painter.setPen(foreground); painter.drawText(screen(project(*object.a))+QPointF(7,-7),object.curve->name);
      }
   }
   if (!orbit) for (const auto &curve : data->curves) {
      if (!curve.visible) continue;
      const PlotPoint *previous=nullptr, *last=nullptr;
      for (const auto &point : curve.points) {
         if (point.frame > visibleFrame) continue;
         const QPointF position=project(point), pixel=screen(position);
         painter.setPen(curvePen(curve,point.color));
         if (ground && data->footprints) {
            // Match wx's legacy five-degree reference circles; these are not
            // computed sensor coverage or horizon visibility footprints.
            const auto footprint=PlotModel::groundFootprint(position);
            for (int i=1;i<footprint.size();++i)
               for (const auto &segment:PlotModel::groundSegments(footprint[i-1],footprint[i]))
                  painter.drawLine(screen(segment.first),screen(segment.second));
         }
         if (curve.lines && previous && point.connect) {
            if (ground) for (const auto &segment : PlotModel::groundSegments(project(*previous),position)) painter.drawLine(screen(segment.first),screen(segment.second));
            else painter.drawLine(screen(project(*previous)),pixel);
         }
         if (!orbit && !ground && curve.errorBars && (point.high != 0 || point.low != 0)) {
            const auto high=screen({point.x, point.y+point.high}), low=screen({point.x,point.y-point.low});
            painter.drawLine(high,low); painter.drawLine(high-QPointF(3,0),high+QPointF(3,0)); painter.drawLine(low-QPointF(3,0),low+QPointF(3,0));
         }
         if (curve.markers || point.highlighted) drawMarker(painter,pixel,point.marker<0 ? curve.markerType : point.marker,
            curve.markerSize,point.color,point.highlighted);
         previous=&point; last=&point;
      }
      if (last && curve.showObject && (orbit || ground)) {
         const QPointF pixel=screen(project(*last));
         painter.setPen(last->color); painter.setBrush(last->color); painter.drawEllipse(pixel,3.5,3.5);
         if (data->labels) { painter.setPen(orbit ? foreground : last->color); painter.drawText(pixel+QPointF(7,-7),curve.name); }
      }
   }
   for (const auto &station : data->stations) {
      const auto p=screen({station.longitude,station.latitude}); painter.setPen(station.color); painter.setBrush(station.color);
      painter.drawRect(QRectF(p-QPointF(3,3),QSizeF(6,6))); if (data->labels) painter.drawText(p+QPointF(6,-6),station.name);
   }
   painter.restore(); painter.setPen(foreground);
   if (!orbit) {
      painter.drawRect(area);
      for (int i=0;i<=5;++i) {
         const double x=xmin+(xmax-xmin)*i/5, y=ymin+(ymax-ymin)*i/5;
         const double xp=area.left()+area.width()*i/5, yp=area.bottom()-area.height()*i/5;
         painter.drawText(QRectF(xp-38,area.bottom()+4,76,20),Qt::AlignCenter,QString::number(x,'g',5));
         painter.drawText(QRectF(24,yp-10,area.left()-31,20),Qt::AlignRight|Qt::AlignVCenter,QString::number(y,'g',5));
      }
      painter.drawText(QRectF(area.left(),height()-28,area.width(),22),Qt::AlignCenter,data->xLabel);
      painter.save(); painter.translate(1,area.center().y()); painter.rotate(-90); painter.drawText(QRectF(-area.height()/2,0,area.height(),20),Qt::AlignCenter,painter.fontMetrics().elidedText(data->yLabel,Qt::ElideRight,static_cast<int>(area.height()))); painter.restore();
   } else painter.drawText(QRectF(10,height()-28,width()-20,22),Qt::AlignCenter,data->coordinates + " · km");
   if (data->legend) {
      int x=12;
      for (const auto &curve : data->curves) {
         if (!curve.visible) continue;
         painter.setPen(QPen(curve.color,2)); painter.drawLine(x,37,x+15,37);
         painter.setPen(foreground); painter.drawText(x+20,42,curve.name);
         x += 34+painter.fontMetrics().horizontalAdvance(curve.name);
      }
   }
}

PlotWidget::PlotWidget(std::shared_ptr<PlotModel> model, QWidget *parent) : QWidget(parent), data(std::move(model))
{
   historyGeneration=data->historyGeneration;
   auto *layout = new QVBoxLayout(this); layout->setContentsMargins(0,0,0,0);
   auto *bar = new QToolBar(this); layout->addWidget(bar);
   drawing = new PlotCanvas(data,this);
   auto *fit = bar->addAction("Fit"); connect(fit,&QAction::triggered,drawing,&PlotCanvas::fit);
   if (data->kind==PlotModel::Kind::Orbit) {
      auto *scriptView=bar->addAction("Script view");
      scriptView->setToolTip("Restore the scripted camera, tracking and scale");
      connect(scriptView,&QAction::triggered,drawing,&PlotCanvas::scriptView);
      auto *sunlight=bar->addAction("Sunlight"); sunlight->setCheckable(true); sunlight->setChecked(data->sunlight);
      sunlight->setToolTip("Use the mission's Sun position; turn off for lighting from the camera");
      connect(sunlight,&QAction::toggled,this,[this](bool checked) { data->sunlight=checked; drawing->refresh(); });
   }
   auto *save = bar->addAction("Save image…");
   connect(save,&QAction::triggered,this,[this] {
      const auto path=QFileDialog::getSaveFileName(this,"Save plot image",data->title+".png","PNG image (*.png)");
      if (path.isEmpty()) return;
      QSaveFile file(path);
      if (!file.open(QIODevice::WriteOnly) || !drawing->captureImage().save(&file,"PNG") || !file.commit())
         QMessageBox::warning(this,"Could not save plot",file.errorString());
   });
   timeline = new QSlider(Qt::Horizontal,this); timeline->setRange(0,1000); timeline->setValue(1000);
   timeline->hide();
   timer = new QTimer(this); timer->setInterval(30);
   if (data->kind != PlotModel::Kind::XY) {
      replay=bar->addAction("Replay"); replay->setCheckable(true);
      connect(replay,&QAction::toggled,this,[this](bool checked) { if (checked) { timeline->setValue(0); timer->start(); } else timer->stop(); });
      connect(timer,&QTimer::timeout,this,[this] { timeline->setValue(std::min(1000,timeline->value()+10)); if (timeline->value()==1000) replay->setChecked(false); });
      bar->addWidget(timeline);
      timeline->show();
   }
   connect(timeline,&QSlider::valueChanged,this,[this] { updateReplayFrame(); });
   layout->addWidget(drawing,1);
}
void PlotWidget::updateReplayFrame()
{
   quint64 first=data->frame;
   for (const auto &curve:data->curves) if (!curve.points.empty()) first=std::min(first,curve.points.front().frame);
   const int value=timeline->value();
   drawing->setFrame(value==1000 ? std::numeric_limits<quint64>::max() : first+static_cast<quint64>((data->frame-first)*(value/1000.0)));
}
void PlotWidget::refresh()
{
   if (historyGeneration!=data->historyGeneration) {
      historyGeneration=data->historyGeneration;
      timer->stop();
      if (replay) replay->setChecked(false);
      timeline->setValue(1000);
   }
   // The retained interval moves as MaxPlotPoints evicts old samples. Resolve
   // the slider against that interval on every update, not only mouse movement.
   updateReplayFrame();
}
