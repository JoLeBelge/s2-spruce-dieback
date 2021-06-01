#include "ts1pos.h"

bool debugDetail(1); // affiche l'ensemble des valeurs de bandes en plus des codes état

void TS1Pos::analyse(){

    //nettoyer();

    for (int i(0);i<mVDates.size();i++){
        //std::cout << "date " << mVDates.at(i) << ", état " << mVEtat.at(i) << std::endl;
        if (mVEtat.at(i)==1){
            mVEtatFin.at(i)=1;
        } else if (mVEtat.at(i)==3){

            // diminution temps calcul; tout d'abord voir la valeur calulée à l'étape d'avant dans mVEtatFin. Ca diminue bien le temps de calcul.
            // check 2 dates avant et après.
            if (i>0 && mVEtatFin.at(i-1)==3){
                mVEtatFin.at(i)=3;
            } else if (repetition(i,3)){
                mVEtatFin.at(i)=3;
            } else {
                mVEtatFin.at(i)=filtreVoisinDirect(i,3);
            }
        } else if (mVEtat.at(i)==2){

            // diminution temps calcul; tout d'abor voir la valeur calculée à l'étape d'avant dans mVEtatFin.
            // check 2 dates avant et après.
            if (i>0 && mVEtatFin.at(i-1)==2){
                mVEtatFin.at(i)=2;
            } else if (repetition(i,2)){
                mVEtatFin.at(i)=2;
            } else {
                mVEtatFin.at(i)=filtreVoisinDirect(i,2);
            }
        }
    }

    // filtre ; si détection de 3 x sol nu de manière consécutif, je met toutes les dates d'après en sol nu.
    std::vector<int>::iterator p =std::find(mVEtatFin.begin(), mVEtatFin.end(), 3);
    bool conseq(0);
    int i(0);
    std::vector<int>::iterator it;
    if (p != mVEtatFin.end()){
        for (it = p; it != mVEtatFin.end(); it++){
            if (*it==3){i++;} else{i=0;}
            if (i>2){conseq=1;}
            // si j'ai détecté 3 sol nu d'affilé, je met tout les suivants en code sol nu
            if (conseq){*it=3;}
        }
    }
    // OLD OLD filtre ; si détection de 3 x stress de manière consécutif, je met toutes les dates d'après en stress sauf si sol nu.
    // non c'est pas du tout comme cela qu'il faut procéder. j'avais mal lu la slide de Dutrieux. C'est si 3x un retour sans stress consécutif après une période de stress, on considère que ce n'est pas un dépérissement mais uniquement un stress passagé. "retour à la normale"
    p =std::find(mVEtatFin.begin(), mVEtatFin.end(), 2);
    conseq=0;
    i=0;
    int j(0);
    bool retourNormale(0);
    std::vector<int> aVEtatTmp=mVEtatFin;
    //std::vector<int>::iterator it2;

    if (p != mVEtatFin.end()){
        int posInit= p - mVEtatFin.begin();
        // vérifie que j'ai 3 valeurs consécutives de stress hydrique
        //for (it = p; it != mVEtatFin.end(); it++){
        for (int pos= posInit; pos< mVEtatFin.size(); pos++){

            if (mVEtatFin.at(pos)==2){i++;} else{i=0;}
            //if (*it==2){i++;} else{i=0;}
               if (i>2){conseq=1;retourNormale=0;}
            // j'accepte un retour à la normale uniquement sur base des états initiaux, comme cela si j'ai 121212 cela signifie pas de retour à la normale, car dans etat finale cela donne 111111
            if (mVEtat.at(pos)==1){j++;} else{j=0;}
            if (j>2){retourNormale=1;conseq=0;}
            // si j'ai détecté 3 stress, je met tout les suivants en stress sauf si sol nu. Si sol nu, je met code 4
            // si retour à la normale, je change toutes les valeurs de stress précédentes pour noter qu'il s'agit d'un stress temporaire

            if (retourNormale){
                // je change les 2 valeurs précédentes que j'avais écrasées.
                for (int posAnt(pos);posAnt>posInit-1; posAnt--){
                    if (mVEtatFin.at(posAnt)==1) {aVEtatTmp.at(posAnt)=1;} else {aVEtatTmp.at(posAnt)=5;}
                }
                retourNormale=0;
            }

            if (conseq){
                if (mVEtatFin.at(pos)!=3){aVEtatTmp.at(pos)=2;} else {aVEtatTmp.at(pos)=4;}
            }
        }
    }

    mVEtatFin=aVEtatTmp;

    // les pixels qui sont un mélange de résineux et soit de sol ou de feuillus présentent un stress en hiver mais pas de stress en été (car photosynthèse du sol ou du feuillus).
    // je crée une classe supplémentaire pour les détecter. Avant ils étaient en stress temporaire en alternance avec état normal en été seulement
    detectMelange();
    // maintenant, je vais résumer l'état pour chaque année afin d'exporter des cartes annuelles.
    for (auto kv : mVRes){
        mVRes.at(kv.first)=getEtatPourAnnee(kv.first);
    }
}

