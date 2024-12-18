#include "ts1pos.h"

bool debugDetail(1); // affiche l'ensemble des valeurs de bandes en plus des codes état/ redondant avec mDebug
int nbDaysStress(90);
int mPrintDetail(0);
std::string globResXYTest("toto");
month mai{5}, avril{4};

void TS1Pos::analyse(){

    if (mPrintDetail){printDetail();}
    // filtre  pour retirer les valeurs abbérantes de la série temporelle.
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


    // filtre ; si détection de sol nu plusieurs fois et durant une durée longue, je met toutes les dates d'après en sol nu.

    std::vector<int>::iterator p =std::find(mVEtatFin.begin(), mVEtatFin.end(), 3);
    std::vector<int>::iterator it;
    if (p != mVEtatFin.end()){
        std::vector<year_month_day> aVD;
        std::vector<int> aVE, aVDuree;
        std::vector<std::vector<int>> aVPosEtatFin;
        concateneEtat(& aVD, & aVE, &aVPosEtatFin, &aVDuree);

        p = aVE.begin();
        while (p != aVE.end()) {
            p = std::find(p, aVE.end(), 3);
            if (p != aVE.end()) {
                int pos= p - aVE.begin();
                int res=3;
                // sol nu plusieurs fois d'affilé et ou sur longue période
                if (aVPosEtatFin.at(pos).size()>2 | aVDuree.at(pos)>40){
                    // si stressé avant sol nu, code 4. Sinon ; code 3 pour toutes les dates ultérieur
                    if (pos>0 && aVE.at(pos-1)==2 && aVDuree.at(pos-1)>20 ){res=4;} else {res=3;}
                    // change toutes les dates d'après
                    for (int i(aVPosEtatFin.at(pos).at(0)); i < mVEtatFin.size(); i++){
                        mVEtatFin.at(i)=res;
                    }
                    break;// important ce break sinon il pouvair changer un code 3 en code 4 ou autre. bien entendu, c'est pour des pixels litigieux mais quand même
                } else {
                    for (int i : aVPosEtatFin.at(pos)){
                        mVEtatFin.at(i)=6;// mélange car détection de sol nu mais peu de temps
                    }

                }
                p++;

            }
        }
    }

    // etat des lieux après ce filtre ;

   if (mPrintDetail){std::cout << "après filtre sol nu" << std::endl; printDetail();}

    // pour la détection des pixels stressé, on le fait en 2 itérations ; une première avec retour à la normale possible, une deuxième sans retour possible. Sinon ça peux yoyoter.
    detectStresseEtRetour();

    if (mPrintDetail){ std::cout << "après retour normale" << std::endl; printDetail();}

    p =std::find(mVEtatFin.begin(), mVEtatFin.end(), 2);
    if (p != mVEtatFin.end()){
        std::vector<year_month_day> aVD;
        std::vector<int> aVE, aVDuree;
        std::vector<std::vector<int>> aVPosEtatFin;
        concateneEtat(& aVD, & aVE, &aVPosEtatFin, &aVDuree);

        p = aVE.begin();
        while (p != aVE.end()) {
            p = std::find(p, aVE.end(), 2);
            if (p != aVE.end()) {
                int pos= p - aVE.begin();
                // stressé plusieurs fois d'affilé et ou sur longue période
                if (aVPosEtatFin.at(pos).size()>2-1 && aVDuree.at(pos)>20){
                    // on change les valeurs d'après
                    for (int po(pos+1); po<aVE.size();po++){
                        int res(2);
                        if (aVE.at(po)==3 | aVE.at(po)==4){res=4;
                        }
                        for (int i : aVPosEtatFin.at(po)){
                            mVEtatFin.at(i)=res;
                        }

                    }
                    break;
                }
                p++;
            }
        }
    }

     if (mPrintDetail){std::cout << "après 2ieme it de stress" << std::endl;printDetail();}

    // avec les filtres ci-dessus, on peut avoir un arbre avec un stress passager en 2019 et un stress scolyte puis coupé en 2020; manque de cohérence. je refait un test
    // oui mais attention car si stress passagé un an, puis sain 2 an, puis scolyte 3 an ; yoyote sans cohérence non plus!
    /* if(std::find(mVEtatFin.begin(), mVEtatFin.end(), 5)!=mVEtatFin.end() && std::find(mVEtatFin.begin(), mVEtatFin.end(), 2)!=mVEtatFin.end()){
        std::replace (mVEtatFin.begin(), mVEtatFin.end(), 5, 2);
    }

    // etat des lieux après ce filtre ;
    std::cout << "après filtre stressé" << std::endl;
    for (int e : mVEtatFin){
        std::cout << e << std::endl;
    }*/

    // les pixels qui sont un mélange de résineux et soit de sol ou de feuillus présentent un stress en hiver mais pas de stress en été (car photosynthèse du sol ou du feuillus).
    // je crée une classe supplémentaire pour les détecter. Avant ils étaient en stress temporaire en alternance avec état normal en été seulement
    //std::cout << " detect melange" << std::endl;
    detectMelange();


    // maintenant, je vais résumer l'état pour chaque année afin d'exporter des cartes annuelles.
    for (auto kv : mVRes){
        mVRes.at(kv.first)=getEtatPourAnnee(kv.first);
    }
}

