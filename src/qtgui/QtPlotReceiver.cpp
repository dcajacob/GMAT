#include "QtPlotReceiver.hpp"
#include "CameraAlignment.hpp"
#include "TableColumns.hpp"
#include "PlotWidget.hpp"
#include "BodyFixedPoint.hpp"
#include "CelestialBody.hpp"
#include "Spacecraft.hpp"
#include "SolarSystem.hpp"
#include "FileManager.hpp"
#include "CoordinateConverter.hpp"
#include "Moderator.hpp"
#include "MessageInterface.hpp"
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTableWidget>
#include <QHeaderView>
#include <QFileInfo>
#include <algorithm>
#include <cmath>

namespace {
QString text(const std::string &value) { return QString::fromStdString(value); }
QColor rgb(UnsignedInt color) { return QColor::fromRgb(static_cast<QRgb>(color & 0xffffff)); }
constexpr double degrees = 57.2957795130823208768;
}
void QtPlotReceiver::validateCameraReferences(const QMap<QString,QtCameraSetting> &settings)
{
   auto *moderator=Moderator::Instance();
   for (auto it=settings.cbegin();it!=settings.cend();++it) {
      if (it->bodyRelative || it->lookAtRotation) {
         auto *plot=moderator->GetConfiguredObject(it.key().toStdString());
         if (!plot || !plot->IsOfType("OrbitView") || (it->bodyRelative && plot->GetStringParameter("ViewPointRefType")=="Vector") ||
             plot->GetStringParameter("ViewPointVectorType")!="Vector")
            throw std::runtime_error((it.key()+": relative camera needs a vector ViewPointVector; body-relative mode also needs an object ViewPointReference").toStdString());
      }
      for (const auto &view:it->views) {
      for (const auto &name:{view.reference,view.target}) {
         if (name.isEmpty() || name=="CoordinateSystem") continue;
         auto *object=moderator->GetConfiguredObject(name.toStdString());
         if (!object) object=moderator->GetSolarSystemInUse()->GetBody(name.toStdString());
         if (!dynamic_cast<SpacePoint *>(object)) throw std::runtime_error((it.key()+" camera "+view.name+": unknown space point "+name).toStdString());
      }
      }
   }
}
QtPlotReceiver::QtPlotReceiver(QMdiArea *area) : workspace(area) {}
QtPlotReceiver::~QtPlotReceiver() { changed = {}; clear(); }
QtPlotReceiver::Entry *QtPlotReceiver::find(const std::string &name)
{
   auto it=entries.find(text(name)); return it==entries.end() ? nullptr : &it.value();
}
void QtPlotReceiver::clear()
{
   for (auto &entry : entries) if (entry.window) delete entry.window.data();
   entries.clear(); warnings.clear(); if (changed) changed();
}
QStringList QtPlotReceiver::names() const { return entries.keys(); }
std::shared_ptr<const PlotModel> QtPlotReceiver::model(const QString &name) const
{
   auto it=entries.constFind(name); return it==entries.cend() ? nullptr : it->data;
}
bool QtPlotReceiver::show(const QString &name)
{
   auto it=entries.find(name); if (it==entries.end()) return false;
   auto &entry=it.value();
   if (!entry.window) {
      if (entry.data->kind==PlotModel::Kind::Table) {
         entry.table=new QTableWidget;
         configureTableColumns(entry.table);
         entry.table->setEditTriggers(QAbstractItemView::NoEditTriggers);
         entry.table->setRowCount(entry.cells.size());
         int columns=0; for (const auto &row:entry.cells) columns=std::max(columns,static_cast<int>(row.size()));
         entry.table->setColumnCount(columns);
         for (int r=0;r<entry.cells.size();++r) for (int c=0;c<entry.cells[r].size();++c) {
            const auto &cell=entry.cells[r][c];
            auto *item=new QTableWidgetItem(cell.text);
            item->setForeground(cell.foreground); item->setBackground(cell.background);
            entry.table->setItem(r,c,item);
         }
         fitTableColumns(entry.table);
         entry.window=workspace->addSubWindow(entry.table);
      } else {
         entry.widget=new PlotWidget(entry.data);
         if (saveProjection) entry.widget->setProjectionSaver([this,name](bool perspective,double fov) { return saveProjection(name,perspective,fov); });
         entry.widget->setFocusPolicy(Qt::StrongFocus);
         entry.window=workspace->addSubWindow(entry.widget);
      }
      entry.window->setAttribute(Qt::WA_DeleteOnClose);
      entry.window->setWindowTitle(name);
      entry.window->setProperty("plotName",name);
      entry.window->resize(700,480);
   }
   // show() alone leaves a minimized MDI child minimized. Activation also
   // needs an explicit raise when another child covers this plot.
   if (entry.window->isMinimized()) entry.window->showNormal();
   else entry.window->show();
   workspace->setActiveSubWindow(entry.window);
   entry.window->raise();
   entry.window->widget()->setFocus(); return true;
}
QtPlotReceiver::Entry &QtPlotReceiver::create(const std::string &name, PlotModel::Kind kind,
      Real x, Real y, Real w, Real h, bool maximized)
{
   remove(name);
   auto &entry=entries[text(name)]; entry.data=std::make_shared<PlotModel>(kind); entry.data->title=text(name);
   if (kind==PlotModel::Kind::Orbit && cameraSettings.contains(text(name))) {
      const auto setting=cameraSettings.value(text(name));
      entry.data->perspective=setting.perspective; entry.data->fieldOfView=setting.fieldOfView;
      if (!setting.views.isEmpty()) {
         entry.data->cameraViews.append({setting.primaryName.isEmpty() ? QString("Script camera") : setting.primaryName,setting.perspective,setting.fieldOfView,{}});
         for (const auto &view:setting.views) entry.data->cameraViews.append({view.name,view.perspective,view.fieldOfView,{}});
      }
   }
   show(text(name));
   const auto bounds=workspace->viewport()->rect();
   const int width=std::clamp(static_cast<int>((w>0 && w<=1 ? w : .6)*bounds.width()),200,std::max(200,bounds.width()));
   const int height=std::clamp(static_cast<int>((h>0 && h<=1 ? h : .6)*bounds.height()),160,std::max(160,bounds.height()));
   const int left=std::clamp(static_cast<int>((std::isfinite(x) ? std::clamp(x,0.0,1.0) : 0)*bounds.width()),0,std::max(0,bounds.width()-width));
   const int top=std::clamp(static_cast<int>((std::isfinite(y) ? std::clamp(y,0.0,1.0) : 0)*bounds.height()),0,std::max(0,bounds.height()-height));
   entry.window->showNormal(); entry.window->setGeometry(left,top,width,height);
   if (maximized) entry.window->showMaximized();
   // Restoring normal geometry can reactivate the formerly maximized script.
   // Activate the plot only after its final window state has been applied.
   workspace->setActiveSubWindow(entry.window);
   entry.window->raise();
   entry.window->widget()->setFocus();
   if (changed) changed(); return entry;
}
void QtPlotReceiver::refresh(Entry &entry, bool force)
{
   if (++entry.data->pendingUpdates >= entry.data->updateFrequency || force) {
      entry.data->pendingUpdates=0; if (entry.widget) entry.widget->refresh();
   }
}
bool QtPlotReceiver::remove(const std::string &name)
{
   auto it=entries.find(text(name)); if (it==entries.end()) return false;
   if (it->window) delete it->window.data(); entries.erase(it); if (changed) changed(); return true;
}
void QtPlotReceiver::warn(const std::string &name, const std::string &option)
{
   const QString key=text(name+":"+option); if (warnings.contains(key)) return;
   warnings.insert(key); MessageInterface::ShowMessage("Qt plot '%s': %s is not yet supported.\n",name.c_str(),option.c_str());
}
bool QtPlotReceiver::CreateGlPlotWindow(const std::string &name,const std::string &oldName,Real x,Real y,Real w,Real h,bool maximized,Integer)
{
   if (oldName!=name && !oldName.empty()) remove(oldName);
   auto &entry=create(name,currentView==GmatPlot::GROUND_TRACK_PLOT ? PlotModel::Kind::GroundTrack : PlotModel::Kind::Orbit,x,y,w,h,maximized);
   if (entry.data->kind==PlotModel::Kind::GroundTrack) { entry.data->xLabel="Longitude (deg)"; entry.data->yLabel="Latitude (deg)"; }
   return true;
}
void QtPlotReceiver::SetGlSolarSystem(const std::string &name, SolarSystem *solarSystem)
{
   if (auto *entry=find(name)) entry->solarSystem=solarSystem;
}
void QtPlotReceiver::SetGlObject(const std::string &name,const StringArray &names,const std::vector<SpacePoint *> &points)
{
   auto *entry=find(name); if (!entry) return;
   entry->objects=names; entry->points=points;
   for (size_t i=0;i<names.size();++i) {
      auto &curve=entry->data->curves[static_cast<int>(i)]; curve.name=text(names[i]);
      if (i<points.size() && points[i]) {
         curve.color=rgb(points[i]->GetCurrentOrbitColor());
         if (auto *body=dynamic_cast<CelestialBody *>(points[i])) {
            curve.radius=body->GetEquatorialRadius();
            curve.texturePath=text(body->GetStringParameter(body->GetParameterID("TextureMapFullPath")));
            if (!curve.texturePath.isEmpty()) curve.texturePath=QFileInfo(curve.texturePath).absoluteFilePath();
            curve.modelPath=text(body->GetStringParameter(body->GetParameterID("3DModelFileFullPath")));
            if (!curve.modelPath.isEmpty()) curve.modelPath=QFileInfo(curve.modelPath).absoluteFilePath();
            for (int axis=0;axis<3;++axis) {
               const std::string suffix(1,"XYZ"[axis]);
               curve.modelOffset[axis]=body->GetRealParameter(body->GetParameterID("3DModelOffset"+suffix));
               curve.modelRotation[axis]=body->GetRealParameter(body->GetParameterID("3DModelRotation"+suffix));
            }
            curve.modelScale=body->GetRealParameter(body->GetParameterID("3DModelScale"));
         }
         if (auto *spacecraft=dynamic_cast<Spacecraft *>(points[i])) {
            curve.modelPath=text(spacecraft->GetModelFileFullPath());
            if (!curve.modelPath.isEmpty()) curve.modelPath=QFileInfo(curve.modelPath).absoluteFilePath();
            const char *axes[]={"X","Y","Z"};
            for (int axis=0;axis<3;++axis) {
               curve.modelOffset[axis]=spacecraft->GetRealParameter(spacecraft->GetParameterID(std::string("ModelOffset")+axes[axis]));
               curve.modelRotation[axis]=spacecraft->GetRealParameter(spacecraft->GetParameterID(std::string("ModelRotation")+axes[axis]));
            }
            curve.modelScale=spacecraft->GetRealParameter(spacecraft->GetParameterID("ModelScale"));
         }
      }
   }
}
void QtPlotReceiver::SetGlCoordSystem(const std::string &name,CoordinateSystem *internal,CoordinateSystem *view,CoordinateSystem *viewUp)
{
   if (auto *entry=find(name)) { entry->internal=internal; entry->view=view; entry->viewUp=viewUp; entry->data->coordinates=view ? text(view->GetName()) : QString(); }
}
void QtPlotReceiver::SetGl2dDrawingOption(const std::string &name,const std::string &,const std::string &map,Integer footprint)
{
   SetGroundTrackOption(name,"TextureMap",map);
   if (auto *entry=find(name)) entry->data->footprints=footprint!=0;
}
void QtPlotReceiver::SetGl3dDrawingOption(const std::string &name,bool labels,bool ec,bool xy,bool wire,bool axes,bool grid,bool sun,bool,bool,bool stars,bool constellations,Integer count)
{
   if (auto *entry=find(name)) {
      auto &data=*entry->data;
      data.labels=labels; data.axes=axes; data.grid=grid;
      data.xyPlane=xy; data.eclipticPlane=ec; data.wireframe=wire; data.sunLine=sun;
      data.constellationsEnabled=constellations;
      if (constellations && !data.constellationCatalogLoaded) {
         data.constellationCatalogLoaded=true;
         try {
            data.constellationCatalog=ConstellationCatalog::read(text(FileManager::Instance()->FindPath("","CONSTELLATION_FILE",true,false,true)));
         } catch (BaseException &error) { data.constellationCatalog.error=text(error.GetFullMessage()); }
         if (!data.constellationCatalog.error.isEmpty())
            MessageInterface::ShowMessage("Qt OrbitView '%s': cannot load constellations: %s\n",name.c_str(),data.constellationCatalog.error.toStdString().c_str());
      }
      data.starsEnabled=stars; data.starCount=static_cast<int>(std::clamp<Integer>(count,0,std::numeric_limits<int>::max()));
      if (stars && !data.starCatalogLoaded) {
         data.starCatalogLoaded=true;
         try {
            const auto path=FileManager::Instance()->FindPath("","STAR_FILE",true,false,true);
            data.starCatalog=StarCatalog::read(text(path));
         } catch (BaseException &error) { data.starCatalog.error=text(error.GetFullMessage()); }
         if (!data.starCatalog.error.isEmpty())
            MessageInterface::ShowMessage("Qt OrbitView '%s': cannot load star catalog: %s\n",name.c_str(),data.starCatalog.error.toStdString().c_str());
         else if (data.starCatalog.rejectedLines)
            MessageInterface::ShowMessage("Qt OrbitView '%s': skipped %d invalid star catalog lines.\n",name.c_str(),data.starCatalog.rejectedLines);
      }
   }
}
void QtPlotReceiver::SetGl3dViewOption(const std::string &name,SpacePoint *reference,SpacePoint *position,SpacePoint *direction,Real scale,
      const Rvector3 &referenceVector,const Rvector3 &positionVector,const Rvector3 &directionVector,const std::string &upAxis,
      bool referenceIsVector,bool positionIsVector,bool directionIsVector)
{
   if (auto *entry=find(name)) {
      entry->cameraReference=reference; entry->cameraPosition=position; entry->cameraDirection=direction;
      entry->referenceIsVector=referenceIsVector; entry->positionIsVector=positionIsVector; entry->directionIsVector=directionIsVector;
      entry->cameraScale=scale; entry->upVector={0,0,0};
      const auto axis=upAxis.empty() ? 'Z' : upAxis.back(); const int index=axis=='X' ? 0 : axis=='Y' ? 1 : 2;
      entry->upVector[index]=!upAxis.empty() && upAxis.front()=='-' ? -1 : 1;
      if (const auto setting=cameraSettings.value(text(name));setting.up) entry->upVector=*setting.up;
      for (int i=0;i<3;++i) { entry->referenceVector[i]=referenceVector[i]; entry->positionVector[i]=positionVector[i]; entry->directionVector[i]=directionVector[i]; }
      entry->data->scriptedCamera=true;
      if (entry->widget) entry->widget->canvas()->scriptView();
   }
}
void QtPlotReceiver::SetGlDrawOrbitFlag(const std::string &name,const std::vector<bool> &flags)
{
   if (auto *entry=find(name)) for (size_t i=0;i<flags.size();++i) entry->data->curves[static_cast<int>(i)].lines=flags[i];
}
void QtPlotReceiver::SetGlShowObjectFlag(const std::string &name,const std::vector<bool> &flags)
{
   if (auto *entry=find(name)) for (size_t i=0;i<flags.size();++i) entry->data->curves[static_cast<int>(i)].visible=flags[i];
}
void QtPlotReceiver::SetGlUpdateFrequency(const std::string &name,Integer frequency) { if (auto *entry=find(name)) entry->data->updateFrequency=std::max(1,static_cast<int>(frequency)); }
bool QtPlotReceiver::IsThere(const std::string &name) { auto *entry=find(name); return entry && entry->window && entry->window->isVisible(); }
bool QtPlotReceiver::InitializeGlPlot(const std::string &name) { return find(name)!=nullptr; }
bool QtPlotReceiver::RefreshGlPlot(const std::string &name) { if (auto *entry=find(name)) { refresh(*entry,true); return true; } return false; }
bool QtPlotReceiver::DeleteGlPlot(const std::string &name) { return remove(name); }
bool QtPlotReceiver::SetGlEndOfRun(const std::string &name) { return RefreshGlPlot(name); }
void QtPlotReceiver::SetMaxGlDataPoints(const std::string &name,Integer count) { if (auto *entry=find(name)) { entry->data->maxPoints=static_cast<int>(std::clamp<Integer>(count,2,std::numeric_limits<int>::max())); entry->data->trim(); } }
bool QtPlotReceiver::UpdateGlPlot(const std::string &name,const std::string &,const StringArray &names,const Real &epoch,
      const RealArray &x,const RealArray &y,const RealArray &z,const RealArray &,const RealArray &,const RealArray &,
      const ColorMap &colors,const ColorMap &targetColors,bool solving,Integer,bool update,bool drawing,bool)
{
   auto *entry=find(name); if (!entry) return false;
   auto &data=*entry->data; ++data.frame;
   if (data.kind==PlotModel::Kind::Orbit && data.scriptedCamera) {
      auto resolve=[&](bool vector,SpacePoint *object,const std::array<double,3> &value) {
         if (vector) return value;
         if (!object) throw std::runtime_error("Missing camera reference object");
         const auto found=std::find(names.begin(),names.end(),object->GetName());
         if (found!=names.end()) {
            const auto index=static_cast<size_t>(found-names.begin());
            if (index<x.size() && index<y.size() && index<z.size()) return std::array<double,3>{x[index],y[index],z[index]};
         }
         auto state=object->GetMJ2000State(epoch);
         if (entry->internal && entry->view && entry->internal!=entry->view) {
            Rvector6 converted; CoordinateConverter converter;
            converter.Convert(epoch,state,entry->internal,converted,entry->view); state=converted;
         }
         return std::array<double,3>{state[0],state[1],state[2]};
      };
      const auto settings=cameraSettings.value(text(name));
      auto transform=[&](SpacePoint *object,const std::array<double,3> &origin,const std::array<double,3> &target,bool aligned,bool shortest) {
         Rmatrix33 frame;
         if (object) {
            Rmatrix33 viewToBase;
            if (entry->view) {
               entry->view->ToBaseSystem(A1Mjd(epoch),Rvector6(),true);
               viewToBase=entry->view->GetLastRotationMatrix();
            }
            auto bodyToBase=object->GetAttitude(epoch);
            if (object->IsOfType(Gmat::SPACECRAFT)) bodyToBase=bodyToBase.Transpose();
            frame=viewToBase.Transpose()*bodyToBase;
         }
         const auto direction=frame.Transpose()*Rvector3(target[0]-origin[0],target[1]-origin[1],target[2]-origin[2]);
         const auto alignment=aligned ? cameraAlignment(osg::Vec3d(direction[0],direction[1],direction[2]),shortest) : osg::Quat();
         return [frame,alignment](const std::array<double,3> &vector) {
            const auto local=alignment*osg::Vec3d(vector[0],vector[1],vector[2]);
            const auto world=frame*Rvector3(local.x(),local.y(),local.z());
            return std::array<double,3>{world[0],world[1],world[2]};
         };
      };
      try {
         PlotCamera camera; camera.frame=data.frame; camera.solver=solving;
         const auto reference=resolve(entry->referenceIsVector,entry->cameraReference,entry->referenceVector);
         const auto target=resolve(entry->directionIsVector,entry->cameraDirection,entry->directionVector);
         if ((settings.bodyRelative || settings.lookAtRotation) && !entry->positionIsVector)
            throw std::runtime_error("Relative camera position must be a vector");
         if (settings.bodyRelative && !entry->cameraReference) throw std::runtime_error("Missing body-relative camera reference");
         const auto orient=transform(settings.bodyRelative ? entry->cameraReference : nullptr,reference,target,settings.lookAtRotation,settings.shortestAngle);
         const auto position=orient(resolve(entry->positionIsVector,entry->cameraPosition,entry->positionVector));
         camera.target=settings.lookAtRotation ? reference : target;
         const auto orientedUp=orient(entry->upVector);
         Rvector3 up(orientedUp[0],orientedUp[1],orientedUp[2]);
         if (!settings.bodyRelative && !settings.lookAtRotation && entry->viewUp && entry->view && entry->viewUp!=entry->view) {
            entry->viewUp->ToBaseSystem(A1Mjd(epoch),Rvector6(),true);
            const auto upToBase=entry->viewUp->GetLastRotationMatrix();
            entry->view->ToBaseSystem(A1Mjd(epoch),Rvector6(),true);
            up=entry->view->GetLastRotationMatrix().Transpose()*upToBase*up;
         }
         if (settings.centerOffset) {
            const auto offset=orient(*settings.centerOffset);
            for (int i=0;i<3;++i) camera.target[i]+=offset[i];
         }
         for (int i=0;i<3;++i) {
            camera.eye[i]=reference[i]+entry->cameraScale*position[i]; camera.up[i]=up[i];
            if (!std::isfinite(camera.eye[i]) || !std::isfinite(camera.target[i]) || !std::isfinite(camera.up[i]))
               throw std::runtime_error("Nonfinite camera coordinates");
         }
         if (std::hypot(camera.eye[0]-camera.target[0],camera.eye[1]-camera.target[1],camera.eye[2]-camera.target[2])<1e-9)
            throw std::runtime_error("Camera eye and target coincide");
         data.cameras.push_back(camera);
         while (data.cameras.size()>static_cast<size_t>(data.maxPoints)) data.cameras.pop_front();
      } catch (BaseException &) { warn(name,"unresolved scripted camera (using available camera or manual view)"); }
      catch (const std::exception &) { warn(name,"invalid scripted camera (using available camera or manual view)"); }
      for (int index=0;index<settings.views.size();++index) {
         const auto &view=settings.views[index];
         try {
            auto findObject=[&](const QString &objectName) -> SpacePoint * {
               if (objectName.isEmpty() || objectName=="CoordinateSystem") return nullptr;
               SpacePoint *object=nullptr;
               for (auto *point:entry->points) if (point && text(point->GetName())==objectName) { object=point; break; }
               if (!object && entry->solarSystem) object=entry->solarSystem->GetBody(objectName.toStdString());
               if (!object) object=dynamic_cast<SpacePoint *>(Moderator::Instance()->GetInternalObject(objectName.toStdString()));
               return object;
            };
            auto position=[&](const QString &objectName) {
               if (objectName.isEmpty() || objectName=="CoordinateSystem") return std::array<double,3>{};
               return resolve(false,findObject(objectName),{});
            };
            const auto origin=position(view.reference);
            const auto target=view.target.isEmpty() ? origin : position(view.target);
            auto *reference=view.bodyRelative ? findObject(view.reference) : nullptr;
            if (view.bodyRelative && !reference) throw std::runtime_error("Missing body-relative camera reference");
            const auto orient=transform(reference,origin,target,view.lookAtRotation,view.shortestAngle);
            const auto eye=orient(view.eye),center=orient(view.center);
            PlotCamera camera; camera.frame=data.frame; camera.solver=solving;
            camera.up=orient(view.up);
            for (int axis=0;axis<3;++axis) {
               camera.eye[axis]=origin[axis]+eye[axis];
               camera.target[axis]=(view.lookAtRotation ? origin[axis] : target[axis])+center[axis];
               if (!std::isfinite(camera.eye[axis]) || !std::isfinite(camera.target[axis]) || !std::isfinite(camera.up[axis])) throw std::runtime_error("Nonfinite camera position");
            }
            if (std::hypot(camera.eye[0]-camera.target[0],camera.eye[1]-camera.target[1],camera.eye[2]-camera.target[2])<1e-9)
               throw std::runtime_error("Camera eye and target coincide");
            auto &history=data.cameraViews[index+1].cameras;
            history.push_back(camera);
            while (history.size()>static_cast<size_t>(data.maxPoints)) history.pop_front();
         } catch (...) { warn(name,("unresolved camera "+view.name+" (using its available history or manual view)").toStdString()); }
      }
   }
   Rvector6 sunState;
   const bool hasSun=data.kind==PlotModel::Kind::Orbit && entry->solarSystem && entry->internal && entry->view;
   if (hasSun) {
      sunState=entry->solarSystem->GetBody("Sun")->GetMJ2000State(epoch);
      if (entry->internal!=entry->view) {
         Rvector6 converted; CoordinateConverter converter;
         converter.Convert(epoch,sunState,entry->internal,converted,entry->view); sunState=converted;
      }
   }
   if (!entry->ignoreTimeSequence && data.frame>1 && epoch<data.lastEpoch) data.breakLines();
   for (size_t i=0;i<entry->objects.size();++i) {
      const auto &object=entry->objects[i];
      auto &curve=data.curves[static_cast<int>(i)];
      const auto &palette=solving ? targetColors : colors;
      const auto color=palette.find(object); if (color!=palette.end()) curve.color=rgb(color->second);
      auto found=std::find(names.begin(),names.end(),object);
      double px=0,py=0,pz=0;
      if (found!=names.end()) {
         size_t index=static_cast<size_t>(found-names.begin());
         if (index>=x.size() || index>=y.size() || index>=z.size()) { curve.breakNext=true; continue; }
         // OrbitPlot::BufferOrbitData already converted these into view CS.
         px=x[index]; py=y[index]; pz=z[index];
      } else if (i<entry->points.size() && entry->points[i] && !entry->points[i]->IsOfType(Gmat::SPACECRAFT)) {
         auto state=entry->points[i]->GetMJ2000State(epoch);
         if (entry->internal && entry->view && entry->internal!=entry->view) {
            Rvector6 converted; CoordinateConverter converter;
            converter.Convert(epoch,state,entry->internal,converted,entry->view); state=converted;
         }
         px=state[0]; py=state[1]; pz=state[2];
      } else { curve.breakNext=true; continue; }
      if (data.kind==PlotModel::Kind::GroundTrack) {
         if (curve.radius>0 || (px==0 && py==0 && pz==0)) continue;
         const double lon=std::atan2(py,px)*degrees, lat=std::atan2(pz,std::hypot(px,py))*degrees;
         data.append(static_cast<int>(i),lon,lat,0,epoch,drawing,solving);
      } else {
         data.append(static_cast<int>(i),px,py,pz,epoch,drawing,solving);
         if (!curve.points.empty() && curve.points.back().frame==data.frame) {
            curve.points.back().hasSun=hasSun;
            for (int axis=0;axis<3;++axis) curve.points.back().sunPosition[axis]=sunState[axis];
         }
         if (!curve.points.empty() && curve.points.back().frame==data.frame &&
             i<entry->points.size() && entry->points[i]) {
            // Celestial-body GetAttitude returns body-fixed to MJ2000Eq.
            // The coordinate system rotation maps plot coordinates to that base.
            Rmatrix33 viewToBase;
            if (entry->view) {
               entry->view->ToBaseSystem(A1Mjd(epoch),Rvector6(),true);
               viewToBase=entry->view->GetLastRotationMatrix();
            }
            auto attitude=entry->points[i]->GetAttitude(epoch);
            // Spacecraft attitude maps inertial to body, unlike celestial bodies.
            if (entry->points[i]->IsOfType(Gmat::SPACECRAFT)) attitude=attitude.Transpose();
            const auto rotation=viewToBase.Transpose()*attitude;
            for (int row=0;row<3;++row) for (int col=0;col<3;++col) {
               curve.points.back().bodyToView[row*3+col]=rotation(row,col);
               curve.points.back().inertialToView[row*3+col]=viewToBase(col,row);
            }
         }
      }
   }
   if (update) refresh(*entry,true); return true;
}
bool QtPlotReceiver::TakeGlAction(const std::string &name,const std::string &action)
{
   auto *entry=find(name); if (!entry) return false;
   if (action=="PenUp") { entry->data->penDown=false; entry->data->breakLines(); }
   else if (action=="PenDown") entry->data->penDown=true;
   else if (action=="ClearObjects") { entry->objects.clear(); entry->points.clear(); entry->data->curves.clear(); entry->data->cameras.clear(); for (auto &view:entry->data->cameraViews) view.cameras.clear(); }
   else if (action=="ClearSolverData") {
      auto clearSolver=[](std::deque<PlotCamera> &cameras) {
         cameras.erase(std::remove_if(cameras.begin(),cameras.end(),[](const PlotCamera &camera) { return camera.solver; }),cameras.end());
      };
      clearSolver(entry->data->cameras);
      for (auto &view:entry->data->cameraViews) clearSolver(view.cameras);
      for (auto &curve:entry->data->curves) {
         curve.points.erase(std::remove_if(curve.points.begin(),curve.points.end(),[](const PlotPoint &point) { return point.solver; }),curve.points.end());
         curve.breakNext=true;
      }
   } else if (action=="IgnoreTimeSequence") entry->ignoreTimeSequence=true;
   else if (action=="ClearData") entry->data->clear();
   else { warn(name,action); return false; }
   refresh(*entry,true); return true;
}

