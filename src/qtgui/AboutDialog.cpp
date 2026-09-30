#include "AboutDialog.hpp"
#include "InspectionDialog.hpp"
#include "GmatGlobal.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringDecoder>
#include <osg/Version>

AboutDialog::AboutDialog(const QString &rootPath,QWidget *parent) : QDialog(parent)
{
   setObjectName("aboutDialog"); setWindowTitle("About GMAT"); resize(640,540);
   auto *layout=new QVBoxLayout(this);
   auto *global=GmatGlobal::Instance();
   const auto details=QString("General Mission Analysis Tool\n%1 (%2-bit)\nBuild date: %3 %4\nQt %5 desktop interface · OpenSceneGraph %6")
      .arg(QString::fromStdString(global->GetGmatVersion())).arg(global->IsGmatCompiledIn64Bit() ? 64 : 32)
      .arg(QString::fromStdString(global->GetGMATBuildDate()),QString::fromStdString(global->GetGMATBuildTime()),QString::fromLatin1(qVersion()),QString::fromLatin1(osgGetVersion()));
   auto *info=new QLabel(details,this); info->setObjectName("aboutInfo"); info->setWordWrap(true);
   info->setTextInteractionFlags(Qt::TextSelectableByMouse); layout->addWidget(info);
   auto *links=new QLabel("Website: <a href=\"https://sourceforge.net/projects/gmat/\">GMAT project</a><br>Contact: <a href=\"mailto:gmat@gsfc.nasa.gov\">gmat@gsfc.nasa.gov</a><br>License: Apache License, Version 2.0",this);
   links->setObjectName("aboutLinks");
   links->setWordWrap(true); links->setOpenExternalLinks(true); links->setTextInteractionFlags(Qt::TextBrowserInteraction); layout->addWidget(links);
   auto *credits=new QPlainTextEdit(this); credits->setObjectName("aboutCredits"); credits->setReadOnly(true);
   credits->setPlainText("GMAT was developed by NASA/GSFC, Thinking Systems, Inc. and contributors.\n\n"
      "This Qt interface uses Qt and OpenSceneGraph. The GMAT engine and supported plugins use third-party content including:\n"
      " • JPL SPICE\n • IAU SOFA\n • Apache Xerces\n • Boost\n • f2c\n • MSISE 1990 and NRL MSISE 2000 density models\n • IRI 2007 ionosphere model\n"
      "\nPlanetary images courtesy of JPL/Caltech/USGS, Celestia Motherlode, Bjorn Jonsson and NASA World Wind.");
   layout->addWidget(credits,1);
   auto *error=new QLabel(this); error->setObjectName("aboutError"); error->setWordWrap(true); layout->addWidget(error);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Close,this); layout->addWidget(buttons);
   auto *license=new QPushButton("View license…",this); license->setObjectName("showLicense"); buttons->addButton(license,QDialogButtonBox::ActionRole);
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(license,&QPushButton::clicked,this,[this,rootPath,error] {
      const auto path=QFileInfo(QDir(rootPath).filePath("License.txt")).absoluteFilePath(); QFile file(path);
      if (!file.open(QIODevice::ReadOnly)) { error->setText("Cannot read the GMAT license at "+path+": "+file.errorString()); return; }
      QStringDecoder decoder(QStringDecoder::Utf8); const QString text=decoder.decode(file.readAll());
      if (file.error()!=QFileDevice::NoError || decoder.hasError() || text.trimmed().isEmpty()) {
         error->setText("Cannot read a complete UTF-8 license at "+path+". Restore License.txt in the GMAT root folder and try again."); return;
      }
      error->clear(); InspectionDialog dialog("Apache License, Version 2.0",text,path,this);
      dialog.setObjectName("licenseDialog"); dialog.exec();
   });
}
