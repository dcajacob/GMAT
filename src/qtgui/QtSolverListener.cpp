#include "QtSolverListener.hpp"
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPointer>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QVBoxLayout>
#include <QMap>
#include <cmath>

class QtSolverListener final : public ISolverListener
{
public:
   QPointer<QMdiSubWindow> window;
   QPointer<QTableWidget> table;
   QPointer<QLabel> status;
   QMap<QString,int> rows;
   void value(const QString &kind,const std::string &name,const QString &current,
              const QString &desired={},const QString &residual={}) {
      if (!table) return;
      if (status) { status->setText("Iterating…"); status->setProperty("converged",QVariant()); }
      const auto key=kind+"\n"+QString::fromStdString(name);
      if (!rows.contains(key)) { rows[key]=table->rowCount(); table->insertRow(table->rowCount()); }
      const QStringList cells={kind,QString::fromStdString(name),current,desired,residual};
      for (int col=0;col<cells.size();++col) {
         auto *item=new QTableWidgetItem(cells[col]); item->setFlags(item->flags() & ~Qt::ItemIsEditable);
         table->setItem(rows[key],col,item);
      }
   }
   void VariabledChanged(std::string name,Real number) override { value("Variable",name,QString::number(number,'g',16)); }
   void VariabledChanged(std::string name,std::string &text) override { value("Variable",name,QString::fromStdString(text)); }
   void ObjectiveChanged(std::string name,Real number) override { value("Objective",name,QString::number(number,'g',16)); }
   void ConstraintChanged(std::string name,Real desired,Real current,Integer condition) override {
      value(condition==0 ? "Goal =" : condition<0 ? "Goal ≤" : "Goal ≥",name,QString::number(current,'g',16),
         QString::number(desired,'g',16),QString::number(condition==1 ? desired-current : current-desired,'g',16));
   }
   void Convergence(bool converged,std::string info) override {
      if (status) {
         status->setText((converged ? "Converged" : "Did not converge")+
            (info.empty() ? QString() : " — "+QString::fromStdString(info)));
         status->setProperty("converged",converged);
      }
   }
};

QtSolverListenerManager::QtSolverListenerManager(QMdiArea *area) : workspace(area) {}
QtSolverListenerManager::~QtSolverListenerManager()=default;
void QtSolverListenerManager::missionFinished(bool stopped,bool failed)
{
   for (auto &entry:listeners) if (entry.second->status && !entry.second->status->property("converged").isValid())
      entry.second->status->setText(stopped ? "Mission stopped" : failed ? "Mission failed" : "Finished without a convergence report");
}
ISolverListener *QtSolverListenerManager::CreateSolverListener(const std::string &name,const std::string &,
      Real x,Real y,Real width,Real height,bool maximized)
{
   auto &listener=listeners[name];
   if (!listener) listener=std::make_unique<QtSolverListener>();
   if (!listener->window) {
      auto *panel=new QWidget;
      auto *layout=new QVBoxLayout(panel);
      listener->status=new QLabel("Iterating…",panel); listener->status->setObjectName("solverStatus");
      listener->status->setWordWrap(true); layout->addWidget(listener->status);
      listener->table=new QTableWidget(panel); listener->table->setObjectName("solverProgress");
      listener->table->setColumnCount(5);
      listener->table->setHorizontalHeaderLabels({"Type","Name","Current","Desired","Residual"});
      listener->table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
      listener->table->horizontalHeader()->setStretchLastSection(true);
      layout->addWidget(listener->table);
      listener->window=workspace->addSubWindow(panel);
      listener->window->setAttribute(Qt::WA_DeleteOnClose);
      listener->window->setWindowTitle("Solver — "+QString::fromStdString(name).section('\n',0,0));
      listener->window->resize(740,320);
      if (std::isfinite(width) && std::isfinite(height) && width>0 && height>0 && width<=1 && height<=1)
         listener->window->resize(std::max(300,int(width*workspace->width())),std::max(180,int(height*workspace->height())));
      if (std::isfinite(x) && std::isfinite(y) && x>=0 && x<=1 && y>=0 && y<=1)
         listener->window->move(int(x*workspace->width()),int(y*workspace->height()));
   }
   listener->rows.clear(); listener->table->setRowCount(0);
   listener->status->setText("Iterating…"); listener->status->setProperty("converged",QVariant());
   if (maximized) listener->window->showMaximized(); else listener->window->show();
   return listener.get();
}
