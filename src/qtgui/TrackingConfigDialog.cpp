#include "TrackingConfigDialog.hpp"
#include "GmatBase.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "GmatGlobal.hpp"
#include "TableColumns.hpp"
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QTimer>
#include <stdexcept>
#include <algorithm>

QStringList trackingMeasurementTypes()
{
   // ErrorModel::GetAllAvailableTypes and TFSMagicNumbers expose these through
   // plugin-specific APIs, rather than GmatBase's property choice metadata.
   QStringList names={"DSN_SeqRange","DSN_PNRange","DSN_TCP","GPS_PosVec","Range","Range_Skin","RangeRate","Azimuth","Elevation","XEast","YNorth","XSouth","YEast","SN_Range","SN_Doppler","BRTS_Range","BRTS_Doppler","SN_Doppler_Rtn","SN_DOWD"};
   if (GmatGlobal::Instance()->InTestingMode()) names.append({"RightAscension","Declination"});
   return names;
}
namespace {
QStringList tokens(const QString &value) { return value.split(QRegularExpression("[\\s,'\"]+"),Qt::SkipEmptyParts); }
QJsonArray configurationRows(const QString &settings)
{
   QJsonParseError error; const auto document=QJsonDocument::fromJson(settings.toUtf8(),&error);
   if (error.error!=QJsonParseError::NoError || !document.isArray()) throw std::runtime_error("Invalid tracking configuration data.");
   const auto rows=document.array();
   for (const auto &entry:rows) if (!entry.isObject() || !entry.toObject().value("path").isString() || !entry.toObject().value("types").isString()) throw std::runtime_error("Invalid tracking configuration row.");
   return rows;
}
QStringList participantChoices()
{
   QStringList choices;
   for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::SPACE_POINT)) {
      auto *object=Moderator::Instance()->GetConfiguredObject(name);
      if (!object || (!object->IsOfType("GroundStation") && !object->IsOfType("Spacecraft"))) continue;
      choices.append(QString::fromStdString(name));
      for (const auto &sensor:object->GetStringArrayParameter("AddHardware")) choices.append(QString::fromStdString(name+"."+sensor));
   }
   choices.removeDuplicates(); choices.sort(); return choices;
}
QString definition(const QJsonObject &row)
{
   const auto path=tokens(row.value("path").toString()),types=tokens(row.value("types").toString());
   if (path.isEmpty() || types.isEmpty()) throw std::runtime_error("Each row needs a signal path and at least one measurement type.");
   const QRegularExpression participant("^[A-Za-z][A-Za-z0-9_]*(?:\\.[A-Za-z][A-Za-z0-9_]*)?$");
   for (const auto &name:path) {
      if (!participant.match(name).hasMatch()) throw std::runtime_error("Use spacecraft/station names with an optional hardware name in signal paths.");
      auto *object=Moderator::Instance()->GetConfiguredObject(name.section('.',0,0).toStdString());
      if (!object || (!object->IsOfType("GroundStation") && !object->IsOfType("Spacecraft"))) throw std::runtime_error(("Unknown spacecraft or station: "+name).toStdString());
      if (name.contains('.')) {
         const auto hardware=object->GetStringArrayParameter("AddHardware");
         if (std::find(hardware.begin(),hardware.end(),name.section('.',1,1).toStdString())==hardware.end()) throw std::runtime_error(("Hardware is not attached to this participant: "+name).toStdString());
      }
   }
   QSet<QString> unique; const auto available=trackingMeasurementTypes(); QStringList quoted;
   for (const auto &type:types) {
      if (!available.contains(type)) throw std::runtime_error(("Unknown measurement type: "+type).toStdString());
      if (unique.contains(type)) throw std::runtime_error("Measurement types must be unique within a row.");
      unique.insert(type); quoted.append("'"+type+"'");
   }
   return "{{"+path.join(", ")+"}, "+quoted.join(", ")+"}";
}
}
QString trackingConfigurations(GmatBase &object)
{
   QJsonArray rows;
   const QRegularExpression pattern("^\\s*\\{\\s*\\{([^{}]*)\\}\\s*,([^{}]+)\\}\\s*$");
   for (const auto &entry:object.GetStringArrayParameter("AddTrackingConfig")) {
      const auto match=pattern.match(QString::fromStdString(entry));
      if (!match.hasMatch()) throw std::runtime_error("This tracking definition needs the script editor; its existing signal paths will be preserved.");
      rows.append(QJsonObject{{"path",tokens(match.captured(1)).join(", ")},{"types",tokens(match.captured(2)).join(", ")}});
   }
   return QString::fromUtf8(QJsonDocument(rows).toJson(QJsonDocument::Compact));
}
QString replaceTrackingConfigurations(GmatBase &object,const QString &block,const QString &settings)
{
   if (!object.IsOfType("TrackingFileSet")) throw std::runtime_error("Tracking configurations require a TrackingFileSet.");
   const auto key=QString::fromStdString(object.GetName())+".AddTrackingConfig";
   QString result=block;
   result.remove(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?"+QRegularExpression::escape(key)+"[ \\t]*=[^;]*;",QRegularExpression::MultilineOption));
   if (!result.endsWith('\n')) result+='\n';
   QSet<QString> unique;
   for (const auto &row:configurationRows(settings)) {
      const auto value=definition(row.toObject());
      if (unique.contains(value)) throw std::runtime_error("Each tracking configuration must be unique.");
      unique.insert(value); result+="GMAT "+key+" = "+value+";\n";
   }
   return result;
}
TrackingConfigDialog::TrackingConfigDialog(GmatBase &object,const QString &pending,QWidget *parent) : QDialog(parent)
{
   setObjectName("trackingConfigDialog"); setWindowTitle("Tracking configurations — "+QString::fromStdString(object.GetName())); resize(740,440);
   auto *layout=new QVBoxLayout(this); auto *help=new QLabel("Each row contains an ordered signal path and one or more measurement types. Repeated participants are allowed, for example Station, Satellite, Station. OK keeps changes pending until resource Apply.",this); help->setWordWrap(true); layout->addWidget(help);
   table=new QTableWidget(0,2,this); table->setObjectName("trackingConfigs"); table->setHorizontalHeaderLabels({"Signal path (in order)","Measurement types"}); table->setSelectionBehavior(QAbstractItemView::SelectRows); table->setSelectionMode(QAbstractItemView::SingleSelection); configureTableColumns(table,{42,25}); layout->addWidget(table,1);
   for (const auto &entry:configurationRows(pending.isEmpty() ? trackingConfigurations(object) : pending)) { const auto row=entry.toObject(); addRow(row.value("path").toString(),row.value("types").toString()); }
   auto *actions=new QHBoxLayout; layout->addLayout(actions);
   auto button=[&](const QString &text,const QString &name,auto callback) { auto *action=new QPushButton(text,this); action->setObjectName(name); actions->addWidget(action); connect(action,&QPushButton::clicked,this,callback); };
   button("Add","trackingAdd",[this] { addRow(); });
   button("Remove","trackingRemove",[this] { const int row=table->currentRow(); if (row>=0) table->removeRow(row); });
   auto move=[this](int delta) { const int row=table->currentRow(),next=row+delta; if (row<0 || next<0 || next>=table->rowCount()) return; for (int column=0;column<2;++column) { auto *item=table->takeItem(row,column),*other=table->takeItem(next,column); table->setItem(row,column,other); table->setItem(next,column,item); } table->setCurrentCell(next,0); };
   button("Up","trackingUp",[move] { move(-1); }); button("Down","trackingDown",[move] { move(1); });
   button("Signal path…","trackingPath",[this] { choosePath(); });
   button("Types…","trackingTypes",[this] { chooseTypes(); });
   actions->addStretch(); error=new QLabel(this); error->setObjectName("trackingConfigError"); error->setWordWrap(true); layout->addWidget(error); connect(table,&QTableWidget::itemChanged,error,&QLabel::clear);
   connect(table,&QTableWidget::itemChanged,this,[this] { fitTableColumns(table,0); });
   fitTableColumns(table,0); QTimer::singleShot(0,this,[this] { fitTableColumns(table,0); });
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons); connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,this,[this,&object] {
      try { replaceTrackingConfigurations(object,{},settings()); accept(); }
      catch (const std::exception &failure) { error->setText(QString::fromUtf8(failure.what())); }
      catch (BaseException &failure) { error->setText(QString::fromStdString(failure.GetFullMessage())); }
   });
}
void TrackingConfigDialog::addRow(const QString &path,const QString &types)
{
   const int row=table->rowCount(); table->insertRow(row); table->setItem(row,0,new QTableWidgetItem(path)); table->setItem(row,1,new QTableWidgetItem(types)); table->setCurrentCell(row,0);
}
QString TrackingConfigDialog::settings() const
{
   QJsonArray rows; for (int row=0;row<table->rowCount();++row) rows.append(QJsonObject{{"path",table->item(row,0)->text()},{"types",table->item(row,1)->text()}});
   return QString::fromUtf8(QJsonDocument(rows).toJson(QJsonDocument::Compact));
}
void TrackingConfigDialog::choosePath()
{
   const int row=table->currentRow(); if (row<0) { error->setText("Select a tracking row first."); return; }
   QDialog dialog(this); dialog.setObjectName("trackingPathDialog"); dialog.setWindowTitle("Ordered signal path"); auto *layout=new QVBoxLayout(&dialog);
   auto *choices=new QComboBox(&dialog); choices->setObjectName("trackingParticipant"); choices->addItems(participantChoices()); layout->addWidget(choices);
   auto *list=new QListWidget(&dialog); list->setObjectName("trackingSignalPath"); list->addItems(tokens(table->item(row,0)->text())); list->setDragDropMode(QAbstractItemView::InternalMove); layout->addWidget(list);
   auto *actions=new QHBoxLayout; layout->addLayout(actions);
   auto *add=new QPushButton("Add participant",&dialog); add->setObjectName("trackingParticipantAdd"); actions->addWidget(add); connect(add,&QPushButton::clicked,&dialog,[list,choices] { if (!choices->currentText().isEmpty()) list->addItem(choices->currentText()); });
   auto *remove=new QPushButton("Remove selected",&dialog); remove->setObjectName("trackingParticipantRemove"); actions->addWidget(remove); connect(remove,&QPushButton::clicked,&dialog,[list] { delete list->takeItem(list->currentRow()); });
   auto *help=new QLabel("Drag participants to change signal order. A station or spacecraft may occur more than once. Qualified hardware participants can also be entered directly in the table.",&dialog); help->setWordWrap(true); layout->addWidget(help);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); layout->addWidget(buttons); connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject); dialog.resize(460,380);
   if (dialog.exec()==QDialog::Accepted) { QStringList names; for (int i=0;i<list->count();++i) names.append(list->item(i)->text()); table->item(row,0)->setText(names.join(", ")); }
}
void TrackingConfigDialog::chooseTypes()
{
   const int row=table->currentRow(); if (row<0) { error->setText("Select a tracking row first."); return; }
   QDialog dialog(this); dialog.setObjectName("trackingTypesDialog"); dialog.setWindowTitle("Measurement types"); auto *layout=new QVBoxLayout(&dialog); auto *list=new QListWidget(&dialog); list->setObjectName("trackingMeasurementTypes"); layout->addWidget(list);
   const auto selected=tokens(table->item(row,1)->text()); auto names=trackingMeasurementTypes(); for (const auto &name:selected) if (!names.contains(name)) names.append(name);
   for (const auto &name:names) { auto *item=new QListWidgetItem(name,list); item->setFlags(item->flags()|Qt::ItemIsUserCheckable); item->setCheckState(selected.contains(name) ? Qt::Checked : Qt::Unchecked); }
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); layout->addWidget(buttons); connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject); dialog.resize(400,420);
   if (dialog.exec()==QDialog::Accepted) { QStringList types; for (int i=0;i<list->count();++i) if (list->item(i)->checkState()==Qt::Checked) types.append(list->item(i)->text()); table->item(row,1)->setText(types.join(", ")); }
}
