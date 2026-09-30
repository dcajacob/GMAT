#pragma once
#include <QString>
#include <QStringList>
#include <QVector>
#include <atomic>
#include <functional>

enum class FileCompareMode { Text, NumericLines, NumericColumns };
struct FileComparePair { QString baseline, candidate; };
struct FileCompareOptions
{
   FileCompareMode mode=FileCompareMode::Text;
   double tolerance=1e-6;
   bool skipBlankLines=false;
};
struct FileCompareResult
{
   int compared=0, different=0, errors=0;
   bool cancelled=false;
   QString error;
};
// Streams complete results to a file, independent of the UI and mission state.
FileCompareResult compareFiles(const QVector<FileComparePair> &pairs,
   const FileCompareOptions &options,const QString &resultPath,
   const std::atomic_bool &cancel,const std::function<void(int,int)> &progress={});
QStringList comparisonBaseFiles(const QString &directory,const QString &prefix);