void TS1Pos::detectMelange(){
    // premier état stress temporaire
    std::vector<int>::iterator p=std::find(mVEtatFin.begin(), mVEtatFin.end(), 5);
    if (p!=mVEtatFin.end()){
        std::vector<year_month_day> aVD;
        std::vector<int> aVE;
        std::vector<std::vector<int>> aVPosEtatFin;
        concateneEtat(& aVD, & aVE, &aVPosEtatFin);
        // recherche de toutes les positions ou j'ai un stress temporaire et test si c'est en hiver.
        p = aVE.begin();
        while (p != aVE.end()) {
            p = std::find(p, aVE.end(), 5);
            if (p != aVE.end()) {
                int pos= p - aVE.begin();
                // détecte si c'est en hiver
                if (aVD.at(pos).month()>month{9} | aVD.at(pos).month()<month{5}){
                    // si oui, on change les valeurs dans mEtatFinal pour passer en catégorie "mélange feuillus résineux"
                    for (int i : aVPosEtatFin.at(pos)){
                        mVEtatFin.at(i)=6;
                    }
                }
                p++;
            }
        }
    }
}

void TS1Pos::concateneEtat(std::vector<year_month_day> * aVD, std::vector<int> *aVE, std::vector<std::vector<int>> * aVPosInit){

    std::vector<int> posToMerge{0};
    for (int c(1); c<mVEtatFin.size(); c++){
        if (mVEtatFin.at(c)!=mVEtatFin.at(c-1)){
            // rassembler les états de c-1 et antérieurs
            aVE->push_back(mVEtatFin.at(c-1));
            // calcul de la date moyenne
            year_month_day * d1 = mVDates.at(posToMerge.at(0));
            year_month_day * d2 = mVDates.at(posToMerge.at(posToMerge.size()-1));
            days d=(sys_days{*d2}-sys_days{*d1})/2;
            year_month_day x = sys_days{*d1} + days{d};
            aVD->push_back(x);
            aVPosInit->push_back(posToMerge);
            posToMerge.clear();
        }
        posToMerge.push_back(c);

    }
    /* debug
     std::cout << "TS1Pos::concateneEtat \n Date moyenne ; Etat " << std::endl;
     for (int c(0); c<aVE->size(); c++){
         std::cout << aVE->at(c) << " , " << aVD->at(c) << std::endl;
     }*/
}

void TS1Pos::nettoyer(){
    // retirer les no data et noter combien on en a eu
    for (int i(0);i<mVDates.size();i++){
        //std::cout << "date " << mVDates.at(i) << ", état " << mVEtat.at(i) << std::endl;

        if (mVEtat.at(i)==0){
            mVEtat.erase(mVEtat.begin() + i);
            mVEtatFin.erase(mVEtatFin.begin() + i);
            mVDates.erase(mVDates.begin() + i);
            i--;
        }
    }
}

