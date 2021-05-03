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

#include "boost/program_options.hpp"

namespace po = boost::program_options;
namespace qi = boost::spirit::qi;

using namespace libzippp;

using namespace rapidjson;
using namespace rapidxml;
using namespace date;

class rasterFiles;
class pts;

class pDateEtat; // sert à manipuler une date accompagnée d'un code état, à savoir sol nu, pessière saine ou pessière stressée, no data
class tuileS2;
class catalogue;

year_month_day ymdFromString(std::string date);

std::vector<pts> readPtsFile(std::string aFilePath);

std::vector<std::vector<std::string>> parseCSV2V(std::string aFileIn, char aDelim);

double getCRtheorique(year_month_day ymd);

//bool comparePtrToTuileS2(tuileS2* a, tuileS2* b);

inline bool operator< (const tuileS2 & t1, const tuileS2 & t2);

struct PointerCompare {
    bool operator()(const tuileS2* l, const tuileS2* r) {
        return !(*l < *r);
    }
};

std::string getNameMasqueEP(int i=1);

class TSunePosition;// contient un vecteur de valeur individuelle, un vecteur de date, un vecteur pour stoquer l'analyse temporelle

class TSunePosition{
    public:

    private:

    // données de base
    std::vector<year_month_day> mVDates;
    std::vector<int> mVEtat;
    // analyse temporelle ; vecteur de mm dimension
    std::vector<int> mVEtat;
    // résultat par année ; un vecteur par an
    std::map<int,int> mVRes;
};

class pDateEtat{
public:

    pDateEtat(year_month_day ymd,int code):mDate(ymd),mEtat(code){}
    int getYear() const{
        int y= mDate.year().y_;
        return y;
    }

    int getEtat() const { return mEtat;}

    void cat() const {std::cout << mDate << " : " << mEtat << std::endl;}

private:
    year_month_day  mDate;
    int mEtat;

};


// sert pour le téléchargement d'une tuile
class tuileS2
{
public:
    tuileS2():mCloudCover(0),HotSpotDetected(1),RainDetected(1),SunGlintDetected(1),SnowPercent(100),mXmin(0.0), mYmin(0.0), mXmax(0.0),mYmax(0.0),scanPix(NULL),mXSize(0),mYSize(0){
        //std::cout << "creation tuileS2" << std::endl;
    }

    // il faut notre copy contructor si un des membres de la classe est un unique ptr
    tuileS2(const  tuileS2&) = delete;
    tuileS2& operator=(const  tuileS2&) = delete;
    // move
    tuileS2(tuileS2 &&) = default;

    std::string mProd,mFeature_id,mAcqDate,mProdDate;//,mPubDate;
    // attention, feature_id est uniquement utilisé par et pour la requete de téléchargement.

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
    // pour la résolution R1
    int mXSize,mYSize;

    int getXSize(){return mXSize;}
    int getYSize(){return mYSize;}

    year_month_day  mDate;

    double mXmin, mYmin, mXmax,mYmax; // upper left X and Y

    void download();
    void nettoyeArchive();
    void readXML();
    void readXML(std::string aXMLfile);
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

    bool openDS();
    void closeDS();
    int getMasqVal(int aCol, int aRow);
    double getCRnormVal(int aCol, int aRow);


    bool operator < (const tuileS2& t) const {
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

private:
    // rasterFile ; finalement je vais sans doute pas utiliser ces objets, mais plutôt directement un GDALDATASET
    std::unique_ptr<rasterFiles> r_crswir;
    std::unique_ptr<rasterFiles> r_solnu;

    GDALDataset  * mDScrnom;
    GDALDataset  * mDSsolnu;

    float * scanPix;
};

// au départ d'une requête au serveur theia
class catalogue
{
public:
    // soit je crée un catalogue depuis un fichier json résultant d'une requete theia, principalement pour le téléchargement, soit je crée le catalogue depuis le dossier qui contient les produits intermédiaire
    catalogue(std::string aJsonFile);
    // je fait ça car j'effectue le téléchargement des données et les prétraitement sur une machine de traitement puis je prends les cartes de CRSWIR sur une autre machine, sans copier les données de base (trop lourd)
    catalogue();
private:

    void traitement();

    void analyseTS();
    void anaTSOnePosition(std::vector<pDateEtat> * aVTS);
    // ouvre tout les raster dataset
    bool openDS();
    void closeDS();

    // produit; tout les produits, mm si nuage, mm si pas téléchargé, et pas nécessairement ordonné par date
    std::vector<tuileS2 *> mVProduts;
    // produit OK ; sont tous téléchargé, pas trop de nuage, vecteur ordonné par date d'acquisition
    std::vector<tuileS2 *>  mVProdutsOK;


    void summary(){
        for (tuileS2 * t : mVProduts){t->cat();}
        std::cout << " Nombre de produits ok ; " << countValid() << std::endl;
    }
    // comptage des produits avec cloudcover ok
    int countValid();

    // extrait valeur de crswir et masque sol nu pour toute les dates pour une liste de points. Sert pour la calibration du modèle harmonique
    void extractRatioForPts(std::vector<pts> * aVpts);

    int getMasqEPVal(int aCol, int aRow);

    // masque pessière R1
    GDALDataset  * mDSmaskEP;
    // une map de dataset gdal contenant les résultats, une carte raster pour chaque année.
    // clé ; année. val ; dataset ptr
    std::map<int,GDALDataset *> mMapZScolTS;

};




#endif // TUILES2_H