void TS1Pos::detectStresseEtRetour(){

    std::vector<int>::iterator p;
    // stressé
    p =std::find(mVEtatFin.begin(), mVEtatFin.end(), 2);
    if (p != mVEtatFin.end()){
        std::vector<year_month_day> aVD;
        std::vector<int> aVE, aVDuree;
        std::vector<std::vector<int>> aVPosEtatFin;
        concateneEtat(& aVD, & aVE, &aVPosEtatFin, &aVDuree);

        p = aVE.begin();
        while (p != aVE.end()) {
            p = std::find(p, aVE.end(), 2);
            if (p != aVE.end()) {
                int pos= p - aVE.begin();
                // stressé plusieurs fois d'affilé et ou sur longue période
                if (aVPosEtatFin.at(pos).size()>2 && aVDuree.at(pos)>30){
                    // on change les valeurs d'après
                    for (int po(pos+1); po<aVE.size();po++){
                        int res(2);
                        if (aVE.at(po)==3 | aVE.at(po)==4){res=4;
                        }
                        //std::cout<< " change valeur etat " << std::endl;
                        for (int i : aVPosEtatFin.at(po)){
                            //std::cout<< " po " << po << ", i " << i << ", état = " << res << std::endl;
                            mVEtatFin.at(i)=res;
                        }

                    }
                    break;
                }
                p++;

            }
        }

        // etat des lieux après ce filtre ;
        /*std::cout << "retour normale" << std::endl;
        for (int e : mVEtatFin){
            std::cout << e << std::endl;
        }*/

        // check qu'il n'est pas détecté en coupe sanitaire, sinon pas de retour à la normale
        // std::vector<int>::iterator p3=std::find(mVEtatFin.begin(), mVEtatFin.end(), 4);
        if (std::find(mVEtatFin.begin(), mVEtatFin.end(), 4)==mVEtatFin.end()){
            // retour à la normale. attention, je peux avoir un vrai retour à la normal suivi d'un vrai scolyte puis coupé
            //p = std::find(aVE.begin(), aVE.end(), 2);
            p=aVE.begin();
            while ((p = std::find_if(p, aVE.end(), isScolyte)) != aVE.end())
            {

                //while (p != aVE.end()) {
                std::vector<int>::iterator p2;
                p2 = std::find(p, aVE.end(), 1);
                if (p2 != aVE.end()) {
                    int pos= p2 - aVE.begin();
                    // retour à la normale suite à un stress temporaire
                    if ((aVPosEtatFin.at(pos).size()>2) && (aVDuree.at(pos)>30) && (aVE.at(pos-1)==2) && (aVDuree.at(pos-1)<nbDaysStress)){

                        // retour à la normale suite à un stress temporaire
                        // situation vicieuse ou jai 222 1111 (moins de 90j) 222 1111 (plus de 90 j)
                        // donc je dois vérifier toutes les valeurs d'état d'avant pour déterminer si j'ai des stress

                        if (pos-2==0 |  (pos-2>=0 && mVEtatFin.at(aVPosEtatFin.at(pos-2).at(0))!=2)){
                            bool testStressAnterieur(0);
                            for(int i(0) ; i < aVPosEtatFin.at(pos-2).at(0); i++){
                                if (mVEtatFin.at(i)==2){testStressAnterieur=1;}
                            }

                            if (!testStressAnterieur){
                                // on change les valeurs de stress pour les mettre en stress temporaire
                                for (int i : aVPosEtatFin.at(pos-1)){
                                    mVEtatFin.at(i)=5;
                                }
                                // on remet les valeurs d'état comme avant qu'elles soient modifié par la détection du stress
                                for (int po(pos); po<aVE.size();po++){
                                    for (int i : aVPosEtatFin.at(po)){
                                        mVEtatFin.at(i)=aVE.at(po);
                                    }
                                }
                            }
                        }

                    }


                }
                p++;
                //p2++;
            }
        }
    }

}

