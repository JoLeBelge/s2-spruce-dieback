#ifndef cataloguePeriodPheno_H
#define cataloguePeriodPheno_H
#include "catalogue.h"
#include "tuiles2OneDate.h"
// octobre 2021 ; le masque ep dans les vosges nous semble vraiment bancal, donc on va refaire un masque épicéa sur base de la signature spectrale de l'épicéa dans les série tempo S2
// entrainement d'un modèle de classification des essences RF sur base d'observation de réflectance pour moyenne par trimestre pour chacune des bandes 2, 3, 4, 8, 11, 12. Jeu d'entrainement ; point de la carte compo all es de nicolas Latte.

// création depuis un catalogue, vecteur de moi en entrée, date min date max (optionnel)
// création depuis un fichier xml qui renseigne les chemin d'accès des bandes déjà calculée, en prévision de faire tourner le modèle random forest via une autre classe

// méthode pour le calcul de bande spectrale (se calquer sur méthode carte état san de catalogue

// finalement mon object hérite de catalogue, vu que c'est en effet une variante du catalogue précédemment codé.

extern std::string wd;
extern std::string globTuile;

class cataloguePeriodPheno : public catalogue
{
public:
    cataloguePeriodPheno(){}
    cataloguePeriodPheno(std::string aJsonFile):catalogue(aJsonFile){}

    bool openDS();
    void closeDS();

     void traitement();

    // renvoyer la valeur moyenne de la radiation par trimestre
    // key ; id du trimestre. val ; vecteur de radiance (1 val par bande étudiée)
    std::map<int, std::vector<double> > *getMeanRadByTri1Pt(double X, double Y);


    void syntheseTempoRadiation(std::vector<tuileS2OneDatePheno *> aVTuileS2, std::string aOutSuffix);

private:
    void createMaskForTuile();
    std::vector<tuileS2OneDatePheno *> getTuileS2ForTri(int trimestre);
    std::string getNameMasque(int i=1){return  wd+"input/masque_compo_"+globTuile+"_R"+std::to_string(i)+".tif";}

    std::vector<tuileS2OneDatePheno *>  mVProdutsOK;
};

#endif // cataloguePeriodPheno_H
