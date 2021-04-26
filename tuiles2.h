#ifndef TUILES2_H
#define TUILES2_H
#include "rapidjson/rapidjson.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidxml/rapidxml.hpp"
#include "iostream"
#include <sstream>
#include <fstream>
#include "date.h"

#include <boost/filesystem.hpp>
#include "./libzippp/src/libzippp.h"

#include <execution>

using namespace libzippp;

using namespace rapidjson;
using namespace rapidxml;
using namespace date;

class tuileS2;
class catalogue;

year_month_day ymdFromString(std::string date);
// sert pour le téléchargement d'une tuile
class tuileS2
{
public:
    tuileS2():mCloudCover(0),HotSpotDetected(1),RainDetected(1),SunGlintDetected(1),SnowPercent(100),mXmin(0.0), mYmin(0.0), mXmax(0.0),mYmax(0.0){}
    std::string mProd,mFeature_id,mAcqDate,mProdDate,mPubDate;
    int mCloudCover;
    std::string archiveName, decompressDirName, outputDirName,interDirName;
    void cat(){std::cout << "produit " << mProd << " , id " << mFeature_id << ", date "<< mAcqDate << ", cloudcover " << mCloudCover << std::endl; }
    void catQual(){std::cout << "mCloudCover " << mCloudCover << " , HotSpotDetected " << HotSpotDetected << ", RainDetected "<< RainDetected << ", SunGlintDetected " << SunGlintDetected << ", SnowPercent " << SnowPercent<< std::endl;
                  std::cout << "mTile " << mTile << " , mOrbitN " << mOrbitN<< ", EPSG"<< mEPSG << ", date" << mAcqDate << ", Sat " << mSat<< ", ULX " << mXmin << ", ULY " << mYmin <<  std::endl;
                  if (mEPSG!=32631){
                    std::cout << "\n attention, epsg code different de 32631 \n" <<std::endl;

                  }
                  }

    // quality index
    bool HotSpotDetected,RainDetected,SunGlintDetected;
    int SnowPercent;

    // autre du xml
    std::string mTile, mSat;
    int mOrbitN, mEPSG;

    year_month_day  mDate;

    double mXmin, mYmin, mXmax,mYmax; // upper left X and Y

    void download();
    void nettoyeArchive();
    void readXML();
    void decompresse();
    bool pretraitementDone();
    void removeArchive();
    void wrap();
    // masque généraux, nuages et no data (edge)
    void masque();

    // bandes à 20 m, 8A, 11 et 12
    void resample();
    void computeCR();

    void masqueSpecifique();
    std::string getRasterMasqSecName();
    std::string getRasterMasqGenName(int resol);

    std::string getRasterCRName();
private:
};

// au départ d'une requête au serveur theia
class catalogue
{
public:
    catalogue(std::string aJsonFile);
private:
    std::vector<tuileS2> mVProduts;
    void summary(){
        for (tuileS2 t : mVProduts){t.cat();}
        std::cout << " Nombre de produits ok ; " << countValid() << std::endl;
    }
    // comptage des produits avec cloudcover ok
    int countValid();
};




#endif // TUILES2_H