// rendre plus rapide. Comparaison des voisins direct d'abor?
bool TS1Pos::repetition(int pos,int val, std::vector<int> * aVPt){
    bool aRes(0);

    if ((pos>0 && aVPt->at(pos-1)==val) | (pos+1>aVPt->size() && aVPt->at(pos+1)==val)){

        int iMin(pos-2),iMax(pos+2);
        // on commence par identifier la liste des positions du vecteur que l'on peut utiliser ,en fonction de sa taille et de la position à analyser
        if (aVPt->size()<=iMax){

            iMax=aVPt->size()-1;
            //std::cout << "iMax =" <<iMax << std::endl;
        }
        if (0>pos-2){
            iMin=0;
        }

        //Maintenant on analyse les triplettes ; trois valeurs consécutives.
        //std::cout << "iMax-iMin-1) = " << iMax << " - " << iMin << "-2-1 =" << (iMax-iMin-1) << std::endl;
        for (int trip(0); trip < (iMax-iMin-1);trip++){
            //std::cout << "triplette num " << trip << std::endl;
            if (aVPt->at(iMin+trip)==val && aVPt->at(iMin+trip+1)==val && aVPt->at(iMin+trip+2)==val){
                aRes=1;
                //std::cout << "repetition! --" << std::endl;
                break;
            }
        }
    }
    //std::cout << "done --" << std::endl;
    return aRes;
}

bool TS1Pos::repetition(int pos,int val){
    return repetition(pos,val,&mVEtat);
}

int TS1Pos::filtreVoisinDirect(int i, int val){
    int aRes(val);
    if ((mVEtat.size()>i+1 && mVEtat.at(i+1)==val) | (i>0 && mVEtat.at(i-1)==val)){return aRes;} else
    {
        if (mVEtat.size()>i+1){
            // je retiens le code le moins restrictif
            aRes=std::min(val,mVEtat.at(i+1));
        }
        if (0<i-1){
            aRes=std::min(aRes,mVEtat.at(i-1));
        }
    }
    return aRes;
}

// en fait c'est débile car j'ai déjà fait le test de 3 valeurs consécutives dans le premier filtre!! a mais non il y a aussi l'idée du filtre voisin, donc on a des situation ou j'ai 2 fois un code stress consécutif.
int TS1Pos::getEtatPourAnnee(int y){
    int aRes(1);
    // récupère un vecteur de code d'état qui correspond à cette année
    year ay{y};
    std::vector<int> etat;
    int i(0);
    for (year_month_day * ymd : mVDates){
        if (ymd->year()==ay){
            etat.push_back(mVEtatFin.at(i));
        }
        i++;
    }


    // il faut vérifier que j'ai 3 dates consécutives avec code 2.
    if (etat.size()>0){

        std::vector<int>::iterator p =std::find(etat.begin(), etat.end(), 4);
        if (p != etat.end()){return 4;} else {
            p =std::find(etat.begin(), etat.end(), 2);
            bool conseq(0);
            int c(0);
            std::vector<int>::iterator it;
            if (p != etat.end()){
                // vérifie que j'ai 3 valeurs consécutives de stress hydrique
                for (it = p; it != etat.end(); it++){
                    if (*it==2){c++;} else{c=0;}
                    if (c>2){conseq=1; aRes=2; break;}
                }
            }
            // pas stressé 3 fois d'affilé ; je teste si état =coupé 3 fois d'affilé
            if (!conseq){
                p =std::find(etat.begin(), etat.end(), 3);
                if (p != etat.end()){
                    c=0;
                    for (it = p; it != etat.end(); it++){
                        if (*it==3){c++;} else{c=0;}
                        if (c>2){conseq=1; aRes=3; break;}
                    }
                }
            }
            // ni stress dépérissement, ni coupé, test si stress passagé ou si mélange résineux-feuillus
            if (aRes==1){
                p =std::find(etat.begin(), etat.end(), 5);
                if (p != etat.end()){aRes=5;}
                p =std::find(etat.begin(), etat.end(), 6);
                if (p != etat.end()){aRes=6;}
            }
        }
    }
    return aRes;
}

void TS1Pos::printDetail(){
    std::cout << "Détail série temporelle pour un point ---" <<std::endl;
    std::cout << "date;etat;etatFinal" <<std::endl;
    for (int i(0);i<mVDates.size();i++){
        std::cout << *mVDates.at(i) << ";" << mVEtat.at(i) << ";" << mVEtatFin.at(i) <<std::endl;
    }
}

