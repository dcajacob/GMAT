#include "FileComparison.hpp"
#include "FileUtil.hpp"
#include "StringUtil.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QStringDecoder>
#include <QTextStream>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace {
struct Cancelled {};
class Lines
{
public:
   Lines(const QString &path,const std::atomic_bool &cancel):file(path),cancel(cancel),initial(path)
   {
      if (!initial.isFile() || !file.open(QIODevice::ReadOnly))
         throw std::runtime_error(("Cannot open file: "+path+" — "+file.errorString()).toStdString());
   }
   bool next(QString &line,bool skipBlank)
   {
      for (;;) {
         if (cancel.load()) throw Cancelled();
         if (file.atEnd()) return false;
         QByteArray bytes;
         // Read long lines in interruptible chunks. Never hold an entire report.
         while (!file.atEnd()) {
            if (cancel.load()) throw Cancelled();
            const auto block=file.readLine(64*1024);
            if (file.error()!=QFileDevice::NoError) fail("Read failed");
            bytes+=block;
            if (bytes.size()>16*1024*1024) fail("Line exceeds 16 MiB");
            if (block.endsWith('\n')) break;
         }
         ++number;
         if (number==1 && bytes.startsWith("\xef\xbb\xbf")) bytes.remove(0,3);
         if (bytes.endsWith('\n')) bytes.chop(1);
         if (bytes.endsWith('\r')) bytes.chop(1);
         QStringDecoder decoder(QStringDecoder::Utf8,QStringConverter::Flag::Stateless);
         line=decoder(bytes);
         if (decoder.hasError() || line.contains(QChar(0))) fail("File is not UTF-8 text");
         if (!skipBlank || !line.trimmed().isEmpty()) return true;
      }
   }
   void unchanged()
   {
      const QFileInfo now(file.fileName());
      if (!now.exists() || now.size()!=initial.size() || now.lastModified()!=initial.lastModified()) fail("File changed during comparison; retry");
   }
   [[noreturn]] void fail(const QString &message) const
   { throw std::runtime_error(QString("%1: %2 (line %3)").arg(message,file.fileName()).arg(number).toStdString()); }
   qint64 number=0;
private:
   QFile file;
   const std::atomic_bool &cancel;
   QFileInfo initial;
};
void finiteNumbers(Lines &input,const QString &line);
bool dataLine(Lines &input,QString &line,RealArray &values,bool &started)
{
   while (input.next(line,true)) {
      finiteNumbers(input,line);
      const bool parsed=GmatFileUtil::GetRealColumns(line.toStdString(),values) && !values.empty();
      if (!parsed) {
         if (!started) continue; // wx skips headers before the first data row.
         input.fail("Invalid numeric data row");
      }
      for (double value:values) if (!std::isfinite(value)) input.fail("Non-finite numeric data");
      started=true; return true;
   }
   return false;
}
void finiteNumbers(Lines &input,const QString &line)
{
   const auto tokens=GmatStringUtil::SeparateBy(GmatStringUtil::RemoveInlineComment(line.toStdString(),"%")," ,:\t",true);
   for (const auto &token:tokens) {
      Real value;
      const auto lower=QString::fromStdString(token).toLower();
      if ((GmatStringUtil::ToReal(token,value) && !std::isfinite(value)) ||
          QStringList{"nan","+nan","-nan","inf","+inf","-inf","infinity","+infinity","-infinity"}.contains(lower))
         input.fail("Non-finite numeric data");
   }
}
}

QStringList comparisonBaseFiles(const QString &directory,const QString &prefix)
{
   QStringList names;
   for (const auto &name:QDir(directory).entryList(QDir::Files,QDir::Name)) {
      const auto suffix=QFileInfo(name).suffix();
      if (name!="GmatLog.txt" && name.startsWith(prefix) &&
          QStringList{"txt","report","data","script","eph","oem","e","truth"}.contains(suffix)) names.append(name);
   }
   return names;
}

