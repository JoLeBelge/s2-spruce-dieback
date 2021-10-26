#ifndef TUILES2ONEDATEPHENO_H
#define TUILES2ONEDATEPHENO_H

#include "tuiles2OneDate.h"

class tuileS2OneDatePheno : public tuileS2OneDate
{
public:
    tuileS2OneDatePheno(tuileS2OneDate * t,std::string aNameMasqGlobR1,std::string aNameMasqGlobR2):tuileS2OneDate(t),mNameMasqGlobR1(aNameMasqGlobR1),mNameMasqGlobR2(aNameMasqGlobR2)
      ,scanLineMasq(NULL)
      ,scanLine1(NULL)
      ,scanLine2(NULL)
      ,scanLine3(NULL)
        {}

    bool openDS();
    void closeDS();

    void masque();

    //std::string getNameMasque(int i=1){return  wd+"input/masque_compo1_"+globTuile+"_R"+std::to_string(i)+".tif";}
    float * scanLineMasq;
    float * scanLine1;
    float * scanLine2;
    float * scanLine3;
    void readLines(int resol,int aRow) const;
    void initLines(int resol);
private :
    std::string getNameMasque(int i){
        std::string aRes("toto");
        switch (i){
        case 1: {aRes=mNameMasqGlobR1; break;}
        case 2: {aRes=mNameMasqGlobR2; break;  }    }
        return aRes;
    }
    std::string mNameMasqGlobR1, mNameMasqGlobR2;
};

#endif // TUILES2ONEDATEPHENO_H
