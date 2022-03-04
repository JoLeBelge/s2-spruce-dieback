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
#include "boost/program_options.hpp"
#include "./libzippp/src/libzippp.h"
#include <execution>
#include "rasterfile.h"

#include <libgen.h>         // dirname
#include <unistd.h>         // readlink
#include <linux/limits.h>   // PATH_MAX

namespace po = boost::program_options;
namespace qi = boost::spirit::qi;
using namespace libzippp;
using namespace rapidjson;
using namespace rapidxml;
using namespace date;

class rasterFiles;
class pts;

class tuileS2OneDate;

year_month_day ymdFromString(std::string date);

std::vector<pts> readPtsFile(std::string aFilePath);
std::vector<std::vector<std::string>> parseCSV2V(std::string aFileIn, char aDelim);

double getCRtheorique(year_month_day ymd);


std::string getNameMasqueEP(int i=1);

// sert pour le téléchargement d'une tuile

// je voulais faire une classe de base pour toutes mes applis, puis un hériatage pour tuileS2 Sco, tuileS2 composition, mais jai ma classe "catalogue" qui doit avoir comme membre un vecteur d'objet identique (entre catalogue Sco et catalogue Compo), sinon ces catalogues n'ont plus rien à partager.

class tuileS2OneDate
{
public:
    tuileS2OneDate():mCloudCover(0),HotSpotDetected(1),RainDetected(1),SunGlintDetected(1),SnowPercent(100),mXmin(0.0), mYmin(0.0), mXmax(0.0),mYmax(0.0),scanPix(NULL),mXSize(0),mYSize(0),mSuffix(""),mEPSG(0){
        //std::cout << "creation tuileS2" << std::endl;
    }

    tuileS2OneDate(tuileS2OneDate * t):mCloudCover(t->mCloudCover)
      ,HotSpotDetected(t->HotSpotDetected)
      ,RainDetected(t->RainDetected)
      ,SunGlintDetected(t->SunGlintDetected)
      ,SnowPercent(t->SnowPercent)
      ,mXmin(t->mXmin)
      , mYmin(t->mYmin)
      , mXmax(t->mXmax)
      ,mYmax(t->mYmax)
      ,scanPix(t->scanPix)
      ,mXSize(t->mXSize)
      ,mYSize(t->mYSize)
      ,decompressDirName(t->decompressDirName)
      ,interDirName(t->interDirName)
    ,outputDirName(t->outputDirName)
    ,mDate(t->mDate)
    ,mProd(t->mProd)
    ,mAcqDate(t->mAcqDate)
    ,mFeature_id(t->mFeature_id)
    ,mPtrDate(t->mPtrDate)
    ,mSuffix(t->mSuffix)
    ,mEPSG(t->mEPSG){}

    // il faut notre copy contructor si un des membres de la classe est un unique ptr
    tuileS2OneDate(const  tuileS2OneDate&) = delete;
    tuileS2OneDate& operator=(const  tuileS2OneDate&) = delete;
    // move
    tuileS2OneDate(tuileS2OneDate &&) = default;

    std::string mProd,mFeature_id,mAcqDate,mProdDate;//,mPubDate;
    // attention, feature_id est uniquement utilisé par et pour la requete de téléchargement.

    int mCloudCover;
    std::string archiveName, decompressDirName, outputDirName,interDirName;
    void cat(){std::cout << "produit " << mProd << " , id " << mFeature_id << ", date "<< mAcqDate << ", cloudcover " << mCloudCover << ", EPSG " << mEPSG <<std::endl; }
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
    // pour la résolution R1
    int mXSize,mYSize;

    int getXSize(){return mXSize;}
    int getYSize(){return mYSize;}

    year_month_day  mDate;
    year_month_day  * mPtrDate;

    double mXmin, mYmin, mXmax,mYmax; // upper left X and Y

    void download();
    void nettoyeArchive();
    void readXML();
    void readXML(std::string aXMLfile);
    void decompresse();
    bool pretraitementDone();
    void removeArchive();
    //void wrap();


    // bandes à 20 m, 8A, 11 et 12
    void resample();

    std::string getRasterR1Name(std::string numBand);
    // attention, il s'agit des bandes rééchantillonnée à 10 m!
    std::string getRasterR2Name(std::string numBand, int aR=1);
    // celle la c'est les noms des raster originaux
    std::string getOriginalRasterR2Name(std::string numBand);

    std::string getRasterMasqSecName() const;
    std::string getRasterMasqGenName(int resol=1);

    std::string getDate();

    int getEPSG(){return mEPSG;}

    double getCRth(){return getCRtheorique(mDate);}

   /* bool openDS();
    void closeDS();
    */

    double getDSVal(std::string bande,int aCol, int aRow);

    pts getUV(double x, double y);
    // retourne la position uv pour un point

    bool operator < (const tuileS2OneDate& t) const {
        days diff=date::sys_days(getymd())-date::sys_days(t.getymd());
        if (diff.count()>0){
            return 1;
        } else {
            return 0;
        }
    }

    year_month_day getymd() const {
        return mDate;
    }

    year_month_day * const & getymdPt() const{
        return mPtrDate;
    }

    int gety() const {
        return mDate.year().y_;
    }
    std::string mSuffix;
protected:

   std::map<std::string, GDALDataset*> vDS; // pour Test1Pos ; je veux pafois faire tourner le test sur des centaines de points, là ça deviens trop long alors je créer une maps de Dataset, un pour chaque bande, affin de ne pas devoir les réouvrir à chaque position
   float * scanPix;

};


#endif // TUILES2_H