bool isScolyte(int x){
    return x == 2;
}

void TS1Pos::detectMelange(){
    // premier état stress temporaire
    std::vector<int>::iterator p=std::find(mVEtatFin.begin(), mVEtatFin.end(), 5);
    // je m'intéresse au stress passager sans aucune présence de stress de catégorie 2
    if (p!=mVEtatFin.end() && std::find(mVEtatFin.begin(), mVEtatFin.end(), 2)==mVEtatFin.end()){
        std::vector<year_month_day> aVD;
        std::vector<int> aVE, aVDuree;
        std::vector<std::vector<int>> aVPosEtatFin;
        concateneEtat(& aVD, & aVE, &aVPosEtatFin, &aVDuree);

        /*
        for (int i(0);i<aVD.size();i++){
            std::cout << " date; " << aVD.at(i) << " , duree " << aVDuree.at(i) << " , etat " << aVE.at(i) << std::endl;

        }*/

        // recherche de toutes les positions ou j'ai un stress temporaire et test si c'est en hiver.
        p = aVE.begin();
        while (p != aVE.end()) {
            p = std::find(p, aVE.end(), 5);
            if (p != aVE.end()) {
                int pos= p - aVE.begin();
                // détecte si c'est en hiver
                if ((aVD.at(pos).month()>month{9} | aVD.at(pos).month()<month{5}) && aVDuree.at(pos)<200){
                    // si oui, on change les valeurs dans mEtatFinal pour passer en catégorie "mélange feuillus résineux"
                    // warn! bug! si stress temporaire hivernal suivit de scolyte l'été d'après, va tout mettre en 6. pas bon
                    //std::cout << "detecte stress hivernal en date du " << aVD.at(pos) << std::endl;
                    for (int i : aVPosEtatFin.at(pos)){
                        //std::cout << "change la valeur  " << mVEtatFin.at(i) << " de la date " <<  * mVDates.at(i) << std::endl;
                        mVEtatFin.at(i)=6;
                    }
                    // je ne sais pas à quoi c'est du, mais j'ai des code 5 en état final qui commence durant un stress (etat ini 2) puis continuent sur un code 1 (pseudo-retour à la normal) qui finissent pas un code 3 (coupé). Il faudrait donc changer les 5 en 2 (scolyté) et les 3 en 4 (coupé àprès scolyté)
                } else if (aVE.size()>pos+1 && aVE.at(pos+1)==3) {

                    //std::cout << "detecte coupe après stress temporaire " << aVD.at(pos) << std::endl;
                    for (int i : aVPosEtatFin.at(pos)){
                        //std::cout << "change la valeur  " << mVEtatFin.at(i) << " de la date " <<  * mVDates.at(i) << std::endl;
                        mVEtatFin.at(i)=2;
                    }
                    for (int i : aVPosEtatFin.at(pos+1)){
                        //std::cout << "change la valeur  " << mVEtatFin.at(i) << " de la date " <<  * mVDates.at(i) << " en coupe après scolyte" << std::endl;
                        mVEtatFin.at(i)=4;
                    }
                }
                p++;
            }

        }
    }
}


// on ajoute par après une regle ; si plus de x mois stressé, on empêche le retour à la normale
void TS1Pos::restrictRetourNorm(){
    // premier état stress temporaire
    std::vector<int>::iterator p=std::find(mVEtatFin.begin(), mVEtatFin.end(), 5);
    if (p!=mVEtatFin.end()){
        std::vector<year_month_day> aVD;
        std::vector<int> aVE;
        std::vector<int> aVDuree;
        std::vector<std::vector<int>> aVPosEtatFin;
        concateneEtat(& aVD, & aVE, &aVPosEtatFin, & aVDuree);
        // recherche de toutes les positions ou j'ai un stress temporaire
        p = aVE.begin();
        while (p != aVE.end()) {
            p = std::find(p, aVE.end(), 5);
            if (p != aVE.end()) {
                int pos= p - aVE.begin();
                // détecte si le stress a duré plus que le nombre de jours seuil (defaut 90).
                if (aVDuree.at(pos)>nbDaysStress){
                    // si oui, on change toutes les valeurs suivantes dans mEtatFinal pour repasser en scolyté
                    for (int i : aVPosEtatFin.at(pos)){
                        mVEtatFin.at(i)=2;
                    }
                }
                p++;
            }
        }
    }
}