FileCompareResult compareFiles(const QVector<FileComparePair> &pairs,
   const FileCompareOptions &options,const QString &resultPath,
   const std::atomic_bool &cancel,const std::function<void(int,int)> &progress)
{
   FileCompareResult result;
   if (pairs.isEmpty()) { result.error="No files to compare."; return result; }
   if (!std::isfinite(options.tolerance) || options.tolerance<0) { result.error="Tolerance must be finite and nonnegative."; return result; }
   QFile report(resultPath);
   if (!report.open(QIODevice::WriteOnly|QIODevice::Truncate)) { result.error="Cannot create comparison results: "+report.errorString(); return result; }
   QTextStream output(&report); output.setEncoding(QStringConverter::Utf8);
   output<<"GMAT Qt file comparison\nMode: "<<QStringList{"Text lines","Numeric lines","Numeric columns"}.at(int(options.mode))
         <<"\nAbsolute tolerance: "<<QString::number(options.tolerance,'g',17)<<"\n\n";
   try {
      for (int pair=0;pair<pairs.size();++pair) {
         if (cancel.load()) throw Cancelled();
         const auto &paths=pairs[pair];
         output<<"=== Comparison "<<pair+1<<" of "<<pairs.size()<<" ===\nBaseline: "<<paths.baseline<<"\nCandidate: "<<paths.candidate<<"\n";
         try {
            Lines baseline(paths.baseline,cancel),candidate(paths.candidate,cancel);
            qint64 rows=0,differences=0;
            QString left,right;
            RealArray a,b,maxima;
            bool startedA=false,startedB=false;
            for (;;) {
               bool hasA,hasB;
               if (options.mode==FileCompareMode::NumericColumns) {
                  hasA=dataLine(baseline,left,a,startedA); hasB=dataLine(candidate,right,b,startedB);
               } else {
                  const bool skip=options.mode==FileCompareMode::NumericLines || options.skipBlankLines;
                  hasA=baseline.next(left,skip); hasB=candidate.next(right,skip);
               }
               if (!hasA && !hasB) break;
               ++rows;
               bool equal=hasA && hasB;
               if (equal && options.mode==FileCompareMode::Text) equal=left==right;
               else if (equal && options.mode==FileCompareMode::NumericLines) {
                  finiteNumbers(baseline,left); finiteNumbers(candidate,right);
                  Real difference;
                  equal=GmatFileUtil::CompareLines(left.toStdString(),right.toStdString(),difference,options.tolerance);
               } else if (equal && options.mode==FileCompareMode::NumericColumns) {
                  equal=a.size()==b.size();
                  maxima.resize(std::max(maxima.size(),std::min(a.size(),b.size())),0);
                  for (size_t column=0;column<std::min(a.size(),b.size());++column) {
                     const double difference=std::abs(a[column]-b[column]);
                     maxima[column]=std::max(maxima[column],difference);
                     if (difference>options.tolerance) equal=false;
                  }
               }
               if (!equal) {
                  ++differences;
                  output<<"Row "<<rows<<" (baseline line "<<baseline.number<<", candidate line "<<candidate.number<<")\n"
                        <<"  Baseline: "<<(hasA ? left : "<end of file>")<<"\n"
                        <<"  Candidate: "<<(hasB ? right : "<end of file>")<<"\n";
               }
               if (output.status()!=QTextStream::Ok) throw std::runtime_error("Could not write comparison results.");
            }
            baseline.unchanged(); candidate.unchanged();
            if (options.mode==FileCompareMode::NumericColumns && (!startedA || !startedB)) throw std::runtime_error("No numeric data records found.");
            if (!maxima.empty()) {
               output<<"Column   Maximum absolute difference   Above tolerance\n";
               for (size_t column=0;column<maxima.size();++column)
                  output<<column+1<<"   "<<QString::number(maxima[column],'g',17)<<"   "<<(maxima[column]>options.tolerance ? "yes" : "no")<<"\n";
            }
            output<<"Rows compared: "<<rows<<"\nDifferent rows: "<<differences<<"\n"<<(differences ? "DIFFERENT" : "EQUAL")<<"\n\n";
            ++result.compared; if (differences) ++result.different;
         } catch (const std::exception &error) { ++result.errors; output<<"ERROR: "<<QString::fromUtf8(error.what())<<"\n\n"; }
         if (progress) progress(pair+1,pairs.size());
      }
   } catch (const Cancelled &) { result.cancelled=true; }
   output<<"Summary: "<<result.compared<<" compared, "<<result.different<<" different, "<<result.errors<<" errors.\n";
   output.flush();
   if (output.status()!=QTextStream::Ok || report.error()!=QFileDevice::NoError) result.error="Could not write complete comparison results.";
   return result;
}
