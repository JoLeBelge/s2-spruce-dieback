#include "reportings2.h"

reportingS2::reportingS2()
{

}

void reportingS2::add1Tuile(catalogueSco * aTuile){

    mMTuileAndDates.emplace(std::make_pair(aTuile->getTuileName(), aTuile->getAllDates()));
    mMTuileNbDates.emplace(std::make_pair(std::make_pair(aTuile->getTuileName(),1000), 0));

}

void reportingS2::genReport(std::string aOut){

    // compiler toutes les dates différentes
    std::ofstream out;
    out.open(aOut);
    out <<"#1=utilise la prise de vue. 0=n'utilise pas (ennuagement)\n" ;
    out <<"annee;date;" ;
    std::vector<year_month_day> allD;
    for (auto kv : mMTuileAndDates){
        for (year_month_day ymd: kv.second){
                if (std::find(allD.begin(), allD.end(), ymd) == allD.end()){allD.push_back(ymd);}
        }
       // std::cout << " tuile " << kv.first << " comprends " << kv.second.size() << std::endl;
        //std::cout << kv.first << ";" ;
        out  << kv.first << ";" ;
    }
    //std::cout << std::endl ;
    out  << "\n";
    sort(allD.begin(), allD.end());



    // maintenant on est prêt à faire notre tableau récapitulatif
    year curY(0);
      for (year_month_day ymd : allD){
          //std::cout <<ymd ;
          out <<ymd.year() <<";" << ymd;
          for (auto kv : mMTuileAndDates){
              std::string tuile=kv.first;
              if (ymd.year()!=curY){
                  curY=ymd.year();
              }

              bool e(0);
              for (year_month_day ymd2: kv.second){
                      if (ymd==ymd2){
                          e=1;
                          mMTuileNbDates.at(std::make_pair(tuile,1000))+=1;

                          std::pair<std::string,int> p(tuile,curY);
                          if (mMTuileNbDates.find(p)==mMTuileNbDates.end()){
                               mMTuileNbDates.emplace(p,1);
                          }else{
                          mMTuileNbDates.at(p)+=1;
                          }
                      }
              }
              //std::cout <<";"<< e ;
              out <<";"<< e ;
          }
           out <<"\n";
         // std::cout << std::endl ;
      }

      //maintenant j'affiche le tableau reprennant le nombre de date par année et par tuile
      for (auto kv : mMTuileNbDates){
           std::string tuile=kv.first.first;
           int y=kv.first.second;
           int nb=kv.second;
           std::string year("total");
           if (y!=1000){year=std::to_string(y);}
           out <<tuile << ";"<< year << ";" << nb << "\n";

      }
    out.close();
}
