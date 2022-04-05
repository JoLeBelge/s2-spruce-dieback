#ifndef cataloguePeriodPheno_H
#define cataloguePeriodPheno_H
#include "catalogue.h"
#include "tuiles2onedatepheno.h"

#include "ranger/globals.h"
#include "ranger/Forest/Forest.h"
#include "ranger/Forest/ForestClassification.h"
#include "ranger/Forest/ForestProbability.h"
#include "ranger/Forest/ForestSurvival.h"
#include "ranger/Forest/ForestRegression.h"
#include "ranger/utility/utility.h"

#include "StdAfx.h"

// octobre 2021 ; le masque ep dans les vosges nous semble vraiment bancal, donc on va refaire un masque épicéa sur base de la signature spectrale de l'épicéa dans les série tempo S2
// entrainement d'un modèle de classification des essences RF sur base d'observation de réflectance pour moyenne par trimestre pour chacune des bandes 2, 3, 4, 8, 11, 12. Jeu d'entrainement ; point de la carte compo all es de nicolas Latte.
//
// mon object hérite de catalogue, vu que c'est en effet une variante du catalogue de base

extern std::string wd;
extern std::string globTuile;
extern std::string globSuffix;

extern std::vector<std::string> vBR2;

using namespace  ranger;

// mes points
class mpt : public   OGRPoint
{
public:
   mpt(double x, double y, int code):OGRPoint(x,y),mCode(code){}
   std::string cat(){ return std::to_string(mCode)+";"+std::to_string(getX())+";"+std::to_string(getY());}
   int Code(){return mCode;}
private:
     int mCode;
};

std::string getNameOutputCompo(std::string aTuile);


class cataloguePeriodPheno : public catalogue
{
public:
    cataloguePeriodPheno(){vBR2={"5","6","7","8A","11","12"};}
    cataloguePeriodPheno(std::string aJsonFile):catalogue(aJsonFile){vBR2={"5","6","7","8A","11","12"};}

    bool openDS();
    void closeDS();

    bool openDS4RF();

    // calcule les bandes moyennes par trimestre
    void traitement();

    // applique une forêt aléatoire pour chaque pixel
    void applyRF(std::string pathRFmodel);
    int callRF(std::vector<double> aV);
    void initRF(std::string pathRFmodel);

    // renvoyer la valeur moyenne de la radiation par trimestre
    // key ; id du trimestre. val ; vecteur de radiance (1 val par bande étudiée)
    std::map<int, std::vector<double> > *getMeanRadByTri1Pt(double X, double Y);

    // plus abouti, beaucoup plus rapide surtout. point en BL72
    void getMeanRadByTriMultiPt(std::vector<mpt*> aVPt, std::string aOut);

    void syntheseTempoRadiation(std::vector<tuileS2OneDatePheno *> aVTuileS2, std::string aOutSuffix);

    std::string getNameBandPeriodPheno(std::string aPrefix, std::string aBand){return wd+"output/"+aPrefix+"_"+aBand +"_"+globTuile+"_"+globSuffix+".tif";}
    std::string getNameOutputRF(){return wd+getNameOutputCompo(globTuile);}

    // pour la R1 bien entendu. input = utm 31 N
    Pt2di getUV(double x, double y);

    bool isInMasq(const Pt2di & pt){
        bool aRes(0);
        if (getMasqVal(pt.x, pt.y)==1){aRes=1;}
        return aRes;
    }


private:
    void createMaskForTuile();
    std::vector<tuileS2OneDatePheno *> getTuileS2ForTri(int trimestre);
    std::string getNameMasque(int i=1){return  wd+"input/masque_compo_"+globTuile+"_"+globSuffix+"_R"+std::to_string(i)+".tif";}

    std::vector<tuileS2OneDatePheno *>  mVProdutsOK;

    // une map de dataset gdal contenant les résultats (pas les mm pour catalogue PeriodPheno que catalogue Sco
    // clé ; nom de la banbde. val ; dataset ptr
    std::map<std::string,GDALDataset *> mMapResults;

    std::unique_ptr<ranger::ForestClassification> forest;
};


class periodePhenoRasters;

// je vais plutôt tenter d'utilser elise pour changer de GDAL
class periodePhenoRasters
{
public:
 periodePhenoRasters(std::vector<std::string> aVBandsPath);

 std::vector<int> getVal(const Pt2di &pt, int resol=1);

 void resampleR1toR2(std::string aR1);
 std::string getR2Name(std::string aR1);

private:

  std::unique_ptr<Im2D_U_INT1>  b2;
  std::unique_ptr<Im2D_U_INT1>  b3;
  std::unique_ptr<Im2D_U_INT1>  b4;
  std::unique_ptr<Im2D_U_INT1>  b8;

  std::unique_ptr<Im2D_U_INT1>  b5;
  std::unique_ptr<Im2D_U_INT1>  b6;
  std::unique_ptr<Im2D_U_INT1>  b7;
  std::unique_ptr<Im2D_U_INT1>  b8A;
  std::unique_ptr<Im2D_U_INT1>  b11;
  std::unique_ptr<Im2D_U_INT1>  b12;

  std::unique_ptr<Im2D_U_INT1>  b2R2;
  std::unique_ptr<Im2D_U_INT1>  b3R2;
  std::unique_ptr<Im2D_U_INT1>  b4R2;
  std::unique_ptr<Im2D_U_INT1>  b8R2;
};


#endif // cataloguePeriodPheno_H
