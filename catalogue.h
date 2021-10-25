#ifndef CATALOGUE_H
#define CATALOGUE_H
class catalogue;
#include "tuiles2OneDate.h"
#include "ts1pos.h"
#include <omp.h>

class tuileS2OneDate;
class TS1Pos;

struct PointerCompare {
    bool operator()(const tuileS2OneDate* l, const tuileS2OneDate* r) {
        return !(*l < *r);
    }
};

class catalogue
{
public:
    // je fait ça car j'effectue le téléchargement des données et les prétraitement sur une machine de traitement puis je prends les cartes de CRSWIR sur une autre machine, sans copier les données de base (trop lourd)
    catalogue();
    // soit je crée un catalogue depuis un fichier json résultant d'une requete theia, principalement pour le téléchargement, soit je crée le catalogue depuis le dossier qui contient les produits intermédiaire
    catalogue(std::string aJsonFile);


    void summary();
    // comptage des produits avec cloudcover ok
    int countValid();
    void readMasqLine(int aRow);

protected:
    // produit; tout les produits, mm si nuage, mm si pas téléchargé, et pas nécessairement ordonné par date
    std::vector<tuileS2OneDate *> mVProduts;

    // une map de dataset gdal contenant les résultats, une carte raster pour chaque année.
    // clé ; année. val ; dataset ptr
    std::map<int,GDALDataset *> mMapResults;
    // vector des années couvertes par la TS
    std::vector<int> mYs;

    // masque pessière R1  pour catalogueSco ou masque forêt -non foret
    GDALDataset  * mDSmask;

    int mX,mY;
    float * scanLine;
    float * scanPix;

};


#endif // CATALOGUE_H
