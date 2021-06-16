#ifndef TS1POS_H
#define TS1POS_H
#include "date.h"
#include <iostream>
#include "map"
#include "tuiles2.h"
using namespace date;

class tuileS2;
class pts;

class TS1Pos;// contient un vecteur de valeur individuelle, un vecteur de date, un vecteur pour stoquer l'analyse temporelle

class TS1PosTest;// beaucoup plus complêt que TS1Pos, car contient les valeurs de chaque bande pour cette position. Sert à débugger et à visualiser toutes les valeurs qui nous intéressent pour une position

// série temporelle pour une position donnée (un pixel)
class TS1Pos{
    public:
    TS1Pos(int aU, int aV, std::vector<int> * aVYears, int n):mU(aU),mV(aV),mVDates(n,NULL),mVEtat(n,0),mVEtatFin(n,0),c(0){//,mVYs(aVYears)
        for (int y : * aVYears){
            mVRes.emplace(std::make_pair(y,0));
        }
            //int n = 3;
            // Create a vector of size n with all as 0.
            //vector<int> vect(n, 10);
    }

    void add1Date(year_month_day * ymd,int code){
        mVDates.at(c)=ymd;
        mVEtat.at(c)=code;
        //mVDates.push_back(ymd);
        //mVEtat.push_back(code);
        c++;
    }
    // effectue l'analyse ; détection de plusieurs dates consécutives avec état dépérissant ou sol nu
    void analyse();

    // enlève les code 0
    void nettoyer();

    void restrictRetourNorm();
    void detectMelange();
    //simplification des vecteurs pour supprimer les dates consécutives qui ont le même état et ne garder que la date moyenne
    void concateneEtat(std::vector<year_month_day> *aVD, std::vector<int>*  aVE, std::vector<std::vector<int> > *aVPosInit, std::vector<int> *aVDuree=NULL);

    // regarde si la valeur val est répétée 3 fois de suite dans la série temporelle, centrée sur la position pos
    bool repetition(int pos, int val);
    // je voulais utiliser cette fonction dans la partie getEtatPour annee mais je n'ai pas de position à renseigner à la fonction...
    bool repetition(int pos, int val, std::vector<int> *aVPt);
    int filtreVoisinDirect(int i, int val);
    int getEtatPourAnnee(int y);
    void printDetail();
    int mU, mV;
    // résultat par année ; un vecteur par an
    std::map<int,int> mVRes;
    protected:

    // données de base
    std::vector<year_month_day *> mVDates;
    // analyse temporelle ; vecteur de mm dimension
    std::vector<int> mVEtat;
    // état final ; déterminé par comparaison des dates successives (enlever le bruit de mVEtat)
    std::vector<int> mVEtatFin;
    // état final ; filtre à plus longue période temporelle que etatFin
    //std::vector<int> mVEtatFin2;

    int c;// compteur pour savoir quantiemme date on ajoute
    // vecteur des années à analyser, nécessaire en input car si j'ai trop de no data (nuage) je baque toute les dates de l'année
    //std::vector<int> mVYs;
};

class TS1PosTest : public TS1Pos
{
public:
    TS1PosTest(std::vector<int> * aVYears, int n, pts pt):TS1Pos(0,0,aVYears,n),mVCRSWIR(n,0),mVCRSWIRNorm(n,0),mVB2(n,0),mVB3(n,0),mVB4(n,0),mVB8A(n,0),mVB11(n,0),mVB12(n,0),pt_(pt){}
    void nettoyer();
    void printDetail();

    // overload add1Date
    void add1Date(int code, tuileS2 * t);
private:
  std::vector<double> mVCRSWIR;
  std::vector<double> mVCRSWIRNorm;
  std::vector<double> mVB2;
  std::vector<double> mVB3;
  std::vector<double> mVB4;
  std::vector<double> mVB8A;
  std::vector<double> mVB11;
  std::vector<double> mVB12;
  pts pt_;
};

#endif // TS1POS_H
