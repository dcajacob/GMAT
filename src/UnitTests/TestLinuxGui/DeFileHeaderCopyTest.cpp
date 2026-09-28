#include "DeFile.hpp"
#include <cassert>
#include <cstring>

struct HeaderAccess : DeFile
{
   using DeFile::recOneData;
};

int main()
{
   HeaderAccess::recOneData source;
   // Both fields can occupy every byte without a terminating null character.
   std::memset(source.label, 'L', sizeof(source.label));
   std::memset(source.constName, 'C', sizeof(source.constName));
   source.numConst = 400;
   source.DENUM = 405;
   source.AU = 149597870.7;

   HeaderAccess::recOneData copied(source);
   HeaderAccess::recOneData assigned;
   assigned = source;
   assigned = assigned;

   for (const auto *record : {&copied, &assigned})
   {
      assert(std::memcmp(record->label, source.label, sizeof(source.label)) == 0);
      assert(std::memcmp(record->constName, source.constName, sizeof(source.constName)) == 0);
      assert(record->numConst == source.numConst);
      assert(record->DENUM == source.DENUM);
      assert(record->AU == source.AU);
   }
}
