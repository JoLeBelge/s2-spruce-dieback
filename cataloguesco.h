#ifndef CATALOGUESCO_H
#define CATALOGUESCO_H
#include "catalogue.h"
#include "tuiles2OneDate.h"

class catalogueSco;


class catalogueSco : public catalogue
{
public:
    catalogueSco(){}
    catalogueSco(std::string aJsonFile):catalogue(aJsonFile){}
    void traitement();
private:

    void createMaskForTuile();
    void analyseTS();
    void analyseTSinit();
    void analyseTSTest1pixel(double X, double Y, std::string aFileOut);
    void writeRes1pos(TS1Pos * ts) const;// boh c'est pas const mais je triche
    //void anaTSOnePosition(std::vector<pDateEtat> * aVTS);
    // produit OK ; sont tous téléchargé, pas trop de nuage, vecteur ordonné par date d'acquisition
    std::vector<tuileS2OneDateSco *>  mVProdutsOK;

    // extrait valeur de crswir et masque sol nu pour toute les dates pour une liste de points. Sert pour la calibration du modèle harmonique OLD OLD pas utilisé
    //void extractRatioForPts(std::vector<pts> * aVpts);

    int getMasqEPVal(int aCol, int aRow);
    std::string getNameMasque(int i=1){return getNameMasqueEP(i);}
    // ouvre tout les raster dataset
    bool openDS();
    void closeDS();
};



#endif // CATALOGUESCO_H