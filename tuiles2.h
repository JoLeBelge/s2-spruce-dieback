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
#include <boost/spirit/include/qi.hpp>
#include "./libzippp/src/libzippp.h"

#include <execution>
#include "rasterfile.h"

namespace qi = boost::spirit::qi;

using namespace libzippp;

using namespace rapidjson;
using namespace rapidxml;
using namespace date;

class rasterFiles;
class pts;

class tuileS2;
class catalogue;

year_month_day ymdFromString(std::string date);

std::vector<pts> readPtsFile(std::string aFilePath);

std::vector<std::vector<std::string>> parseCSV2V(std::string aFileIn, char aDelim);

double getCRtheorique(year_month_day ymd);

// sert pour le téléchargement d'une tuile
class tuileS2
{
public:
    tuileS2():mCloudCover(0),HotSpotDetected(1),RainDetected(1),SunGlintDetected(1),SnowPercent(100),mXmin(0.0), mYmin(0.0), mXmax(0.0),mYmax(0.0){}

    // il faut notre copy contructor si un des membres de la classe est un unique ptr
    tuileS2(const  tuileS2&) = delete;
    tuileS2& operator=(const  tuileS2&) = delete;
    // move
    tuileS2(tuileS2 &&) = default;

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

    //crée une couche qui normalise le CR par le CR sensé être ok pour cette date ; sera plus facile à manipuler
    void normaliseCR();
    std::string getRasterCRnormName();

    void masqueSpecifique();
    std::string getRasterMasqSecName();
    std::string getRasterMasqGenName(int resol);

    std::string getRasterCRName();

    double getCRSWIR(pts & pt);
    int getMaskSolNu(pts & pt);

    std::string getDate();

    double getCRth(){return getCRtheorique(mDate);}

private:

    std::unique_ptr<rasterFiles> r_crswir;
    std::unique_ptr<rasterFiles> r_solnu;
};

// au départ d'une requête au serveur theia
class catalogue
{
public:
    catalogue(std::string aJsonFile);
private:
    std::vector<tuileS2 *> mVProduts;
    void summary(){
        for (tuileS2 * t : mVProduts){t->cat();}
        std::cout << " Nombre de produits ok ; " << countValid() << std::endl;
    }
    // comptage des produits avec cloudcover ok
    int countValid();

    // extrait valeur de crswir et masque sol nu pour toute les dates pour une liste de points. Sert pour la calibration du modèle harmonique
    void extractRatioForPts(std::vector<pts> * aVpts);

};




#endif // TUILES2_H