bool QtPlotReceiver::CreateXyPlotWindow(const std::string &name,const std::string &oldName,Real x,Real y,Real w,Real h,bool maximized,const std::string &title,const std::string &xlabel,const std::string &ylabel,bool grid,bool)
{
   if (oldName!=name && !oldName.empty()) remove(oldName);
   auto &entry=create(name,PlotModel::Kind::XY,x,y,w,h,maximized);
   entry.data->title=text(title.empty() ? name : title); entry.data->xLabel=text(xlabel); entry.data->yLabel=text(ylabel); entry.data->grid=grid; return true;
}
bool QtPlotReceiver::DeleteXyPlot(const std::string &name) { return remove(name); }
bool QtPlotReceiver::AddXyPlotCurve(const std::string &name,int index,const std::string &title,UnsignedInt color)
{
   auto *entry=find(name); if (!entry || index<0) return false;
   auto &curve=entry->data->curves[index]; curve.name=text(title); curve.color=rgb(color); curve.markerType=index%10; return true;
}
bool QtPlotReceiver::DeleteAllXyPlotCurves(const std::string &name,const std::string &) { if (auto *entry=find(name)) { entry->data->curves.clear(); return true; } return false; }
bool QtPlotReceiver::DeleteXyPlotCurve(const std::string &name,int index) { if (auto *entry=find(name)) return entry->data->curves.remove(index)>0; return false; }
void QtPlotReceiver::ClearXyPlotData(const std::string &name) { if (auto *entry=find(name)) { entry->data->clear(); refresh(*entry,true); } }
void QtPlotReceiver::XyPlotPenUp(const std::string &name) { if (auto *entry=find(name)) { entry->data->penDown=false; entry->data->breakLines(); } }
void QtPlotReceiver::XyPlotPenDown(const std::string &name) { if (auto *entry=find(name)) entry->data->penDown=true; }
void QtPlotReceiver::XyPlotDarken(const std::string &name,Integer factor,Integer index,Integer curve)
{
   curves(name,curve,[&](PlotCurve &c) { for (size_t i=0;i<c.points.size();++i) if (index<0 || static_cast<Integer>(i)>=index) c.points[i].color=c.points[i].color.darker(std::max(100,static_cast<int>(factor))); });
}
void QtPlotReceiver::XyPlotLighten(const std::string &name,Integer factor,Integer index,Integer curve)
{
   curves(name,curve,[&](PlotCurve &c) { for (size_t i=0;i<c.points.size();++i) if (index<0 || static_cast<Integer>(i)>=index) c.points[i].color=c.points[i].color.lighter(std::max(100,static_cast<int>(factor))); });
}
void QtPlotReceiver::XyPlotMarkPoint(const std::string &name,Integer index,Integer curve) {
   curves(name,curve,[&](PlotCurve &c) {
      const auto position=index<0 ? static_cast<Integer>(c.points.size())-1 : index;
      if (position>=0 && position<static_cast<Integer>(c.points.size())) c.points[position].highlighted=true;
   });
}
void QtPlotReceiver::XyPlotMarkBreak(const std::string &name,Integer index,Integer curve)
{
   curves(name,curve,[&](PlotCurve &c) {
      const size_t position=index<0 ? c.points.size() : std::min(c.points.size(),static_cast<size_t>(index));
      const auto frame=position<c.points.size() ? c.points[position].frame : c.points.empty() ? 0 : c.points.back().frame+1;
      if (!c.breaks.contains(frame)) c.breaks.append(frame);
      c.breakNext=true;
   });
}
void QtPlotReceiver::XyPlotClearFromBreak(const std::string &name,Integer which,Integer end,Integer curve)
{
   curves(name,curve,[&](PlotCurve &c) {
      if (c.breaks.isEmpty()) return;
      const int index=which<0 ? c.breaks.size()-1 : static_cast<int>(which);
      if (index<0 || index>=c.breaks.size()) return;
      const auto frame=c.breaks[index];
      const auto last=end>=0 && end<c.breaks.size() ? c.breaks[end] : std::numeric_limits<quint64>::max();
      if (last<frame) return;
      c.points.erase(std::remove_if(c.points.begin(),c.points.end(),[&](const PlotPoint &point) { return point.frame>=frame && point.frame<last; }),c.points.end());
      for (auto &point:c.points) if (point.frame>=last) { point.connect=false; break; }
      if (end<0) c.breaks.resize(index+1); // Retain the anchor for the next solver iteration.
      else if (end>index) c.breaks.remove(index+1,end-index);
      c.breakNext=true;
   });
}
void QtPlotReceiver::XyPlotChangeColor(const std::string &name,Integer index,UnsignedInt color,Integer curve)
{
   curves(name,curve,[&](PlotCurve &c) { c.color=rgb(color); if (index>=0) for (size_t i=index;i<c.points.size();++i) c.points[i].color=c.color; });
}
void QtPlotReceiver::XyPlotChangeMarker(const std::string &name,Integer index,Integer marker,int curve) {
   curves(name,curve,[&](PlotCurve &c) {
      const int type=marker<0 ? c.markerType : marker%10;
      for (size_t i=std::max<Integer>(0,index);i<c.points.size();++i) c.points[i].marker=type;
      c.markerType=type;
   });
}
void QtPlotReceiver::XyPlotChangeWidth(const std::string &name,Integer,Integer width,int curve) { curves(name,curve,[&](PlotCurve &c) { c.width=std::clamp(static_cast<int>(width),1,20); }); }
void QtPlotReceiver::XyPlotChangeStyle(const std::string &name,Integer,Integer style,int curve) { curves(name,curve,[&](PlotCurve &c) { if (style>=0) c.lineStyle=style; }); }
void QtPlotReceiver::XyPlotRescale(const std::string &name) { if (auto *entry=find(name)) if (entry->widget) entry->widget->canvas()->fit(); }
void QtPlotReceiver::XyPlotCurveSettings(const std::string &name,bool lines,Integer width,Integer style,bool markers,Integer size,Integer marker,bool hiLo,Integer curve)
{
   curves(name,curve,[&](PlotCurve &c) {
      c.lines=lines; c.width=std::clamp(static_cast<int>(width),1,20); if (style>=0) c.lineStyle=style;
      c.markers=markers; c.markerSize=std::clamp(static_cast<int>(size),1,30); c.errorBars=hiLo;
      if (marker>=0) c.markerType=marker%10;
      for (auto &point:c.points) point.marker=c.markerType;
   });
}
void QtPlotReceiver::SetXyPlotTitle(const std::string &name,const std::string &title) { if (auto *entry=find(name)) entry->data->title=text(title); }
void QtPlotReceiver::ShowXyPlotLegend(const std::string &name) { if (auto *entry=find(name)) entry->data->legend=true; }
bool QtPlotReceiver::RefreshXyPlot(const std::string &name) { return RefreshGlPlot(name); }
bool QtPlotReceiver::UpdateXyPlot(const std::string &name,const std::string &,const Real &x,const Rvector &ys,const std::string &title,const std::string &xlabel,const std::string &ylabel,bool update,bool grid)
{
   auto *entry=find(name); if (!entry) return false;
   if (!title.empty()) entry->data->title=text(title);
   if (!xlabel.empty()) entry->data->xLabel=text(xlabel);
   if (!ylabel.empty()) entry->data->yLabel=text(ylabel);
   entry->data->grid=grid; ++entry->data->frame;
   for (Integer i=0;i<ys.GetSize();++i) entry->data->append(i,x,ys[i]);
   if (update) refresh(*entry,true); return true;
}
bool QtPlotReceiver::UpdateXyPlotData(const std::string &name,const Real &x,const Rvector &ys,const Rvector *highs,const Rvector *lows)
{
   auto *entry=find(name); if (!entry) return false;
   ++entry->data->frame;
   for (Integer i=0;i<ys.GetSize();++i) entry->data->append(i,x,ys[i],0,0,true,false,
         highs && i<highs->GetSize() ? (*highs)[i] : 0,lows && i<lows->GetSize() ? (*lows)[i] : 0);
   refresh(*entry); return true;
}
bool QtPlotReceiver::UpdateXyPlotCurve(const std::string &name,const Integer curve,const Real x,const Real y,const Real high,const Real low)
{
   auto *entry=find(name); if (!entry || curve<0) return false;
   ++entry->data->frame; entry->data->append(curve,x,y,0,0,true,false,high,low); refresh(*entry); return true;
}
bool QtPlotReceiver::DeactivateXyPlot(const std::string &name) { if (auto *entry=find(name)) { entry->data->active=false; entry->data->breakLines(); return true; } return false; }
bool QtPlotReceiver::ActivateXyPlot(const std::string &name) { if (auto *entry=find(name)) { entry->data->active=true; return true; } return false; }
bool QtPlotReceiver::TakeXYAction(const std::string &name,const std::string &action)
{
   if (action=="AlwaysRedraw") { SetGlUpdateFrequency(name,1); return find(name)!=nullptr; }
   if (action=="RunModeRedraw") return find(name)!=nullptr;
   if (action=="Clear") { ClearXyPlotData(name); return find(name)!=nullptr; }
   return TakeGlAction(name,action);
}

