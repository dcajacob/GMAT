#include "ReportViewer.hpp"
#include "FindReplaceDialog.hpp"
#include <QPlainTextEdit>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QFontDatabase>
#include <QSignalBlocker>
#include <QShortcut>
#include <algorithm>
#include <limits>

ReportViewer::ReportViewer(const QString &path,const QString &name,QWidget *parent)
   : QWidget(parent),path(path)
{
   auto *layout=new QVBoxLayout(this); layout->setContentsMargins(4,4,4,4);
   auto *controls=new QHBoxLayout; layout->addLayout(controls);
   auto button=[&](const QString &label,const QString &id) { auto *b=new QPushButton(label,this); b->setObjectName(id); controls->addWidget(b); return b; };
   first=button("First","reportFirst"); previous=button("Previous","reportPrevious");
   pageNumber=new QSpinBox(this); pageNumber->setObjectName("reportPage"); pageNumber->setPrefix("Page "); pageNumber->setKeyboardTracking(false); pageNumber->setRange(1,1); controls->addWidget(pageNumber);
   next=button("Next","reportNext"); last=button("Last","reportLast");
   auto *reload=button("Reload","reportReload"),*find=button("Find…","reportFind"); controls->addStretch();
   status=new QLabel(this); status->setObjectName("reportPageStatus"); status->setWordWrap(true); layout->addWidget(status);
   text=new QPlainTextEdit(this); text->setObjectName("report:"+name); text->setReadOnly(true);
   text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont)); text->setLineWrapMode(QPlainTextEdit::NoWrap); text->setToolTip(path); layout->addWidget(text);
   auto *search=new FindReplaceDialog(text,this);
   connect(find,&QPushButton::clicked,search,&FindReplaceDialog::openSearch);
   auto *shortcut=new QShortcut(QKeySequence::Find,this); shortcut->setContext(Qt::WidgetWithChildrenShortcut);
   connect(shortcut,&QShortcut::activated,search,&FindReplaceDialog::openSearch);
   connect(first,&QPushButton::clicked,this,[this] { loadPage(0); });
   connect(previous,&QPushButton::clicked,this,[this] { loadPage(currentPage-1); });
   connect(next,&QPushButton::clicked,this,[this] { loadPage(currentPage+1); });
   connect(last,&QPushButton::clicked,this,[this] { loadPage(std::numeric_limits<qint64>::max()); });
   connect(reload,&QPushButton::clicked,this,[this] { loadPage(currentPage); });
   connect(pageNumber,&QSpinBox::valueChanged,this,[this](int value) { loadPage(value-1); });
   first->setEnabled(false); previous->setEnabled(false); next->setEnabled(false); last->setEnabled(false);
   loadPage(0);
}
void ReportViewer::loadPage(qint64 requested)
{
   QFile file(path);
   auto fail=[&](const QString &message) { status->setText(message+" Previous displayed content is retained."); const QSignalBlocker block(pageNumber); pageNumber->setValue(int(currentPage+1)); };
   if (!file.open(QIODevice::ReadOnly)) { fail("Report unavailable: "+file.errorString()); return; }
   constexpr qint64 chunk=1024*1024;
   const qint64 size=file.size(),pages=(size==0 ? 1 : (size-1)/chunk+1);
   if (pages>std::numeric_limits<int>::max()) { fail("Report exceeds the supported page count."); return; }
   const qint64 page=std::clamp<qint64>(requested,0,pages-1),start=page*chunk,end=std::min(size,start+chunk);
   // Extend both edges by up to three bytes to keep valid UTF-8 characters
   // intact. The same boundary rule partitions adjacent pages without gaps.
   const qint64 readStart=std::max<qint64>(0,start-3);
   if (!file.seek(readStart)) { fail("Report seek failed: "+file.errorString()); return; }
   const auto bytes=file.read(end-readStart);
   if (file.error()!=QFileDevice::NoError || bytes.size()!=end-readStart) { fail("Report changed or could not be read. Reload to retry."); return; }
   auto boundary=[&](qint64 offset) {
      while (offset>0 && offset<bytes.size() && (static_cast<unsigned char>(bytes[offset]) & 0xc0)==0x80) --offset;
      return offset;
   };
   qint64 begin=boundary(start-readStart);
   if (begin>0 && begin<bytes.size() && bytes[begin]=='\n' && bytes[begin-1]=='\r') --begin;
   qint64 finish=bytes.size();
   if (end<size) {
      // Inspect the byte immediately after this page to detect a split codepoint.
      const auto following=file.read(1);
      if (following.size()!=1 || file.error()!=QFileDevice::NoError) { fail("Report changed or could not be read. Reload to retry."); return; }
      if (!following.isEmpty() && !bytes.isEmpty() && following[0]=='\n' && bytes.back()=='\r') --finish;
      else if (!following.isEmpty() && (static_cast<unsigned char>(following[0]) & 0xc0)==0x80) {
         finish=bytes.size()-1;
         while (finish>0 && (static_cast<unsigned char>(bytes[finish]) & 0xc0)==0x80) --finish;
      }
   }
   text->setPlainText(QString::fromUtf8(bytes.mid(begin,finish-begin))); currentPage=page;
   { const QSignalBlocker block(pageNumber); pageNumber->setRange(1,int(std::min<qint64>(pages,std::numeric_limits<int>::max()))); pageNumber->setValue(int(page+1)); }
   first->setEnabled(page>0); previous->setEnabled(page>0); next->setEnabled(page+1<pages); last->setEnabled(page+1<pages);
   status->setText(QString("Page %1 of %2 · %3 bytes total. Rows may continue on adjacent pages. Find searches this page.").arg(page+1).arg(pages).arg(size));
}
