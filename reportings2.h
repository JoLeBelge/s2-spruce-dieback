#ifndef REPORTINGS2_H
#define REPORTINGS2_H
#include "cataloguesco.h"

class reportingS2
{
public:
    reportingS2();

    void add1Tuile(catalogueSco * aTuile);
    void genReport(std::string aOut);

    // je stocke dans quoi le résultat? un tableau ou une liste des dates? ou une maps, clé=tuile, val=vecteur de date.
    std::map<std::string, std::vector<year_month_day>>  mMTuileAndDates;
};

#endif // REPORTINGS2_H