bool QtPlotReceiver::CreateGroundTrackWindow(const std::string &name,const std::string &oldName,const std::string &title,Real x,Real y,Real w,Real h,bool maximized)
{
   if (oldName!=name && !oldName.empty()) remove(oldName);
   auto &entry=create(name,PlotModel::Kind::GroundTrack,x,y,w,h,maximized);
   if (auto *configured=Moderator::Instance()->GetConfiguredObject(name)) {
      if (configured->IsOfType("GroundTrack")) {
         SetMaxGlDataPoints(name,configured->GetIntegerParameter("MaxPlotPoints"));
         entry.data->defaultLineWidth=std::clamp(static_cast<int>(configured->GetIntegerParameter("LineWidth")),1,20);
         entry.data->footprints=configured->GetStringParameter("ShowFootPrints")=="All";
      }
   }
   entry.data->title=text(title.empty() ? name : title); entry.data->xLabel="Longitude (deg)"; entry.data->yLabel="Latitude (deg)"; return true;
}
void QtPlotReceiver::SetGroundTrackOption(const std::string &name,const std::string &option,const std::string &value)
{
   if (auto *entry=find(name)) {
      if (option=="TextureMap") {
         entry->data->map=QImage(text(value));
         if (!value.empty() && entry->data->map.isNull()) MessageInterface::ShowMessage("Qt GroundTrack '%s': could not load map '%s'; using coordinate grid.\n",name.c_str(),value.c_str());
      } else warn(name,option);
   }
}
bool QtPlotReceiver::UpdateGroundTrackData(const std::string &name,const double epoch,const double *points,const int count)
{
   auto *entry=find(name); if (!entry || count<0 || (count>0 && !points)) return false;
   auto &data=*entry->data; ++data.frame;
   if (data.frame>1 && epoch<data.lastEpoch) data.breakLines();
   for (int i=0;i<count;++i) data.append(i,points[2*i],points[2*i+1],0,epoch);
   refresh(*entry); return true;
}
bool QtPlotReceiver::TakeGroundTrackAction(const std::string &name,const std::string &action)
{
   auto *entry=find(name); if (!entry) return false;
   const auto command=text(action).section('=',0,0), argument=text(action).section('=',1);
   if (command=="Satellites") {
      const auto names=argument.split('|',Qt::SkipEmptyParts);
      for (int i=0;i<names.size();++i) {
         auto &curve=entry->data->curves[i]; curve.name=names[i];
         curve.width=entry->data->defaultLineWidth;
         if (auto *object=Moderator::Instance()->GetSpacePoint(names[i].toStdString())) curve.color=rgb(object->GetCurrentOrbitColor());
      }
   } else if (command=="SatColors") {
      // Satellites resolves GMAT's RGB colors directly, including named colors.
   } else if (command=="AddStation") {
      auto *point=dynamic_cast<BodyFixedPoint *>(Moderator::Instance()->GetSpacePoint(argument.toStdString()));
      if (!point) { MessageInterface::ShowMessage("Qt GroundTrack '%s': missing or invalid station '%s'.\n",name.c_str(),argument.toStdString().c_str()); return false; }
      const auto location=point->GetSphericalLocation(21545.0);
      double longitude=std::remainder(location[1]*degrees,360.0);
      entry->data->stations.append({argument,longitude,location[0]*degrees,rgb(point->GetCurrentOrbitColor())});
   } else if (command=="PlotUpdateFrequency") {
      bool ok=false; int value=argument.toInt(&ok); if (!ok || value<1) return false; entry->data->updateFrequency=value;
   } else if (command=="UseGrid") entry->data->grid=argument=="True";
   else if (command=="LatitudeLineCount" || command=="LongitudeLineCount") {
      bool ok=false; int value=argument.toInt(&ok); if (!ok || value<1 || value>1000) return false;
      if (command=="LatitudeLineCount") entry->data->latitudeLines=value; else entry->data->longitudeLines=value;
   } else if (command=="Reinitialize" || command=="ClearData" || command=="Reset") {
      entry->data->clear(); entry->data->stations.clear();
      if (command=="Reinitialize") entry->data->curves.clear();
   } else if (command=="Refresh" || command=="RunComplete") refresh(*entry,true);
   else return TakeGlAction(name,action);
   return true;
}
bool QtPlotReceiver::CreateDynamicDataDisplay(const std::string &name,const std::string &oldName,const std::string &title,Real x,Real y,Real w,Real h)
{
   if (oldName!=name && !oldName.empty()) remove(oldName);
   auto &entry=create(name,PlotModel::Kind::Table,x,y,w,h,false);
   if (!title.empty()) entry.window->setWindowTitle(text(title)); return true;
}
bool QtPlotReceiver::SetDynamicDataTableSize(const std::string &name,Integer rows,Integer columns)
{
   auto *entry=find(name); if (!entry || !entry->table || rows<0 || columns<0 || rows>10000 || columns>1000) return false;
   entry->table->setRowCount(rows); entry->table->setColumnCount(columns); entry->cells.resize(rows);
   for (auto &row:entry->cells) row.resize(columns); return true;
}
bool QtPlotReceiver::UpdateDynamicDataDisplay(const std::string &name,std::vector<std::vector<DDD>> rows)
{
   auto *entry=find(name); if (!entry) return false;
   int columns=0; for (const auto &row:rows) columns=std::max(columns,static_cast<int>(row.size()));
   entry->cells.resize(rows.size());
   if (entry->table) { entry->table->setRowCount(rows.size()); entry->table->setColumnCount(columns); }
   for (int r=0;r<static_cast<int>(rows.size());++r) {
      entry->cells[r].resize(columns);
      for (int c=0;c<static_cast<int>(rows[r].size());++c) {
         const auto &cell=rows[r][c]; const QString value=text(cell.paramName.empty() ? cell.paramValue : cell.paramName+" = "+cell.paramValue);
         entry->cells[r][c]={value,rgb(cell.paramTextColor),rgb(cell.paramBackgroundColor)};
         if (entry->table) {
            auto *item=new QTableWidgetItem(value); item->setForeground(rgb(cell.paramTextColor)); item->setBackground(rgb(cell.paramBackgroundColor)); entry->table->setItem(r,c,item);
         }
      }
   }
   if (entry->table) fitTableColumns(entry->table); return true;
}
bool QtPlotReceiver::DeleteDynamicData(const std::string &name,const std::string &) { return remove(name); }
bool QtPlotReceiver::SetDynamicDataTextColor(const std::string &name,std::vector<std::vector<DDD>> rows)
{
   auto *entry=find(name); if (!entry) return false;
   for (int r=0;r<static_cast<int>(rows.size());++r) for (int c=0;c<static_cast<int>(rows[r].size());++c)
      if (r<entry->cells.size() && c<entry->cells[r].size()) {
         entry->cells[r][c].foreground=rgb(rows[r][c].paramTextColor);
         if (entry->table) if (auto *item=entry->table->item(r,c)) item->setForeground(rgb(rows[r][c].paramTextColor));
      }
   return true;
}