void TS1PosTest::add1Date(int code, tuileS2 * t){

    mVDates.at(c)=t->getymdPt();
    mVEtat.at(c)=code;
    mVCRSWIR.at(c)=t->getCRSWIR(pt_);
    mVCRSWIRNorm.at(c)=t->getCRSWIRNorm(pt_);
    rasterFiles r_b2(t->getRasterR1Name("2"));
    mVB2.at(c)=r_b2.getValue(pt_.X(),pt_.Y(),1)/10000.0;
    rasterFiles r_b3(t->getRasterR1Name("3"));
    mVB3.at(c)=r_b3.getValue(pt_.X(),pt_.Y(),1)/10000.0;
    rasterFiles r_b4(t->getRasterR1Name("4"));
    mVB4.at(c)=r_b4.getValue(pt_.X(),pt_.Y(),1)/10000.0;
    rasterFiles r_b8A(t->getRasterR2Name("8A"));
    mVB8A.at(c)=r_b8A.getValue(pt_.X(),pt_.Y())/10000.0;
    rasterFiles r_b11(t->getRasterR2Name("11"));
    mVB11.at(c)=r_b11.getValue(pt_.X(),pt_.Y())/10000.0;
    rasterFiles r_b12(t->getRasterR2Name("12"));
    mVB12.at(c)=r_b12.getValue(pt_.X(),pt_.Y())/10000.0;

    c++;
}

void TS1PosTest::nettoyer(){
    // retirer les no data et noter combien on en a eu
    //std::cout << "nettoyer le TS1PosTest" << std::endl;
    for (int i(0);i<mVDates.size();i++){
        if (mVEtat.at(i)==0){
            mVEtat.erase(mVEtat.begin() + i);
            mVEtatFin.erase(mVEtatFin.begin() + i);
            mVDates.erase(mVDates.begin() + i);

            mVCRSWIR.erase(mVCRSWIR.begin() + i);
            mVCRSWIRNorm.erase(mVCRSWIRNorm.begin() + i);
            mVB2.erase(mVB2.begin() + i);
            mVB3.erase(mVB3.begin() + i);
            mVB4.erase(mVB4.begin() + i);
            mVB8A.erase(mVB8A.begin() + i);
            mVB11.erase(mVB11.begin() + i);
            mVB12.erase(mVB12.begin() + i);
            i--;
        }
    }
}

void TS1PosTest::printDetail(){
    std::cout << "Détail série temporelle pour un point ---" <<std::endl;

    if (debugDetail){
        std::cout << "date;etat;etatFinal;CRSWIR;CRSWIRNorm;B2;B3;B4;B8A;B11;B12" <<std::endl;
        for (int i(0);i<mVDates.size();i++){
            std::cout << *mVDates.at(i) << ";" << mVEtat.at(i) << ";" << mVEtatFin.at(i) << ";" << mVCRSWIR.at(i) << ";" << mVCRSWIRNorm.at(i) << ";" << mVB2.at(i) << ";" << mVB3.at(i) << ";" << mVB4.at(i) << ";" << mVB8A.at(i) << ";" << mVB11.at(i) << ";" << mVB12.at(i) <<std::endl;
        }
    }
    else{
        std::cout << "date;etat;etatFinal" <<std::endl;
        for (int i(0);i<mVDates.size();i++){
            std::cout << *mVDates.at(i) << ";" << mVEtat.at(i) << ";" << mVEtatFin.at(i) <<std::endl;
        }
    }
    std::cout << "\n annee;etat" <<std::endl;
    for (auto kv : mVRes){
        std::cout << kv.first << ";" << kv.second <<std::endl;
    }

}

/* // scolyté puis coupé
 if (std::find(etat.begin(), etat.end(), 3) != etat.end()){aRes=3;} else{
 // scolyté mais pas coupé
     aRes=2;
 }
} else if (std::find(etat.begin(), etat.end(), 3) != etat.end()) {
 // coupé mais sans avoir été scolyté avant
 aRes=4;*/
