#include "OrbitObjectDrawingDialog.hpp"
#include "ScriptCompatibility.hpp"
#include "TableColumns.hpp"
#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QLineEdit>
#include <QTabWidget>
#include <stdexcept>
#include <cmath>
#include <limits>
#include <QLabel>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QScreen>
#include <QRegularExpression>
#include <algorithm>
OrbitObjectDrawingDialog::OrbitObjectDrawingDialog(const QStringList &names,const QMap<QString,bool> &labels,const QMap<QString,bool> &trajectories,QWidget *parent,const QtCameraSetting *drawing) : QDialog(parent)
{
   setObjectName("orbitObjectDrawingDialog"); setWindowTitle("Object drawing");
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Choose each object's paths, labels and markers. Default uses normal viewer choices: width 1 pixel and marker size 10 pixels. Show labels is the label master switch. Orbit-view setup controls models/bodies. OK keeps edits pending until Apply.",this);
   help->setWordWrap(true); layout->addWidget(help);
   objects=new QTableWidget(names.size(),4,this); objects->setObjectName("orbitObjectDrawing");
   objects->setHorizontalHeaderLabels({"Object","Trajectory","Label","Width (px)"}); objects->verticalHeader()->hide();
   configureTableColumns(objects,{10,8,8,6});
   for (int row=0;row<names.size();++row) {
      auto *item=new QTableWidgetItem(names[row]); item->setFlags(item->flags()&~Qt::ItemIsEditable); objects->setItem(row,0,item);
      for (int column=1;column<3;++column) {
         const auto &values=column==1 ? trajectories : labels;
         auto *choice=new QComboBox(objects); choice->setObjectName(QString("orbitDrawing_%1_%2").arg(column==1 ? "trajectory" : "label",names[row]));
         choice->addItems({"Default","On","Off"}); choice->setCurrentIndex(values.contains(names[row]) ? (values.value(names[row]) ? 1 : 2) : 0);
         objects->setCellWidget(row,column,choice);
      }
      auto *width=new QLineEdit(objects); width->setObjectName("orbitDrawing_width_"+names[row]); width->setPlaceholderText("Default");
      if (drawing && drawing->objectLineWidths.contains(names[row])) width->setText(QString::number(drawing->objectLineWidths.value(names[row]),'g',17));
      objects->setCellWidget(row,3,width);
   }
   markers=new QTableWidget(names.size(),4,this); markers->setObjectName("orbitObjectMarkers");
   markers->setHorizontalHeaderLabels({"Object","Center","Endpoints","Size (px)"}); markers->verticalHeader()->hide(); configureTableColumns(markers,{10,8,8,6});
   for (int row=0;row<names.size();++row) {
      auto *item=new QTableWidgetItem(names[row]); item->setFlags(item->flags()&~Qt::ItemIsEditable); markers->setItem(row,0,item);
      for (int column=1;column<3;++column) {
         const auto values=drawing ? (column==1 ? drawing->objectCenters : drawing->objectEndpoints) : QMap<QString,bool>();
         auto *choice=new QComboBox(markers); choice->setObjectName(QString("orbitDrawing_%1_%2").arg(column==1 ? "center" : "endpoints",names[row]));
         choice->addItems({"Default","On","Off"}); choice->setCurrentIndex(values.contains(names[row]) ? (values.value(names[row]) ? 1 : 2) : 0); markers->setCellWidget(row,column,choice);
      }
      auto *size=new QLineEdit(markers); size->setObjectName("orbitDrawing_size_"+names[row]); size->setPlaceholderText("Default");
      if (drawing && drawing->objectMarkerSizes.contains(names[row])) size->setText(QString::number(drawing->objectMarkerSizes.value(names[row])));
      markers->setCellWidget(row,3,size);
   }
   auto *tabs=new QTabWidget(this); tabs->setObjectName("orbitDrawingTabs"); tabs->addTab(objects,"Paths and labels"); tabs->addTab(markers,"Markers"); layout->addWidget(tabs,1);
   auto *status=new QLabel(this); status->setObjectName("orbitDrawingStatus"); status->setWordWrap(true); layout->addWidget(status);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   connect(buttons,&QDialogButtonBox::accepted,this,[this,status] { try { settings(); accept(); } catch (const std::exception &error) { status->setText(QString::fromUtf8(error.what())); } }); connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   resize(560,std::min(380,QGuiApplication::primaryScreen()->availableGeometry().height()-80)); fitTableColumns(objects); fitTableColumns(markers);
}
QMap<QString,QString> OrbitObjectDrawingDialog::settings() const
{
   QMap<QString,bool> labels,trajectories;
   for (int row=0;row<objects->rowCount();++row) for (int column=1;column<3;++column) {
      const auto *choice=qobject_cast<QComboBox *>(objects->cellWidget(row,column));
      if (choice->currentIndex()) (column==1 ? trajectories : labels).insert(objects->item(row,0)->text(),choice->currentIndex()==1);
   }
   QMap<QString,double> widths;
   for (int row=0;row<objects->rowCount();++row) {
      const auto text=qobject_cast<QLineEdit *>(objects->cellWidget(row,3))->text().trimmed();
      if (text.isEmpty()) continue;
      bool ok=false; const auto width=text.toDouble(&ok);
      if (!ok || !std::isfinite(width) || width<1 || width>std::numeric_limits<float>::max()) throw std::runtime_error((objects->item(row,0)->text()+": enter a finite line width >= 1 within the renderer's range, or leave it empty for Default.").toStdString());
      widths[objects->item(row,0)->text()]=width;
   }
   QMap<QString,bool> centers,endpoints; QMap<QString,quint32> sizes;
   for (int row=0;row<markers->rowCount();++row) {
      const auto name=markers->item(row,0)->text();
      for (int column=1;column<3;++column) {
         const auto *choice=qobject_cast<QComboBox *>(markers->cellWidget(row,column));
         if (choice->currentIndex()) (column==1 ? centers : endpoints).insert(name,choice->currentIndex()==1);
      }
      const auto text=qobject_cast<QLineEdit *>(markers->cellWidget(row,3))->text().trimmed();
      if (!text.isEmpty()) {
         bool ok=false; const auto size=text.toULongLong(&ok);
         if (!ok || size>4294967295ULL || !QRegularExpression("^[0-9]+$").match(text).hasMatch()) throw std::runtime_error((name+": enter an unsigned integer marker size or leave it empty for Default.").toStdString());
         sizes[name]=static_cast<quint32>(size);
      }
   }
   return {{"@QtObjectLabels",qtObjectFlagsJson(labels)},{"@QtObjectTrajectories",qtObjectFlagsJson(trajectories)},{"@QtObjectCenters",qtObjectFlagsJson(centers)},{"@QtObjectEndpoints",qtObjectFlagsJson(endpoints)},{"@QtObjectMarkerSizes",qtObjectSizesJson(sizes)},{"@QtObjectLineWidths",qtObjectWidthsJson(widths)}};
}
