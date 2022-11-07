#include "reportings2.h"

reportingS2::reportingS2()
{

}

void reportingS2::add1Tuile(catalogueSco * aTuile){

    mMTuileAndDates.emplace(std::make_pair(aTuile->getTuileName(), aTuile->getAllDates()));
}

void reportingS2::genReport(std::string aOut){

    // compiler toutes les dates différentes

    std::vector<year_month_day> allD;
    for (auto kv : mMTuileAndDates){
        for (year_month_day ymd: kv.second){
                if (std::find(allD.begin(), allD.end(), ymd) == allD.end()){allD.push_back(ymd);}
        }
        //std::cout << " tuile " << kv.first << " comprends " << kv.second.size() << std::endl;
    }
    sort(allD.begin(), allD.end());

    // maintenant on est prêt à faire notre tableau récapitulatif
}
