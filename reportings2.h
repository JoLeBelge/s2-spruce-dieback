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

    // clé: nom tuile puis année. valeur : nombre de date
    // pour toute les années, code année de 1000
    std::map<std::pair<std::string,int>, int>  mMTuileNbDates;
};

#endif // REPORTINGS2_H
