#ifndef CATALOGUE_H
#define CATALOGUE_H
#include "tuiles2.h"
class catalogue;
class tuileS2;
class TS1Pos;

//inline bool operator< (const tuileS2 & t1, const tuileS2 & t2);
struct PointerCompare {
    bool operator()(const tuileS2* l, const tuileS2* r) {
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
private:

    void traitement();
    void analyseTS();
    //void anaTSOnePosition(std::vector<pDateEtat> * aVTS);
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

#endif // CATALOGUE_H