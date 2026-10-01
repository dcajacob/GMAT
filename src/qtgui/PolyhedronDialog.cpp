#include "PolyhedronDialog.hpp"
#include "UserParameter.hpp"
#include "ScriptStatements.hpp"
#include "GmatBase.hpp"
#include "Moderator.hpp"
#include <QJsonObject>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QRegularExpression>
#include <QSet>
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace {
QString normalize(QString code) { code=code.trimmed(); code.remove(QRegularExpression("^GMAT\\s+")); if (code.endsWith(';')) code.chop(1); return code.remove(QRegularExpression("\\s+")); }
QString literal(QString value) { value.replace("'","''"); return "'"+value+"'"; }
}
QJsonArray polyhedronSettings(GmatBase &model)
{
   QJsonArray settings;
   for (int i=0;i<model.GetOwnedObjectCount();++i) if (auto *force=model.GetOwnedObject(i);force && force->IsOfType("PolyhedronGravityModel"))
      settings.append(QJsonObject{{"body",QString::fromStdString(force->GetStringParameter("CreateForceBody"))},{"shape",QString::fromStdString(force->GetStringParameter("ShapeFileName"))},{"density",QString::number(force->GetRealParameter("BodyDensity"),'g',17)}});
   return settings;
}
QString validatePolyhedronSettings(const QJsonArray &settings)
{
   QSet<QString> bodies;
   for (const auto &entry:settings) {
      if (!entry.isObject()) return "Invalid polyhedron settings.";
      const auto row=entry.toObject(); const auto body=row.value("body").toString();
      auto *object=Moderator::Instance()->GetConfiguredObject(body.toStdString());
      if (!object || !object->IsOfType("CelestialBody")) return "Select an existing celestial body for each contributor.";
      if (bodies.contains(body)) return "Use only one polyhedron contributor for each body.";
      bodies.insert(body);
      bool ok; const double density=row.value("density").toString().toDouble(&ok);
      if (!ok || !std::isfinite(density) || density<=0) return "Enter a finite positive density in kg/m³.";
      if (row.value("shape").toString().trimmed().isEmpty()) return "Select a shape file for each contributor.";
   }
   return {};
}
QString polyhedronScript(const QString &source,GmatBase &model,const QJsonArray &settings,const QString &firstCommand)
{
   const auto invalid=validatePolyhedronSettings(settings); if (!invalid.isEmpty()) throw std::runtime_error(invalid.toStdString());
   const auto name=QString::fromStdString(model.GetName());
   QMap<QString,QJsonObject> old,pending; QStringList bodies;
   for (const auto &entry:polyhedronSettings(model)) { const auto row=entry.toObject(); if (old.contains(row.value("body").toString())) throw std::runtime_error("The model contains duplicate polyhedron contributors. Correct the source before editing."); old[row.value("body").toString()]=row; }
   for (const auto &entry:settings) { const auto row=entry.toObject(); const auto body=row.value("body").toString(); pending[body]=row; bodies.append(body); }
   QString candidate=source,retained; QStringList properties={"PolyhedralBodies"}; QSet<QString> written;
   struct Edit { QVector<qsizetype> positions; QString value; }; QVector<Edit> legacyCreators;
   const QRegularExpression assignment("^\\s*(?:GMAT\\s+)?"+QRegularExpression::escape(name)+"\\.([A-Za-z0-9_.]+)\\s*=");
   for (const auto &statement:scriptStatements(source)) {
      if (statement.code.trimmed().startsWith("BeginMissionSequence") || (!firstCommand.isEmpty() && normalize(statement.code)==normalize(firstCommand))) break;
      const auto match=assignment.match(statement.code); if (!match.hasMatch()) continue;
      const auto property=match.captured(1);
      if (property=="UserDefined") {
         const auto value=statement.code.mid(match.capturedEnd()).trimmed();
         if (value.contains("PolyhedronGravityModel")) {
            const auto list=QRegularExpression("^\\{([^}]+)\\}\\s*;?$").match(value); if (!list.hasMatch()) throw std::runtime_error("Cannot safely edit the legacy user-force declaration.");
            auto names=list.captured(1).split(',',Qt::SkipEmptyParts); for (auto &entry:names) entry=entry.trimmed(); names.removeAll("PolyhedronGravityModel");
            auto begin=match.capturedEnd(),end=statement.code.size(); while (begin<end && statement.code[begin].isSpace()) ++begin; while (end>begin && (statement.code[end-1].isSpace() || statement.code[end-1]==';')) --end;
            legacyCreators.append({statement.positions.mid(begin,end-begin),"{"+names.join(", ")+"}"});
         }
         continue;
      }
      QString body,leaf;
      for (auto it=old.cbegin();it!=old.cend();++it) { const auto prefix="PolyhedronGravityModel."+it.key()+"."; if (property.startsWith(prefix)) { body=it.key(); leaf=property.mid(prefix.size()); break; } }
      if (QStringList{"CreateForceBody","ShapeFileName","BodyDensity"}.contains(property) && !old.isEmpty()) {
         if (old.size()!=1) throw std::runtime_error("Use body-qualified polyhedron settings when configuring multiple contributors.");
         body=old.firstKey(); leaf=property;
      }
      if (body.isEmpty()) continue;
      properties.append(property);
      if (!pending.contains(body)) continue;
      const QString key=leaf=="ShapeFileName" ? "shape" : leaf=="BodyDensity" ? "density" : QString();
      if (!key.isEmpty() && old.value(body).value(key)!=pending.value(body).value(key)) continue;
      // Retain existing expressions and unknown settings, without filling in
      // implicit defaults of this or unrelated forces. Legacy aliases acquire
      // a body prefix so adding another contributor cannot change ownership.
      QString code=statement.code.trimmed(); const auto start=code.indexOf('=');
      if (start<0) throw std::runtime_error("Cannot locate the polyhedron assignment.");
      if (!property.startsWith("PolyhedronGravityModel.")) code="GMAT "+name+".PolyhedronGravityModel."+body+"."+leaf+" "+code.mid(start);
      if (!code.endsWith(';')) code+=';'; retained+=code+'\n'; written.insert(body+"."+leaf);
   }
   for (auto it=legacyCreators.crbegin();it!=legacyCreators.crend();++it) {
      for (auto position=it->positions.crbegin();position!=it->positions.crend();++position) candidate.remove(*position,1);
      candidate.insert(it->positions.first(),it->value);
   }
   QString block="GMAT "+name+".PolyhedralBodies = {"+bodies.join(", ")+"};\n"+retained;
   for (const auto &body:bodies) {
      const auto row=pending.value(body); const auto prefix="GMAT "+name+".PolyhedronGravityModel."+body+".";
      if (!written.contains(body+".CreateForceBody")) block+=prefix+"CreateForceBody = "+body+";\n";
      for (const auto &field:QStringList{"ShapeFileName","BodyDensity"}) {
         const auto key=field=="ShapeFileName" ? "shape" : "density";
         if (written.contains(body+"."+field) || (old.contains(body) && old.value(body).value(key)==row.value(key))) continue;
         block+=prefix+field+" = "+(field=="ShapeFileName" ? literal(row.value(key).toString()) : row.value(key).toString())+";\n";
      }
   }
   properties.removeDuplicates(); return setConfigurationBlock(candidate,name,properties,block,firstCommand);
}
QString polyhedronSettingsError(GmatBase &model,const QJsonArray &settings)
{
   const auto actual=polyhedronSettings(model); if (actual.size()!=settings.size()) return "The model did not retain exactly the requested polyhedron contributors.";
   QMap<QString,QJsonObject> expected; for (const auto &entry:settings) expected[entry.toObject().value("body").toString()]=entry.toObject();
   for (const auto &entry:actual) {
      const auto row=entry.toObject(),want=expected.value(row.value("body").toString());
      if (want.isEmpty() || row.value("shape")!=want.value("shape") || row.value("density").toString().toDouble()!=want.value("density").toString().toDouble()) return "The model did not retain the requested body, shape and density.";
      expected.remove(row.value("body").toString());
   }
   if (!expected.isEmpty()) return "The model did not retain all requested bodies.";
   for (int i=0;i<model.GetOwnedObjectCount();++i) if (auto *force=model.GetOwnedObject(i);force && force->IsOfType("PolyhedronGravityModel")) if (!force->Validate()) return "The polyhedron contributor rejected its configuration.";
   return {};
}
PolyhedronDialog::PolyhedronDialog(const QJsonArray &settings,QWidget *parent) : QDialog(parent)
{
   setObjectName("polyhedronDialog"); setWindowTitle("Polyhedron gravity"); resize(850,480);
   auto *layout=new QVBoxLayout(this); auto *note=new QLabel("Add one contributor per celestial body. Remove a row to remove that contributor. Changes stay pending until you Apply the force model.",this); note->setWordWrap(true); layout->addWidget(note);
   table=new QTableWidget(0,3,this); table->setObjectName("polyhedronTable"); table->setHorizontalHeaderLabels({"Body","Shape file (coordinates in km)","Density (kg/m³)"}); table->setSelectionBehavior(QAbstractItemView::SelectRows); table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive); table->horizontalHeader()->setStretchLastSection(false); table->setColumnWidth(0,150); table->setColumnWidth(1,470); table->setColumnWidth(2,160); layout->addWidget(table);
   for (const auto &entry:settings) { const auto row=entry.toObject(); addRow(row.value("body").toString(),row.value("shape").toString(),row.value("density").toString()); }
   auto *actions=new QHBoxLayout; layout->addLayout(actions);
   const auto button=[&](const QString &text,const QString &name) { auto *item=new QPushButton(text,this); item->setObjectName(name); actions->addWidget(item); return item; };
   connect(button("Add contributor","polyhedronAdd"),&QPushButton::clicked,this,[this] { addRow(); error->clear(); });
   connect(button("Remove selected","polyhedronRemove"),&QPushButton::clicked,this,[this] { const auto selected=table->selectionModel()->selectedRows(); QVector<int> rows; for (const auto &index:selected) rows.append(index.row()); std::sort(rows.begin(),rows.end(),std::greater<int>()); for (const auto row:rows) table->removeRow(row); error->setText(rows.isEmpty() ? "Select a contributor to remove." : QString()); });
   connect(button("Browse shape…","polyhedronBrowse"),&QPushButton::clicked,this,[this] { const auto row=table->currentRow(); if (row<0) { error->setText("Select a contributor first."); return; } const auto path=QFileDialog::getOpenFileName(this,"Select closed triangle mesh",table->item(row,1)->text(),"Shape files (*.txt *.tab);;All files (*)"); if (!path.isEmpty()) table->item(row,1)->setText(path); });
   actions->addStretch(); error=new QLabel(this); error->setObjectName("polyhedronError"); error->setWordWrap(true); layout->addWidget(error);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   connect(table,&QTableWidget::itemChanged,error,&QLabel::clear);
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons->button(QDialogButtonBox::Ok),&QPushButton::clicked,this,[this] {
      QJsonArray pending; for (int row=0;row<table->rowCount();++row) pending.append(QJsonObject{{"body",qobject_cast<QComboBox *>(table->cellWidget(row,0))->currentText()},{"shape",table->item(row,1)->text().trimmed()},{"density",table->item(row,2)->text().trimmed()}});
      const auto invalid=validatePolyhedronSettings(pending); if (!invalid.isEmpty()) { error->setText(invalid); return; }
      accepted=pending; accept();
   });
}
void PolyhedronDialog::addRow(const QString &body,const QString &shape,const QString &density)
{
   const auto row=table->rowCount(); table->insertRow(row); auto *choice=new QComboBox(table);
   for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::CELESTIAL_BODY)) choice->addItem(QString::fromStdString(name));
   if (!body.isEmpty()) choice->setCurrentText(body); table->setCellWidget(row,0,choice); table->setItem(row,1,new QTableWidgetItem(shape)); table->setItem(row,2,new QTableWidgetItem(density)); table->setCurrentCell(row,1); table->selectRow(row);
   connect(choice,&QComboBox::currentTextChanged,this,[this] { if (error) error->clear(); });
}
