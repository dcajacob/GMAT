#include "RegionDialog.hpp"
#include "ResourceProperties.hpp"
#include "ResourcePreview.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <memory>
#include <algorithm>
#include "Rvector.hpp"

RegionDialog::RegionDialog(GmatBase &region,QWidget *parent) : QDialog(parent)
{
   setObjectName("regionDialog"); setWindowTitle("Planetographic region");
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Edit settings here. Apply commits them; Close lets you discard pending changes.",this);
   help->setWordWrap(true); layout->addWidget(help);
   auto *form=new QFormLayout; layout->addLayout(form);
   body=new QComboBox(this); body->setObjectName("region_CentralBody"); body->addItem("Earth");
   const auto currentBody=QString::fromStdString(region.GetStringParameter("CentralBody"));
   if (currentBody!="Earth") body->addItem(currentBody);
   body->setCurrentText(currentBody); form->addRow("Central body",body);
   source=new QComboBox(this); source->setObjectName("regionSourceMode");
   source->addItem("Vertices","Vertices"); source->addItem("Area file","File"); form->addRow("Define region with",source);
   const auto currentFile=QString::fromStdString(region.GetStringParameter("AreaFileName"));
   auto *row=new QWidget(this); auto *fileLayout=new QHBoxLayout(row); fileLayout->setContentsMargins(0,0,0,0);
   file=new QLineEdit(currentFile,row); file->setObjectName("region_AreaFileName"); fileLayout->addWidget(file);
   auto *browse=new QPushButton("Browse…",row); browse->setObjectName("regionBrowse"); fileLayout->addWidget(browse);
   form->addRow("Area file",row);
   connect(browse,&QPushButton::clicked,this,[this] {
      const auto selected=QFileDialog::getOpenFileName(this,"Choose region area file",file->text(),"Region area files (*.AT *.at);;All files (*)");
      if (!selected.isEmpty()) file->setText(selected);
   });
   auto *instructions=new QLabel("Enter at least three paired vertices in degrees. The last vertex connects to the first. Area files use the PlanetographicRegion text format.",this);
   instructions->setWordWrap(true); layout->addWidget(instructions);
   vertices=new QTableWidget(this); vertices->setObjectName("regionVertices"); vertices->setColumnCount(2);
   vertices->setHorizontalHeaderLabels({"Latitude (deg)","Longitude (deg)"});
   vertices->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
   vertices->setSelectionBehavior(QAbstractItemView::SelectRows); vertices->setSelectionMode(QAbstractItemView::SingleSelection);
   vertices->setMinimumHeight(180); layout->addWidget(vertices,1);
   const Rvector latitude=region.GetRvectorParameter("Latitude"),longitude=region.GetRvectorParameter("Longitude");
   vertices->setRowCount(std::max(3,std::max(latitude.GetSize(),longitude.GetSize())));
   for (int i=0;i<vertices->rowCount();++i) {
      vertices->setItem(i,0,new QTableWidgetItem(i<latitude.GetSize() ? QString::number(latitude[i],'g',17) : QString()));
      vertices->setItem(i,1,new QTableWidgetItem(i<longitude.GetSize() ? QString::number(longitude[i],'g',17) : QString()));
   }
   auto *vertexButtons=new QHBoxLayout; layout->addLayout(vertexButtons);
   auto *add=new QPushButton("Add vertex",this); add->setObjectName("regionAddVertex"); vertexButtons->addWidget(add);
   auto *remove=new QPushButton("Remove vertex",this); remove->setObjectName("regionRemoveVertex"); vertexButtons->addWidget(remove); vertexButtons->addStretch();
   connect(add,&QPushButton::clicked,this,[this] {
      if (vertices->rowCount()>=100000) return;
      const int i=vertices->rowCount(); vertices->insertRow(i);
      vertices->setItem(i,0,new QTableWidgetItem); vertices->setItem(i,1,new QTableWidgetItem);
      vertices->setCurrentCell(i,0); vertices->editItem(vertices->item(i,0));
   });
   connect(remove,&QPushButton::clicked,this,[this] { if (vertices->currentRow()>=0) vertices->removeRow(vertices->currentRow()); });
   const auto dependencies=[this,row,add,remove] {
      const bool fromFile=source->currentData().toString()=="File";
      row->setEnabled(fromFile); vertices->setEnabled(!fromFile); add->setEnabled(!fromFile); remove->setEnabled(!fromFile);
   };
   connect(source,&QComboBox::currentIndexChanged,this,dependencies);
   source->setCurrentIndex(currentFile.isEmpty() ? 0 : 1); dependencies();
   auto *error=new QLabel(this); error->setWordWrap(true); layout->addWidget(error);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   auto original=qtResourcePreviewShared(region.Clone());
   connect(buttons,&QDialogButtonBox::accepted,this,[this,original,error] {
      try { QtResourcePreview preview(original->Clone()); applyRegionProperties(*preview,settings()); accept(); }
      catch (BaseException &failure) { error->setText(QString::fromStdString(failure.GetFullMessage())); }
      catch (const std::exception &failure) { error->setText(QString::fromUtf8(failure.what())); }
   });
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
}
QMap<QString,QString> RegionDialog::settings() const
{
   const bool fromFile=source->currentData().toString()=="File";
   QMap<QString,QString> result{{"@RegionSource",fromFile ? "File" : "Vertices"},{"CentralBody",body->currentText()},
      {"AreaFileName",fromFile ? file->text().trimmed() : QString()},{"Latitude",QString()},{"Longitude",QString()}};
   if (!fromFile) for (int column=0;column<2;++column) {
      QStringList values;
      for (int row=0;row<vertices->rowCount();++row) {
         const auto *item=vertices->item(row,column); const auto text=item ? item->text().trimmed() : QString();
         values.append(text.isEmpty() ? "?" : text);
      }
      result[column==0 ? "Latitude" : "Longitude"]=values.join(' ');
   }
   return result;
}
