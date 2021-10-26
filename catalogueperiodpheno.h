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

// octobre 2021 ; le masque ep dans les vosges nous semble vraiment bancal, donc on va refaire un masque épicéa sur base de la signature spectrale de l'épicéa dans les série tempo S2
// entrainement d'un modèle de classification des essences RF sur base d'observation de réflectance pour moyenne par trimestre pour chacune des bandes 2, 3, 4, 8, 11, 12. Jeu d'entrainement ; point de la carte compo all es de nicolas Latte.

// mon object hérite de catalogue, vu que c'est en effet une variante du catalogue de base

extern std::string wd;
extern std::string globTuile;

using namespace  ranger;



class cataloguePeriodPheno : public catalogue
{
public:
    cataloguePeriodPheno(){}
    cataloguePeriodPheno(std::string aJsonFile):catalogue(aJsonFile){}

    bool openDS();
    void closeDS();

    // calcule les bandes moyennes par trimestre
    void traitement();

    // applique une forêt aléatoire pour chaque pixel
    void applyRF(std::string pathRFmodel);

    // renvoyer la valeur moyenne de la radiation par trimestre
    // key ; id du trimestre. val ; vecteur de radiance (1 val par bande étudiée)
    std::map<int, std::vector<double> > *getMeanRadByTri1Pt(double X, double Y);


    void syntheseTempoRadiation(std::vector<tuileS2OneDatePheno *> aVTuileS2, std::string aOutSuffix);

    std::string getNameBandPeriodPheno(std::string aPrefix, std::string aBand){return wd+"output/"+aPrefix+"_"+aBand +"_"+globTuile+".tif";}
private:
    void createMaskForTuile();
    std::vector<tuileS2OneDatePheno *> getTuileS2ForTri(int trimestre);
    std::string getNameMasque(int i=1){return  wd+"input/masque_compo_"+globTuile+"_R"+std::to_string(i)+".tif";}

    std::vector<tuileS2OneDatePheno *>  mVProdutsOK;

    // une map de dataset gdal contenant les résultats, une carte raster pour chaque année.
    // clé ; année. val ; dataset ptr
    //std::map<int,GDALDataset *> mMapResults;
    std::map<std::string,GDALDataset *> mMapResults;
};

#endif // cataloguePeriodPheno_H
