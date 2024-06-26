#ifndef TS1POS_H
#define TS1POS_H
#include "date.h"
#include <iostream>
#include "map"
#include "tuiles2OneDate.h"
#include "tuiles2onedatesco.h"
#include "algorithm"
using namespace date;

extern bool mDebug;

bool isScolyte(int x);

class tuileS2OneDate;
class tuileS2OneDateSco;
class pts;

class TS1Pos;// contient un vecteur de valeur individuelle, un vecteur de date, un vecteur pour stoquer l'analyse temporelle

class TS1PosTest;// beaucoup plus complêt que TS1Pos, car contient les valeurs de chaque bande pour cette position. Sert à débugger et à visualiser toutes les valeurs qui nous intéressent pour une position

// série temporelle pour une position donnée (un pixel)
class TS1Pos{
    public:
    TS1Pos(int aU, int aV, std::vector<int> * aVYears, int n, std::vector<tuileS2OneDateSco*> aVTuiles):mU(aU),mV(aV),mVDates(n,NULL),mVEtat(n,0),mVEtatFin(n,0),c(0),mVPtrTS2(aVTuiles){//,mVYs(aVYears)
        for (int y : * aVYears){
            mVRes.emplace(std::make_pair(y,0));
        }
    }

    void add1Date(year_month_day * ymd,int code){
       // if (mDebug){std::cout << "TS1Pos::add1Date" << std::endl;}
        mVDates.at(c)=ymd;
        mVEtat.at(c)=code;
        c++;
    }

    void writeIntermediateRes1pos();

    // effectue l'analyse ; détection de plusieurs dates consécutives avec état dépérissant ou sol nu
    void analyse();
    // enlève les code 0
    void nettoyer();
    void restrictRetourNorm();
    void detectMelange();

     void detectStresseEtRetour();
    //simplification des vecteurs pour supprimer les dates consécutives qui ont le même état et ne garder que la date moyenne
    void concateneEtat(std::vector<year_month_day> *aVD, std::vector<int>*  aVE, std::vector<std::vector<int> > *aVPosInit, std::vector<int> *aVDuree=NULL);
    // regarde si la valeur val est répétée 3 fois de suite dans la série temporelle, centrée sur la position pos
    bool repetition(int pos, int val);
    // je voulais utiliser cette fonction dans la partie getEtatPour annee mais je n'ai pas de position à renseigner à la fonction...
    bool repetition(int pos, int val, std::vector<int> *aVPt);
    int filtreVoisinDirect(int i, int val);
    int getEtatPourAnnee(int y);
    void printDetail();

    int getPositionFirstSco();

    int getDelaisCoupe(int y,bool firstDate=0);
    int mU, mV;
    // résultat par année ; un vecteur par an
    std::map<int,int> mVRes;
    protected:
    // données de base
    std::vector<year_month_day *> mVDates;
    // analyse temporelle ; vecteur de mm dimension
    std::vector<int> mVEtat;

    std::vector<tuileS2OneDateSco*> mVPtrTS2;

    // état final ; déterminé par comparaison des dates successives (enlever le bruit de mVEtat)
    std::vector<int> mVEtatFin;
    int c;// compteur pour savoir quantiemme date on ajoute
};

class TS1PosTest : public TS1Pos
{
public:
    TS1PosTest(std::vector<int> * aVYears, int n,std::vector<tuileS2OneDateSco*> aVTuiles, pts pt):TS1Pos(0,0,aVYears,n,aVTuiles),mVCRSWIR(n,0),mVCRSWIRNorm(n,0),mVB2(n,0),mVB3(n,0),mVB4(n,0),mVB8A(n,0),mVB11(n,0),mVB12(n,0),mVMasq(n,0),pt_(pt),mVB8(n,0),mVB5(n,0),mVB6(n,0),mVB7(n,0){}
    void nettoyer();
    void printDetail(std::string aOut="toto");
    // overload add1Date
    void add1Date(int code, tuileS2OneDate * t);
    std::map<int, std::vector<double> > * summaryByTri();

    // version plus récente qui permet de faire la synthese spectrale trimestrielle pour un point ET de donner les valeurs des bandes dates par dates, pour faire une figure du process.
    // c'est récent 2022 04 car il y a toute les bandes, pas juste cette utilisée par s2_ts Scolyte
    void summaryByTriTest();

    std::vector<int> getDateIndexForTri(int trimestre);
private:
  std::vector<double> mVCRSWIR;
  std::vector<double> mVCRSWIRNorm;
  std::vector<double> mVB2;
  std::vector<double> mVB3;
  std::vector<double> mVB4;
  std::vector<double> mVB8A;
  std::vector<double> mVB11;
  std::vector<double> mVB12;
  std::vector<double> mVMasq;

  std::vector<double> mVB8;
  std::vector<double> mVB5;
  std::vector<double> mVB6;
  std::vector<double> mVB7;

  pts pt_;
};

#endif // TS1POS_H
