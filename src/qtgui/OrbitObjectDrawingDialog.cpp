#include "OrbitObjectDrawingDialog.hpp"
#include "ScriptCompatibility.hpp"
#include "TableColumns.hpp"
#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QScreen>
#include <algorithm>
OrbitObjectDrawingDialog::OrbitObjectDrawingDialog(const QStringList &names,const QMap<QString,bool> &labels,const QMap<QString,bool> &trajectories,QWidget *parent) : QDialog(parent)
{
   setObjectName("orbitObjectDrawingDialog"); setWindowTitle("Object drawing");
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Choose each object's trajectory and label. Default uses the viewer's usual trajectory setting and labels shown objects. Show labels is the master switch. Body/model visibility is set in Orbit-view setup. OK keeps edits pending until Apply.",this);
   help->setWordWrap(true); layout->addWidget(help);
   objects=new QTableWidget(names.size(),3,this); objects->setObjectName("orbitObjectDrawing");
   objects->setHorizontalHeaderLabels({"Object","Trajectory","Label"}); objects->verticalHeader()->hide();
   configureTableColumns(objects,{10,8,8});
   for (int row=0;row<names.size();++row) {
      auto *item=new QTableWidgetItem(names[row]); item->setFlags(item->flags()&~Qt::ItemIsEditable); objects->setItem(row,0,item);
      for (int column=1;column<3;++column) {
         const auto &values=column==1 ? trajectories : labels;
         auto *choice=new QComboBox(objects); choice->setObjectName(QString("orbitDrawing_%1_%2").arg(column==1 ? "trajectory" : "label",names[row]));
         choice->addItems({"Default","On","Off"}); choice->setCurrentIndex(values.contains(names[row]) ? (values.value(names[row]) ? 1 : 2) : 0);
         objects->setCellWidget(row,column,choice);
      }
   }
   layout->addWidget(objects,1);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   resize(560,std::min(380,QGuiApplication::primaryScreen()->availableGeometry().height()-80)); fitTableColumns(objects);
}
QMap<QString,QString> OrbitObjectDrawingDialog::settings() const
{
   QMap<QString,bool> labels,trajectories;
   for (int row=0;row<objects->rowCount();++row) for (int column=1;column<3;++column) {
      const auto *choice=qobject_cast<QComboBox *>(objects->cellWidget(row,column));
      if (choice->currentIndex()) (column==1 ? trajectories : labels).insert(objects->item(row,0)->text(),choice->currentIndex()==1);
   }
   return {{"@QtObjectLabels",qtObjectFlagsJson(labels)},{"@QtObjectTrajectories",qtObjectFlagsJson(trajectories)}};
}
