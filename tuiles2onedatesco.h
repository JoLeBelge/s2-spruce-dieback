#ifndef TUILES2ONEDATESCO_H
#define TUILES2ONEDATESCO_H
#include "tuiles2OneDate.h"

extern bool overw;
extern bool mDebug;
extern std::string path_otb;
extern std::string compr_otb;
extern std::string wd;
extern std::string globTuile;
extern std::string globSuffix;
extern double seuilCR;


class tuileS2OneDateSco : public tuileS2OneDate
{
public:
    tuileS2OneDateSco(tuileS2OneDate * t):tuileS2OneDate(t){}

    void computeCR();
    //crée une couche qui normalise le CR par le CR sensé être ok pour cette date ; sera plus facile à manipuler
    void normaliseCR();

    void createRastEtat();

    // lecture ligne par ligne ; j'espère gagner du temps - finalement c'est pas là que je dois gagner du temps mais sur le traitement/classe TS1Pos
    void readCRnormLine(int aRow) const;
    double getCRnormVal(int aCol) const;
    double getCRSWIR(pts & pt);
    double getCRSWIRNorm(pts & pt);

    void computeCodeLine();
    int getCodeLine(int aCol) const;

    bool openDS();
    void closeDS();

    // lecture pixel par pixel. fonctionne bien, mais lent
    int getMasqVal(int aCol, int aRow);
    double getCRnormVal(int aCol, int aRow);

    void masqueSpecifique();

    void readMasqLine(int aRow) const;
    int getMasqVal(int aCol) const;
    // masque généraux, nuages et no data (edge)
    void masque();

    std::string getRasterCRnormName() const;
    std::string getRasterCRName();
    std::string getRasterEtatIName();
    std::string getRasterEtatFName();

    int getMaskSolNu(pts & pt);
    // pour sauver les résultats d'état - résultats intermédiaires
    GDALDataset  * mDSetatInit;
    GDALDataset  * mDSetatFinal;

private :
    std::unique_ptr<rasterFiles> r_crswir;
    std::unique_ptr<rasterFiles> r_crswirNorm;
    float * scanLineCR;
    int * scanLineCode;
    float * scanLineSolNu;

    GDALDataset  * mDScrnom;
    GDALDataset  * mDSsolnu;


    // rasterFile ; finalement je vais sans doute pas utiliser ces objets, mais plutôt directement un GDALDATASET
    // ou alors j'utilise juste pour le test sur un point donné. plus facile à écrire et à lire que charger tout les raster de la série temporelle...
    std::unique_ptr<rasterFiles> r_solnu;

};

#endif // TUILES2ONEDATESCO_H
