#ifndef CATALOGUESCO_H
#define CATALOGUESCO_H
#include "catalogue.h"
#include "tuiles2onedatesco.h"

class catalogueSco;


class catalogueSco : public catalogue
{
public:
    catalogueSco(){}
    catalogueSco(std::string aJsonFile):catalogue(aJsonFile){}
    void traitement();

    std::vector<year_month_day> getAllDates();
private:

    void createMaskForTuile();
    void analyseTS();
    void analyseTSinit();
    void analyseTSTest1pixel(double X, double Y, std::string aFileOut);
    void writeRes1pos(TS1Pos * ts) const;// boh c'est pas const mais je triche

    void writeIntermediateRes1pos(TS1Pos * ts);
    //void anaTSOnePosition(std::vector<pDateEtat> * aVTS);
    // produit OK ; sont tous téléchargé, pas trop de nuage, vecteur ordonné par date d'acquisition
    std::vector<tuileS2OneDateSco *>  mVProdutsOK;


    // extrait valeur de crswir et masque sol nu pour toute les dates pour une liste de points. Sert pour la calibration du modèle harmonique OLD OLD pas utilisé
    //void extractRatioForPts(std::vector<pts> * aVpts);

    std::map<int,GDALDataset *> mMapResults;

    // j'ai besoin des délais de coupes et de la date de la première détection d'une attaque de scolyte ; en option
    std::map<int,GDALDataset *> mMapDelaisCoupe;
    std::map<int,GDALDataset *> mMapFirstDateSco;


    std::string getNameMasque(int i=1){return getNameMasqueEP(i);}

    std::string getNameDelaisCoupe(int y){
        return wd+"delaisCoupe_"+globTuile+"_"+globSuffix+"_"+std::to_string(y)+".tif";
    }
    std::string getNameFirstDateSco(int y){
        return wd+"FirstDateSco_"+globTuile+"_"+globSuffix+"_"+std::to_string(y)+".tif";
    }
    std::string getNameES(int y){
        return wd+"etatSanitaire_"+globTuile+"_"+globSuffix+std::to_string(y)+".tif";
    }
    // ouvre tout les raster dataset
    bool openDS();
    void closeDS();


    std::map<int,float *> mSL;
};



#endif // CATALOGUESCO_H