void TS1Pos::concateneEtat(std::vector<year_month_day> * aVD, std::vector<int> *aVE, std::vector<std::vector<int>> * aVPosInit, std::vector<int> *aVDuree){

    std::vector<int> posToMerge{0};
    for (int c(1); c<mVEtatFin.size(); c++){
        if (mVEtatFin.at(c)!=mVEtatFin.at(c-1) | c==mVEtatFin.size()-1){

            if (c==mVEtatFin.size()-1){ posToMerge.push_back(c);}
            // rassembler les états de c-1 et antérieurs
            aVE->push_back(mVEtatFin.at(c-1));
            // calcul de la date moyenne
            year_month_day * d1 = mVDates.at(posToMerge.at(0));
            year_month_day * d2 = mVDates.at(posToMerge.at(posToMerge.size()-1));

            // durée de la période ; utile également, j'ajoute
            days d=(sys_days{*d2}-sys_days{*d1});
            if (aVDuree!=NULL){aVDuree->push_back(d.count());}
            year_month_day x = sys_days{*d1} + days{d/2};
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
            mVPtrTS2.erase(mVPtrTS2.begin() + i);
            i--;
        }
    }
}

// fait ç'est très long de faire ça, en toute logique. car il y a beaucoup d'année..
void TS1Pos::writeIntermediateRes1pos(){
    // boucle sur les tuileOneDate
    // un scanpix qui est redéfini à chaque fois car sinon il sera partagé durant le calcul en parallèle et ça va provoquer des merdes par moment
    float * scanPix2=(float *) CPLMalloc( sizeof( float ) * 1 );

    int c(0);

    for (tuileS2OneDateSco * t : mVPtrTS2){
        //scanPix2[0]=mVEtat.at(c);
        //t->mDSetatInit->GetRasterBand(1)->RasterIO( GF_Write,  mV,mU,1,1,scanPix2, 1, 1,GDT_Float32, 0, 0 );
        scanPix2[0]=mVEtatFin.at(c);
        t->mDSetatFinal->GetRasterBand(1)->RasterIO( GF_Write,  mV,mU,1,1,scanPix2 , 1, 1,GDT_Float32, 0, 0 );
    c++;
    }
    CPLFree(scanPix2);
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
    year ay{y}, ayP1{y+1};

      if (mDebug){std::cout << " get Etat pour Année " << y << std::endl;}

    std::vector<int> etat;
    int i(0);
    for (const year_month_day * ymd : mVDates){

        if ( (ymd->year()==ay && ymd->month()>avril ) | (ymd->year()==ayP1 && ymd->month()<mai) ){
            etat.push_back(mVEtatFin.at(i));

           if (mDebug){std::cout << " ajout date " << *ymd << std::endl;}
            // on rajoute deux dates car si on observe un stress scolyte en décembre, suivi de deux autres l'année d'après, on veux le savoir.
            /*if (i+1<mVDates.size() && mVDates.at(i+1)->year()>ay && mVEtatFin.at(i)==2){
                etat.push_back(mVEtatFin.at(i+1));
                if (i+2<mVDates.size() && mVEtatFin.at(i+1)==2
            }*/
        }
        i++;
    }

    // il faut vérifier que j'ai 3 dates consécutives avec code 2. Sauf si je n'ai que 2 dates!!
    // je pourrais être moins restrictif car des fois j'ai 1 code 2 en décembre et puis 2 autres en janvier de l'année d'après, ça fonctionne pas alors.
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
            // pas stressé 2 fois d'affilé ; je teste si état =coupé 3 fois d'affilé. Seulement deux fois car sinon j'ai des cas ou la dernière année je n'ai que deux dates mais coupé, donc ça dois être coupé!
            if (!conseq){
                p =std::find(etat.begin(), etat.end(), 3);
                if (p != etat.end()){
                    c=0;
                    for (it = p; it != etat.end(); it++){
                        if (*it==3){c++;} else{c=0;}
                        if (c>1){conseq=1; aRes=3; break;}
                    }
                }
            }
            // ni stress dépérissement, ni coupé, test si stress passagé ou si mélange résineux-feuillus
            if (aRes==1){
                // le code 5 est plus contraignant que le code 6.
                p =std::find(etat.begin(), etat.end(), 6);
                if (p != etat.end()){aRes=6;}
                p =std::find(etat.begin(), etat.end(), 5);
                if (p != etat.end()){aRes=5;}
            }
        }
    }
    return aRes;
}

