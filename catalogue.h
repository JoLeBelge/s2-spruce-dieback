#ifndef CATALOGUE_H
#define CATALOGUE_H
class catalogue;
#include "tuiles2OneDate.h"
#include "ts1pos.h"
#include <omp.h>
#include <sys/resource.h>

class tuileS2OneDate;
class TS1Pos;

struct PointerCompare {
    bool operator()(const tuileS2OneDate* l, const tuileS2OneDate* r) {
        return !(*l < *r);
    }
};
extern std::string d1,d2;

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
    void readMasqLine(int aRow, int aRes=1);
    int getMasqVal(int aCol, int aRow);

    int getEPSG();

    void init();


protected:
    // produit; tout les produits, mm si nuage, mm si pas téléchargé, et pas nécessairement ordonné par date
    std::vector<tuileS2OneDate *> mVProduts;

    // vector des années couvertes par la TS
    std::vector<int> mYs;

    // masque pessière R1 (et R2)  pour catalogueSco ou masque forêt -non foret
    GDALDataset  * mDSmaskR1;
    GDALDataset  * mDSmaskR2;

    int mX,mY;

    float * scanLineR1;
    float * scanLineR2;
    float * scanPix;



};


#endif // CATALOGUE_H
