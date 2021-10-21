#ifndef CATALOGUE_H
#define CATALOGUE_H


#include "tuiles2OneDate.h"
#include "ts1pos.h"
#include <omp.h>
class catalogue;
class tuileS2OneDate;
class TS1Pos;

// copier le fichier de style QML pour Etat Sanitaire
void copyStyleES(std::string tifPath);

struct PointerCompare {
    bool operator()(const tuileS2OneDate* l, const tuileS2OneDate* r) {
        return !(*l < *r);
    }
};
class catalogue
{
public:
    // soit je crée un catalogue depuis un fichier json résultant d'une requete theia, principalement pour le téléchargement, soit je crée le catalogue depuis le dossier qui contient les produits intermédiaire
    catalogue(std::string aJsonFile);
    // je fait ça car j'effectue le téléchargement des données et les prétraitement sur une machine de traitement puis je prends les cartes de CRSWIR sur une autre machine, sans copier les données de base (trop lourd)
    catalogue();
    // renvoyer la valeur moyenne de la radiation par trimestre
    // key ; id du trimestre. val ; vecteur de radiance (1 val par bande étudiée)
    std::map<int, std::vector<double> > *getMeanRadByTri1Pt(double X, double Y);

    // ouvre tout les raster dataset
    bool openDS();
    void closeDS();
private:

    void createMaskForTuile();
    void traitement();
    void analyseTS();
    void analyseTSinit();
    void analyseTSTest1pixel(double X, double Y, std::string aFileOut);

    void writeRes1pos(TS1Pos * ts) const;// boh c'est pas const mais je triche
    //void anaTSOnePosition(std::vector<pDateEtat> * aVTS);


    // produit; tout les produits, mm si nuage, mm si pas téléchargé, et pas nécessairement ordonné par date
    std::vector<tuileS2OneDate *> mVProduts;
    // produit OK ; sont tous téléchargé, pas trop de nuage, vecteur ordonné par date d'acquisition
    std::vector<tuileS2OneDate *>  mVProdutsOK;

    void summary();
    // comptage des produits avec cloudcover ok
    int countValid();

    // extrait valeur de crswir et masque sol nu pour toute les dates pour une liste de points. Sert pour la calibration du modèle harmonique OLD OLD pas utilisé
    //void extractRatioForPts(std::vector<pts> * aVpts);

    int getMasqEPVal(int aCol, int aRow);
    void readMasqLine(int aRow);

    // masque pessière R1
    GDALDataset  * mDSmaskEP;
    // une map de dataset gdal contenant les résultats, une carte raster pour chaque année.
    // clé ; année. val ; dataset ptr
    std::map<int,GDALDataset *> mMapZScolTS;
    // vector des années couvertes par la TS
    std::vector<int> mYs;

    int mX,mY;
    float * scanLine;
    float * scanPix;
};

#endif // CATALOGUE_H