int TS1Pos::getPositionFirstSco(){
    int aRes(-1);
    std::vector<int>::iterator p =std::find(mVEtatFin.begin(), mVEtatFin.end(), 2);

    if (p != mVEtatFin.end()){
        std::vector<year_month_day> aVD;
        std::vector<int> aVE, aVDuree;
        std::vector<std::vector<int>> aVPosEtatFin;
        concateneEtat(& aVD, & aVE, &aVPosEtatFin, &aVDuree);

        p = aVE.begin();
        while (p != aVE.end()) {
            int pos=p - aVE.begin();
            if (aVE.at(pos)==2){
                // test si il y a des retour à la normale par après
                if (std::find(p, aVE.end(), 1)==aVE.end() && std::find(p, aVE.end(), 5)==aVE.end() && std::find(p, aVE.end(), 6)==aVE.end()){
                    aRes=aVPosEtatFin.at(pos).at(0);
                    break;
                }
            }
            p++;
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

// j'aimerai que cela fonctionne au moins partiellement pour une position qui serai en dehors du masque utilisé
void TS1PosTest::add1Date(int code, tuileS2OneDate * t){
    //std::cout << " TS1PosTest :: add one date " << std::endl;
    mVDates.at(c)=t->getymdPt();
    mVEtat.at(c)=code;
    //mVCRSWIR.at(c)=t->getCRSWIR(pt_);


    rasterFiles r_b2(t->getRasterR1Name("2"));
    mVB2.at(c)=r_b2.getValue(pt_.X(),pt_.Y(),1)/10000.0;
    rasterFiles r_b3(t->getRasterR1Name("3"));
    mVB3.at(c)=r_b3.getValue(pt_.X(),pt_.Y(),1)/10000.0;
    rasterFiles r_b4(t->getRasterR1Name("4"));
    mVB4.at(c)=r_b4.getValue(pt_.X(),pt_.Y(),1)/10000.0;
    rasterFiles r_b8A(t->getOriginalRasterR2Name("8A"));
    mVB8A.at(c)=r_b8A.getValue(pt_.X(),pt_.Y())/10000.0;
    rasterFiles r_b11(t->getOriginalRasterR2Name("11"));
    mVB11.at(c)=r_b11.getValue(pt_.X(),pt_.Y())/10000.0;
    rasterFiles r_b12(t->getOriginalRasterR2Name("12"));
    mVB12.at(c)=r_b12.getValue(pt_.X(),pt_.Y())/10000.0;

    rasterFiles r_b8(t->getRasterR1Name("8"));
    mVB8.at(c)=r_b8.getValue(pt_.X(),pt_.Y(),1)/10000.0;
    rasterFiles r_b5(t->getOriginalRasterR2Name("5"));
    mVB5.at(c)=r_b5.getValue(pt_.X(),pt_.Y())/10000.0;
    rasterFiles r_b6(t->getOriginalRasterR2Name("6"));
    mVB6.at(c)=r_b6.getValue(pt_.X(),pt_.Y())/10000.0;
    rasterFiles r_b7(t->getOriginalRasterR2Name("7"));
    mVB7.at(c)=r_b7.getValue(pt_.X(),pt_.Y())/10000.0;

    /* B2 bleu
 * B3 vert
 * B4 rouge
 * B8a NIRa
 * B11 SWIR1
 * B12 SWIR2
 *
 * CRSWIR = SWIR1 / ( NIRa + (lSWIR1-lNIRa)* ((SWIR2 - NIRa) / (lSWIR2-lNIRa)  )   )
 *
 * modéliation de 2 ligne droite et ratio des segment qui passent de l'absisse à la longeur d'onde SWIR 1 vers chacunes de ces 2 droites
 */
    if (mVB12.at(c)!=-1){// dans le cas ou je tombe dans du no data, cad en dehors du masque edge
        mVCRSWIR.at(c)=mVB11.at(c)/(mVB8A.at(c)+(1610-865)* ((mVB12.at(c)-mVB8A.at(c))/(2190-865)));
    } else { mVCRSWIR.at(c)=-1;}
    // attention, me retourne 1 si jamais toute les bandes sont
    mVCRSWIRNorm.at(c)=mVCRSWIR.at(c)/getCRtheorique(*t->getymdPt());

    // en procédant comme ceci j'ai des points qui ont des valeurs spectrale pour chaque bande , mais par contre je suis en dehors du masque nuage et je ne le sais pas. Donc je lis aussi ce masque pour pouvoir nettoyer par après
    rasterFiles r_m(t->getRasterMasqGenName());
    mVMasq.at(c)=r_m.getValue(pt_.X(),pt_.Y());

    c++;
}


void TS1PosTest::nettoyer(){
    // retirer les no data et noter combien on en a eu
    //std::cout << "nettoyer le TS1PosTest" << std::endl;
    // attention, si j'utiliser TS1Pos sur une zone en dehors du masque, je ne veux pas qu'il nettoye sinon il ne reste plus rien
    //int c1(0),c0(mVDates.size());
    for (int i(0);i<mVDates.size();i++){
        //if (mVEtat.at(i)==0 && ){

        // if (mVCRSWIR.at(i)==-1 |(mVEtat.at(i)==0 && mVB2.at(i)==0)){// des fois code =0 et B2 aussi mais pas B11 et B12 et CRSWIR
        //if (mVMasq.at(i)==2 | mVMasq.at(i)==3){
        // check que les bandes n'ont pas la valeur -10 000 (-1 car déjà facteur appliqué en amont), parceque le masque qui est calculé seulement pour l'intérieur du masque EP
        //if ((mVMasq.at(i)==2 | mVMasq.at(i)==3) | mVB2.at(i)==-1 ){
        if ((mVMasq.at(i)!=1 && mVMasq.at(i)!=4) ){

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
            mVMasq.erase(mVMasq.begin() + i);
            i--;
            // c1++;
        } else if (mVB2.at(i)==-1){std::cout << " no data ommited" << std::endl;}
    }
    //        std::cout << "nettoyage de " << c1 << " dates sur " << c0 << std::endl;
}

void TS1PosTest::printDetail(std::string aOut){
    std::cout << "Détail série temporelle pour un point ---" <<std::endl;

    if (aOut!="toto"){
        // crée un fichier txt pour l'export des résultats
        std::ofstream out;
        out.open(aOut);

        out << "date;etat;etatFinal;CRSWIR;CRSWIRNorm;B2;B3;B4;B8A;B11;B12\n" ;

        for (int i(0);i<mVDates.size();i++){
            out << *mVDates.at(i) << ";" << mVEtat.at(i) << ";" << mVEtatFin.at(i) << ";" << mVCRSWIR.at(i) << ";" << mVCRSWIRNorm.at(i) << ";" << mVB2.at(i) << ";" << mVB3.at(i) << ";" << mVB4.at(i) << ";" << mVB8A.at(i) << ";" << mVB11.at(i) << ";" << mVB12.at(i) <<"\n";
        }
        out.close();
        std::cout << " sauvé dans fichier " << aOut << std::endl;
    } else {
        if (debugDetail){
            std::cout << "date;etat;etatFinal;CRSWIR;CRSWIRNorm;B2;B3;B4;B8A;B11;B12;Masq" <<std::endl;
            for (int i(0);i<mVDates.size();i++){
                std::cout << *mVDates.at(i) << ";" << mVEtat.at(i) << ";" << mVEtatFin.at(i) << ";" << mVCRSWIR.at(i) << ";" << mVCRSWIRNorm.at(i) << ";" << mVB2.at(i) << ";" << mVB3.at(i) << ";" << mVB4.at(i) << ";" << mVB8A.at(i) << ";" << mVB11.at(i) << ";" << mVB12.at(i)  << ";" << mVMasq.at(i)<<std::endl;
            }
        } else{
            std::cout << "date;etat;etatFinal" <<std::endl;
            for (int i(0);i<mVDates.size();i++){
                std::cout << *mVDates.at(i) << ";" << mVEtat.at(i) << ";" << mVEtatFin.at(i) <<std::endl;
            }
        }
        std::cout << "\n annee;etat;delaisCoupe;FirstDateSco" <<std::endl;
        for (auto kv : mVRes){
            std::cout << kv.first << ";" << kv.second << ";" << getDelaisCoupe(kv.first) << ";" << getDelaisCoupe(kv.first,1) <<std::endl;
        }
    }
}

std::map<int,std::vector<double>> * TS1PosTest::summaryByTri(){
    std::map<int,std::vector<double>> * aRes=new std::map<int,std::vector<double>>;
    // on effectue une moyenne des radiations des bandes par trimestre
    // en première approche, le plus simple ; sélection  pour chaques trimestres des dates qui tombent dans le trimestre puis moyenne de la radiation pour chaque bande
    for (int tri(1);tri<5;tri++){
        std::vector<int> vIndex=getDateIndexForTri(tri);
        double b2(0),b3(0),b4(0),b8(0),b11(0),b12(0);
        if (vIndex.size()>0){
            for (int i : vIndex){
                b2+=mVB2.at(i);
                b3+=mVB3.at(i);
                b4+=mVB4.at(i);
                b8+=mVB8A.at(i);
                b11+=mVB11.at(i);
                b12+=mVB12.at(i);
            }
            // calculer la moyenne
            b2=b2/vIndex.size();
            b3=b3/vIndex.size();
            b4=b4/vIndex.size();
            b8=b8/vIndex.size();
            b11=b11/vIndex.size();
            b12=b12/vIndex.size();
        }
        //std::cout << "calcul moyenne trimestrielle sur " << vIndex.size() << " observations " << std::endl;
        std::vector<double> m{b2,b3,b4,b8,b11,b12};
        aRes->emplace(std::make_pair(tri,m));
    }
    return aRes;
}

void TS1PosTest::summaryByTriTest(){

    std::cout << " TS1PosTest::summaryByTriTest() " << std::endl;
    // on effectue une moyenne des radiations des bandes par trimestre
    // en première approche, le plus simple ; sélection  pour chaques trimestres des dates qui tombent dans le trimestre puis moyenne de la radiation pour chaque bande
    std::cout << "type;date;b2;b3;b4;b8;b5;b6;b7;b8A;b11;b12" << std::endl;
    for (int tri(1);tri<5;tri++){
        std::vector<int> vIndex=getDateIndexForTri(tri);
        //std::cout << "calcul moyenne trimestrielle " << tri << " sur " << vIndex.size() << " observations " << std::endl;
        double b2(0),b3(0),b4(0),b8(0),b8A(0),b5(0),b6(0),b7(0),b11(0),b12(0);
        if (vIndex.size()>0){
            for (int i : vIndex){
                // je voudrai aussi les valeurs initiales!
                std::cout << "o" << ";" << * mVDates.at(i) << ";" << mVB2.at(i) << ";" << mVB3.at(i) << ";" << mVB4.at(i) << ";" << mVB8.at(i)
                          << ";" << mVB5.at(i) << ";" << mVB6.at(i) << ";" << mVB7.at(i) << ";" << mVB8A.at(i) << ";" << mVB11.at(i) << ";" << mVB12.at(i) << std::endl;
                b2+=mVB2.at(i);
                b3+=mVB3.at(i);
                b4+=mVB4.at(i);
                b8+=mVB8.at(i);


                b5+=mVB5.at(i);
                b6+=mVB6.at(i);
                b7+=mVB7.at(i);
                b8A+=mVB8A.at(i);
                b11+=mVB11.at(i);
                b12+=mVB12.at(i);
            }
            // calculer la moyenne
            b2=b2/vIndex.size();
            b3=b3/vIndex.size();
            b4=b4/vIndex.size();
            b8=b8/vIndex.size();

            b5=b5/vIndex.size();
            b6=b6/vIndex.size();
            b7=b7/vIndex.size();
            b8A=b8A/vIndex.size();
            b11=b11/vIndex.size();
            b12=b12/vIndex.size();
        }
        // Maintenant j'affiche ça dans la console ou je le met dans un dataframe pour affichage après?
        std::cout << "m" << ";" << tri << ";" << b2 << ";" << b3 << ";" << b4 << ";" << b8 << ";" << b5 << ";" << b6 << ";" << b7 << ";" << b8A<< ";" << b11 << ";" << b12 << std::endl ;
    }
}

std::vector<int> TS1PosTest::getDateIndexForTri(int trimestre){
    std::vector<int> aRes;
    int c(0);
    int m0(((trimestre-1)*3)+1);
    int m1(((trimestre-1)*3)+3);
    for (year_month_day * d : mVDates){

        if (d->month()>=month{m0} && d->month()<=month{m1}){
            aRes.push_back(c);
        }
        c++;
    }
    return aRes;
}

int TS1Pos::getDelaisCoupe(int y, bool firstDate){
    //if (mDebug){std::cout << "TS1Pos::getDelaisCoupe " << y << std::endl;}

    int aRes(0);

    year ay{y}, ayP1{y+1};

    if (mVRes.at(y)==4 | (mVRes.at(y)==2 && firstDate)){ // quand j'ai deux dates scolyté en fin d'année, il me met dans les résultats que c'est pas scolyté vu qu'il en faut 3 d'affilé Grr

        if(0){
            // ./s2_timeSerie --xmlIn ../s2_sco.xml --XYtest 672435 5567025  me montre une situation conflictuelle ou ça ne fonctionne pas. C'est pour ces pixels qui sont en feuillus et qui sont détecté comme étant scolyté durant l'hiver. Rien ne fonctionne sur les feuillus, l'algo détecte soit de la coupe, soit du stress, rien ne va
            // arbre coupé après dépérissement constaté
            // récupère un vecteur de code d'état qui correspond à cette année - pas la bonne démarche, car une coupe sanitaire peut avoir eu lieu 2 ans après la premiere détection de scolyte.

            std::vector<int> etat;
            std::vector<year_month_day *> dates;
            int i(0);
            //for (const year_month_day * ymd : mVDates){
            for (year_month_day * ymd : mVDates){

                if ( (ymd->year()==ay && ymd->month()>avril ) | (ymd->year()==ayP1 && ymd->month()<mai) ){
                //if (ymd->year()==ay){
                    dates.push_back(ymd);
                    etat.push_back(mVEtatFin.at(i));
                    // à priori la méthode fonctionne pas si le changement de ES 1 vers ES 2 ou de ES2 vers ES 4 à lieu précisément entre deux années. Par exemple un scolyte coupé en décembre.  Faudrai y remédier. en prenant une date de l'année d'après?
                    // si c'est la dernière date de l'année, je prends une date de l'année d'après si jamais le changement 2 --> 4 à lieu à ce moment
                    if (i+1<mVDates.size() && mVDates.at(i+1)->year()>ay){
                        dates.push_back(mVDates.at(i+1));
                        etat.push_back(mVEtatFin.at(i+1));
                    }
                }
                i++;
            }

            std::vector<int>::iterator p =std::find(etat.begin(), etat.end(), 2);
            if (p != etat.end()){
                int pos0=p - etat.begin();
                if (firstDate){
                    if (pos0==0){aRes=255;//std::cout << "scolyte dès le début d'année" << std::endl;
                    } else {
                        year_month_day debuty(dates.at(pos0)->year(),month{1},day{1});
                        //std::cout << " date début de l'année " << debuty << ", début de l'attaque " << *dates.at(pos0) << ", pos0=" << pos0<< std::endl;
                        days d=sys_days{*dates.at(pos0)}-sys_days{debuty};
                        aRes=std::max(d.count()/7,1); // sous forme de nombre de semaines également
                    }
                } else {

                    std::vector<int>::iterator q =std::find(etat.begin(), etat.end(), 4);
                    if (q != etat.end()){
                        int pos1=q - etat.begin();
                        days d=(sys_days{*dates.at(pos1)}-sys_days{*dates.at(pos0)});
                        aRes=std::max(d.count()/7,1);
                    }
                }

            } else {
                // coupée l'année d'avant
                aRes=255;
            }
        }
        // nouvelle version 2022 03 15
        int p=getPositionFirstSco();
        if (p!=-1){
            year_month_day * ymdFirstSco=mVDates.at(p);
            // on compare la date premier scolyte à l'année qui nous intéresse
            if (firstDate){
                year_month_day debuty(ay,month{1},day{1});
                days d=sys_days{*ymdFirstSco}-sys_days{debuty};
                aRes=100+(d.count()/7);
            } else {
                // on compare la date premier scolyte à la date de coupe sanitaire.
                for (int i(0);i<mVDates.size();i++){
                    if (mVEtatFin.at(i)==4){

                        year_month_day * ymdCoupe=mVDates.at(i);
                        //std::cout << " got the coupe date : " << *ymdCoupe << std::endl;
                        days d=sys_days{*ymdCoupe}-sys_days{*ymdFirstSco};
                        //std::cout << " number of days : " << d.count() << std::endl;
                        aRes=std::max(d.count()/7,1);
                        break;
                    }
                }
            }
        }

    }
    return aRes;
}
