#include "PathSettingsDialog.hpp"
#include "QtMessageReceiver.hpp"
#include "BaseException.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QSaveFile>
#include <QPlainTextEdit>
#include <QFontDatabase>
#include <algorithm>
#include <exception>
namespace {
QString normalizedDirectory(const QString &path)
{
   if (path.trimmed().isEmpty()) return {};
   return QDir::cleanPath(QFileInfo(path.trimmed()).absoluteFilePath());
}
bool sameFile(const QString &a,const QString &b)
{
   if (a.isEmpty() || b.isEmpty()) return false;
   const QFileInfo left(a),right(b);
   return left.absoluteFilePath()==right.absoluteFilePath() ||
      (!left.canonicalFilePath().isEmpty() && left.canonicalFilePath()==right.canonicalFilePath());
}
}
PathSettingsDialog::PathSettingsDialog(PathSettings initial,QtMessageReceiver &receiver,Apply apply,const QString &missionFile,QWidget *parent): QDialog(parent),settings(std::move(initial)),missionFile(missionFile)
{
   setWindowTitle("Set paths"); setObjectName("pathSettingsDialog"); resize(760,580);
   auto *layout=new QVBoxLayout(this);
   auto *form=new QFormLayout; startup=new QLineEdit(this); startup->setObjectName("pathStartupFile"); startup->setReadOnly(true); form->addRow("Startup file",startup); layout->addLayout(form);
   auto *startupButtons=new QHBoxLayout;
   auto *read=new QPushButton("Read another startup file…",this); read->setObjectName("pathReadStartup");
   auto *save=new QPushButton("Export startup settings…",this); save->setObjectName("pathSaveStartup");
   startupButtons->addWidget(read); startupButtons->addWidget(save); startupButtons->addStretch(); layout->addLayout(startupButtons);
   auto *note=new QLabel("Apply updates GMAT function and output paths in this session. Export writes a startup file. Python module paths require restarting GMAT with that exported file.",this); note->setWordWrap(true); layout->addWidget(note);
   auto *tabs=new QTabWidget(this); tabs->setObjectName("pathTabs"); layout->addWidget(tabs,1);
   auto *functionTab=new QWidget(tabs); auto *functionLayout=new QVBoxLayout(functionTab);
   auto *orderNote=new QLabel("Search order is top to bottom. The mission directory takes precedence when building a mission. Repeated equivalent directories appear once.",functionTab); orderNote->setWordWrap(true); functionLayout->addWidget(orderNote);
   functions=new QListWidget(functionTab); functions->setObjectName("pathFunctions"); functionLayout->addWidget(functions,1);
   auto *directoryRow=new QHBoxLayout; entry=new QLineEdit(functionTab); entry->setObjectName("pathFunctionEntry"); directoryRow->addWidget(entry,1);
   auto *browse=new QPushButton("Browse…",functionTab); browse->setObjectName("pathFunctionBrowse"); directoryRow->addWidget(browse); functionLayout->addLayout(directoryRow);
   auto *operations=new QHBoxLayout;
   auto button=[&](const QString &text,const char *name) { auto *value=new QPushButton(text,functionTab); value->setObjectName(name); operations->addWidget(value); return value; };
   auto *add=button("Add","pathFunctionAdd"),*replace=button("Replace","pathFunctionReplace"),*remove=button("Remove","pathFunctionRemove"),*up=button("Up","pathFunctionUp"),*down=button("Down","pathFunctionDown"); operations->addStretch(); functionLayout->addLayout(operations); tabs->addTab(functionTab,"GMAT Function");
   auto *pythonTab=new QWidget(tabs); pythonTab->setObjectName("pathPythonTab"); auto *pythonLayout=new QVBoxLayout(pythonTab);
   auto *pythonNote=new QLabel("Top to bottom within GMAT's configured module paths; Python's standard and environment paths are searched first. Export startup settings and restart GMAT with that file to use additions, removals or a new order. Apply does not change Python's search paths or reload imported modules.",pythonTab); pythonNote->setObjectName("pathPythonRestartNote"); pythonNote->setWordWrap(true); pythonLayout->addWidget(pythonNote);
   pythonModules=new QListWidget(pythonTab); pythonModules->setObjectName("pathPythonModules"); pythonLayout->addWidget(pythonModules,1);
   auto *pythonDirectoryRow=new QHBoxLayout; pythonEntry=new QLineEdit(pythonTab); pythonEntry->setObjectName("pathPythonEntry"); pythonDirectoryRow->addWidget(pythonEntry,1);
   auto *pythonBrowse=new QPushButton("Browse…",pythonTab); pythonBrowse->setObjectName("pathPythonBrowse"); pythonDirectoryRow->addWidget(pythonBrowse); pythonLayout->addLayout(pythonDirectoryRow);
   auto *pythonOperations=new QHBoxLayout;
   auto pythonButton=[&](const QString &text,const char *name) { auto *value=new QPushButton(text,pythonTab); value->setObjectName(name); pythonOperations->addWidget(value); return value; };
   auto *pythonAdd=pythonButton("Add","pathPythonAdd"),*pythonReplace=pythonButton("Replace","pathPythonReplace"),*pythonRemove=pythonButton("Remove","pathPythonRemove"),*pythonUp=pythonButton("Up","pathPythonUp"),*pythonDown=pythonButton("Down","pathPythonDown"); pythonOperations->addStretch(); pythonLayout->addLayout(pythonOperations); tabs->addTab(pythonTab,"Python Module");
   auto *outputTab=new QWidget(tabs); outputTab->setObjectName("pathOutputTab"); auto *outputLayout=new QVBoxLayout(outputTab); auto *outputRow=new QHBoxLayout;
   output=new QLineEdit(outputTab); output->setObjectName("pathOutput"); auto *outputBrowse=new QPushButton("Browse…",outputTab); outputBrowse->setObjectName("pathOutputBrowse"); outputRow->addWidget(output,1); outputRow->addWidget(outputBrowse); outputLayout->addLayout(outputRow);
   auto *outputNote=new QLabel("Choose an existing writable directory for default reports and the log. Explicit mission filenames retain their own destinations.",outputTab); outputNote->setWordWrap(true); outputLayout->addWidget(outputNote); outputLayout->addStretch(); tabs->addTab(outputTab,"Output");
   preview=new QPlainTextEdit(tabs); preview->setObjectName("pathStartupPreview"); preview->setReadOnly(true); preview->setLineWrapMode(QPlainTextEdit::NoWrap); preview->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont)); tabs->addTab(preview,"Startup settings");
   status=new QLabel(this); status->setObjectName("pathStatus"); status->setWordWrap(true); layout->addWidget(status); saved=new QLabel(this); saved->setObjectName("pathSavedFile"); saved->setWordWrap(true); layout->addWidget(saved);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::Close,this); layout->addWidget(buttons);
   const auto refresh=[=] { preview->setPlainText(pathSettingsScript(pending())); };
   connect(output,&QLineEdit::textChanged,this,refresh);
   connect(functions->model(),&QAbstractItemModel::rowsInserted,this,refresh); connect(functions->model(),&QAbstractItemModel::rowsRemoved,this,refresh); connect(functions->model(),&QAbstractItemModel::dataChanged,this,refresh);
   connect(functions,&QListWidget::currentTextChanged,entry,&QLineEdit::setText);
   const auto enabled=[=] { const int row=functions->currentRow(); replace->setEnabled(row>=0); remove->setEnabled(row>=0); up->setEnabled(row>0); down->setEnabled(row>=0 && row+1<functions->count()); };
   connect(functions,&QListWidget::currentRowChanged,this,enabled); connect(functions->model(),&QAbstractItemModel::rowsInserted,this,enabled); connect(functions->model(),&QAbstractItemModel::rowsRemoved,this,enabled);
   connect(browse,&QPushButton::clicked,this,[=] { const auto path=QFileDialog::getExistingDirectory(this,"GMAT function directory",entry->text()); if (!path.isEmpty()) entry->setText(path); });
   connect(pythonModules->model(),&QAbstractItemModel::rowsInserted,this,refresh); connect(pythonModules->model(),&QAbstractItemModel::rowsRemoved,this,refresh); connect(pythonModules->model(),&QAbstractItemModel::dataChanged,this,refresh);
   connect(pythonModules,&QListWidget::currentTextChanged,pythonEntry,&QLineEdit::setText);
   const auto pythonEnabled=[=] { const int row=pythonModules->currentRow(); pythonReplace->setEnabled(row>=0); pythonRemove->setEnabled(row>=0); pythonUp->setEnabled(row>0); pythonDown->setEnabled(row>=0 && row+1<pythonModules->count()); };
   connect(pythonModules,&QListWidget::currentRowChanged,this,pythonEnabled); connect(pythonModules->model(),&QAbstractItemModel::rowsInserted,this,pythonEnabled); connect(pythonModules->model(),&QAbstractItemModel::rowsRemoved,this,pythonEnabled);
   connect(pythonBrowse,&QPushButton::clicked,this,[=] { const auto path=QFileDialog::getExistingDirectory(this,"Python module directory",pythonEntry->text()); if (!path.isEmpty()) pythonEntry->setText(path); });
   auto putPython=[=](bool replacing) {
      const auto path=normalizedDirectory(pythonEntry->text());
      if (path.isEmpty() || !QFileInfo(path).isDir() || !QFileInfo(path).isReadable()) { status->setText("Choose an existing readable Python module directory."); return; }
      if (path.contains('"') || path.contains('\\')) { status->setText("Choose a Python module directory without double quotes or backslashes."); return; }
      for (int i=0;i<pythonModules->count();++i) if ((!replacing || i!=pythonModules->currentRow()) && sameFile(path,pythonModules->item(i)->text())) { status->setText("That Python module directory is already listed."); return; }
      if (replacing && pythonModules->currentItem()) pythonModules->currentItem()->setText(path);
      else { pythonModules->insertItem(0,path); pythonModules->setCurrentRow(0); }
      status->clear(); refresh();
   };
   connect(pythonAdd,&QPushButton::clicked,this,[=] { putPython(false); }); connect(pythonEntry,&QLineEdit::returnPressed,pythonAdd,&QPushButton::click); connect(pythonReplace,&QPushButton::clicked,this,[=] { putPython(true); });
   connect(pythonRemove,&QPushButton::clicked,this,[=] { delete pythonModules->takeItem(pythonModules->currentRow()); refresh(); });
   auto movePython=[=](int offset) { const int row=pythonModules->currentRow(); if (row<0 || row+offset<0 || row+offset>=pythonModules->count()) return; auto *item=pythonModules->takeItem(row); pythonModules->insertItem(row+offset,item); pythonModules->setCurrentRow(row+offset); refresh(); };
   connect(pythonUp,&QPushButton::clicked,this,[=] { movePython(-1); }); connect(pythonDown,&QPushButton::clicked,this,[=] { movePython(1); });
   connect(outputBrowse,&QPushButton::clicked,this,[=] { const auto path=QFileDialog::getExistingDirectory(this,"Output directory",output->text()); if (!path.isEmpty()) output->setText(path); });
   auto put=[=](bool replacing) {
      const auto path=normalizedDirectory(entry->text());
      if (path.isEmpty() || !QFileInfo(path).isDir()) { status->setText("Choose an existing function directory."); return; }
      for (int i=0;i<functions->count();++i) if ((!replacing || i!=functions->currentRow()) && sameFile(path,functions->item(i)->text())) { status->setText("That function directory is already listed."); return; }
      if (replacing && functions->currentItem()) functions->currentItem()->setText(path);
      else { functions->insertItem(0,path); functions->setCurrentRow(0); }
      status->clear(); refresh();
   };
   connect(add,&QPushButton::clicked,this,[=] { put(false); }); connect(entry,&QLineEdit::returnPressed,add,&QPushButton::click); connect(replace,&QPushButton::clicked,this,[=] { put(true); });
   connect(remove,&QPushButton::clicked,this,[=] { delete functions->takeItem(functions->currentRow()); refresh(); });
   auto move=[=](int offset) { const int row=functions->currentRow(); if (row<0 || row+offset<0 || row+offset>=functions->count()) return; auto *item=functions->takeItem(row); functions->insertItem(row+offset,item); functions->setCurrentRow(row+offset); refresh(); };
   connect(up,&QPushButton::clicked,this,[=] { move(-1); }); connect(down,&QPushButton::clicked,this,[=] { move(1); });
   connect(read,&QPushButton::clicked,this,[this,&receiver] {
      const auto path=QFileDialog::getOpenFileName(this,"Read startup settings",settings.startupFile,"Startup files (*.txt);;All files (*)"); if (path.isEmpty()) return;
      try { settings=readPathSettings(path,receiver); loadControls(); status->setText("Loaded as pending settings. Apply uses function/output paths; Python module paths require exporting startup settings and restarting with that file."); saved->clear(); }
      catch (BaseException &error) { status->setText(QString::fromStdString(error.GetFullMessage())); }
      catch (const std::exception &error) { status->setText(QString::fromUtf8(error.what())); }
   });
   connect(save,&QPushButton::clicked,this,[this] {
      const auto settings=pending(); const auto error=validatePathSettings(settings); if (!error.isEmpty()) { status->setText(error); return; }
      const auto path=QFileDialog::getSaveFileName(this,"Save startup settings",settings.startupFile,"Startup files (*.txt);;All files (*)"); if (path.isEmpty()) return;
      const auto exportError=exportStartup(path); if (!exportError.isEmpty()) { status->setText(exportError); return; }
      saved->setText("Saved: "+QFileInfo(path).absoluteFilePath()); status->setText("Startup settings exported. Restart GMAT with this file to use its Python module paths. Apply separately to use function/output paths now.");
   });
   connect(buttons->button(QDialogButtonBox::Apply),&QPushButton::clicked,this,[this,apply] { const auto candidate=pending(); const auto error=apply(candidate); if (error.isEmpty()) { settings=candidate; settings.startupText=pathSettingsScript(candidate); status->setText("Applied GMAT function and output paths. Rebuild the mission. Python module paths take effect only after exporting startup settings and restarting GMAT with that file."); } else status->setText(error); });
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   loadControls(); enabled(); pythonEnabled();
}
QString PathSettingsDialog::exportStartup(const QString &path) const
{
   const auto settings=pending(); const auto error=validatePathSettings(settings); if (!error.isEmpty()) return error;
   if (sameFile(path,missionFile)) return "Choose a startup filename different from the mission script.";
   QSaveFile file(path); const auto text=pathSettingsScript(settings).toUtf8();
   if (!file.open(QIODevice::WriteOnly) || file.write(text)!=text.size() || !file.commit()) return "Cannot save startup settings: "+file.errorString();
   return {};
}
PathSettings PathSettingsDialog::pending() const
{
   auto value=settings; value.output=normalizedDirectory(output->text()); value.functions.clear();
   for (int i=0;i<functions->count();++i) value.functions.append(functions->item(i)->text());
   value.pythonModules.clear(); for (int i=0;i<pythonModules->count();++i) value.pythonModules.append(pythonModules->item(i)->text());
   return value;
}
void PathSettingsDialog::loadControls()
{
   startup->setText(settings.startupFile); functions->clear();
   for (const auto &path:settings.functions) functions->addItem(QDir::cleanPath(path));
   if (functions->count()) functions->setCurrentRow(0);
   pythonModules->clear(); for (const auto &path:settings.pythonModules) pythonModules->addItem(QDir::cleanPath(path));
   if (pythonModules->count()) pythonModules->setCurrentRow(0);
   output->setText(QDir::cleanPath(settings.output)); preview->setPlainText(pathSettingsScript(pending()));
}
