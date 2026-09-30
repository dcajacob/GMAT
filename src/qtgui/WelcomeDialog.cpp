#include "WelcomeDialog.hpp"
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QScrollArea>

QStringList recentMissionFiles() { return QSettings().value("RecentFiles").toStringList(); }
void rememberMissionFile(const QString &path)
{
   const auto file=QFileInfo(path); const auto absolute=file.absoluteFilePath(),canonical=file.canonicalFilePath();
   auto files=recentMissionFiles();
   for (int i=files.size()-1;i>=0;--i) if (QFileInfo(files[i]).absoluteFilePath()==absolute || (!canonical.isEmpty() && QFileInfo(files[i]).canonicalFilePath()==canonical)) files.removeAt(i);
   files.prepend(absolute); files=files.mid(0,10); QSettings().setValue("RecentFiles",files);
}
WelcomeDialog::WelcomeDialog(const QString &root,std::function<bool(const QString &)> open,
   std::function<bool()> create,std::function<void(const QString &)> help,QWidget *parent) : QDialog(parent)
{
   setObjectName("welcomeDialog"); setWindowTitle("Welcome to GMAT"); setProperty("helpTopic","UsingGmat"); setAttribute(Qt::WA_DeleteOnClose); resize(680,540);
   auto *outer=new QVBoxLayout(this); auto *scroll=new QScrollArea(this); scroll->setWidgetResizable(true); auto *content=new QWidget(scroll); scroll->setWidget(content); outer->addWidget(scroll,1);
   auto *layout=new QVBoxLayout(content); auto *title=new QLabel("General Mission Analysis Tool",this); auto font=title->font(); font.setPointSize(font.pointSize()+4); font.setBold(true); title->setFont(font); title->setWordWrap(true); layout->addWidget(title);
   auto *intro=new QLabel("Open a recent mission, explore the supplied samples or start a new mission.",this); intro->setWordWrap(true); layout->addWidget(intro);
   auto *createButton=new QPushButton("New mission",this); createButton->setObjectName("welcomeNew"); layout->addWidget(createButton);
   connect(createButton,&QPushButton::clicked,this,[this,create] { if (create()) close(); else status->setText("New mission canceled. The current mission is retained."); });
   layout->addWidget(new QLabel("Recent missions",this)); recent=new QListWidget(this); recent->setObjectName("welcomeRecent"); layout->addWidget(recent,1); refreshRecent();
   auto load=[this,open](const QString &path) {
      if (!QFileInfo(path).isFile()) { status->setText("Mission file is unavailable: "+path); return; }
      if (open(path)) close(); else status->setText("Opening was canceled or the mission could not be built. See the Message Window for build errors.");
   };
   connect(recent,&QListWidget::itemDoubleClicked,this,[load](QListWidgetItem *item) { load(item->data(Qt::UserRole).toString()); });
   auto *recentActions=new QHBoxLayout; layout->addLayout(recentActions); auto *openRecent=new QPushButton("Open selected",this); openRecent->setObjectName("welcomeOpenRecent"); recentActions->addWidget(openRecent);
   connect(openRecent,&QPushButton::clicked,this,[this,load] { if (recent->currentItem()) load(recent->currentItem()->data(Qt::UserRole).toString()); });
   auto *clear=new QPushButton("Clear recent list",this); clear->setObjectName("welcomeClearRecent"); recentActions->addWidget(clear); recentActions->addStretch(); connect(clear,&QPushButton::clicked,this,[this] { QSettings().remove("RecentFiles"); refreshRecent(); });
   auto *samplesRow=new QHBoxLayout; layout->addLayout(samplesRow); auto *samples=new QComboBox(this); samples->setObjectName("welcomeSamples"); samplesRow->addWidget(samples,1);
   QSettings configuration(QDir(root).filePath("bin/GMAT.ini"),QSettings::IniFormat); configuration.beginGroup("Welcome/Samples");
   for (const auto &label:configuration.childKeys()) { const auto configured=configuration.value(label).toString(); samples->addItem(label,QFileInfo(QDir(QDir(root).filePath("bin")).filePath(configured)).absoluteFilePath()); } configuration.endGroup();
   if (!samples->count()) samples->addItem("Sample missions",QDir(root).filePath("samples"));
   auto *browse=new QPushButton("Browse samples…",this); browse->setObjectName("welcomeBrowseSamples"); samplesRow->addWidget(browse);
   connect(browse,&QPushButton::clicked,this,[this,samples,load] { const auto directory=samples->currentData().toString(); if (!QFileInfo(directory).isDir()) { status->setText("Sample folder is unavailable: "+directory); return; } const auto file=QFileDialog::getOpenFileName(this,"Open sample mission",directory,"GMAT scripts (*.script *.m);;All files (*)"); if (!file.isEmpty()) load(file); });
   auto *guides=new QHBoxLayout; layout->addLayout(guides);
   for (const auto &topic:QStringList{"UsingGmat","RefGuide","Tutorials"}) { const auto label=topic=="UsingGmat" ? "Using GMAT" : topic=="RefGuide" ? "Reference guide" : "Tutorials"; auto *button=new QPushButton(label,this); button->setObjectName("welcomeHelp"+topic); guides->addWidget(button); connect(button,&QPushButton::clicked,this,[help,topic] { help(topic); }); }
   // Keep configured project/support links available without replacing the
   // in-app offline guide. A link is opened only when its button is selected.
   for (const auto &group:QStringList{"Welcome/Links","GettingStarted/Tutorials"}) {
      configuration.beginGroup(group); auto *links=new QHBoxLayout; layout->addLayout(links);
      for (const auto &label:configuration.childKeys()) {
         const auto target=configuration.value(label).toString(); auto url=QUrl(target);
         if (url.scheme().isEmpty() && target.endsWith(".html")) {
            // The standard guide topics already have in-app buttons above.
            if (QFileInfo(target).baseName()=="RefGuide" || QFileInfo(target).baseName()=="Tutorials") continue;
            url=QUrl::fromLocalFile(QFileInfo(QDir(QDir(root).filePath("bin")).filePath(target)).absoluteFilePath());
         }
         if (url.scheme()!="https" && url.scheme()!="http" && !url.isLocalFile()) continue;
         auto *button=new QPushButton(label,this); button->setProperty("welcomeLink",url); links->addWidget(button); connect(button,&QPushButton::clicked,this,[this,url] {
            if (url.isLocalFile() && !QFileInfo(url.toLocalFile()).isFile()) { status->setText("Help file is unavailable: "+url.toLocalFile()); return; }
            if (!QDesktopServices::openUrl(url)) status->setText("The desktop could not open "+url.toString());
         });
      } configuration.endGroup();
   }
   status=new QLabel(this); status->setObjectName("welcomeStatus"); status->setWordWrap(true); outer->addWidget(status);
   auto *show=new QCheckBox("Show Welcome on startup",this); show->setObjectName("welcomeOnStartup"); show->setChecked(QSettings().value("Welcome/showOnStartup",true).toBool()); outer->addWidget(show); connect(show,&QCheckBox::toggled,this,[](bool enabled) { QSettings().setValue("Welcome/showOnStartup",enabled); });
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Close,this); outer->addWidget(buttons); connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::close);
}
void WelcomeDialog::refreshRecent()
{
   recent->clear(); for (const auto &file:recentMissionFiles()) { auto *item=new QListWidgetItem(QFileInfo(file).fileName(),recent); item->setData(Qt::UserRole,file); item->setToolTip(file); }
   if (recent->count()) recent->setCurrentRow(0);
}
