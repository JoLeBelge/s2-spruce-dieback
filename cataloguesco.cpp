#include "cataloguesco.h"

extern double seuilCR; // on est encore bien au dessus de ce que font les français
extern bool mDebug;
std::string XYtestFile("toto");
std::vector<pts> globVPts;

bool doAnaTS(1);
bool docleanTS1pos(1);// nettoyage TS d'un point. défaut oui. Mais si pt en dehors du masque, faut pas nettoyer
extern double Xdebug;
extern double Ydebug;

extern std::string wd;
extern std::string path_otb;
extern std::string EP_mask_path;
//extern std::string iprfwFile;
extern int globSeuilCC;
extern int year_analyse;
extern bool overw;
extern bool debugDetail;
extern int nbDaysStress;

extern std::string globTuile;
extern std::string globResXYTest;
extern std::string globSuffix;
extern bool doDelaisCoupe;
extern bool doFirstDateSco;

void catalogueSco::analyseTS(){

    if (openDS()){

        int nb = mVProdutsOK.size();
        int c(0);
        int nbStep=10;
        int step=mY/nbStep;
        int count(0);

        // Le calcul parallèle pixel par pixel nécessite de charger toute les cartes en Mémoire. fonctionne pour une année, mais pas pour la série tempo complête --> pas assez de mémoire vive. FAUX en fait c'est une erreur du système à propos du nombre max de fichier descriptor que l'on peut ouvrir en une fois
        // traitement ligne par ligne pour pouvoir travailler en parallèle.
        for ( int row = 0; row < mY; row++ ){
            // lecture masque ep
            readMasqLine(row);
            //omp_set_dynamic(0); // pour lui imposer le nombre de thread à utiliser, sinon le "dynamic teams " choisi moins de threads si trop de dates dans ma ts
            //This will create one parallel region (aka one fork/join, which is expensive and therefore you don't want to do it for every loop) and run multiple loops in parallel within that region.

#pragma omp parallel //num_threads(8)
            {
#pragma omp for
                for (tuileS2OneDateSco * t : mVProdutsOK){
                    //std::cout << "tuiles  " << t->getDate()<< std::endl;
                    t->readCRnormLine(row);
                    t->readMasqLine(row);
                    t->computeCodeLine();
                }
                //}

                std::vector<int> aVCol;
                for (int col=0 ; col<mX;col++){
                    if (scanLineR1[col]==1){aVCol.push_back(col);}
                }

                //num_threads(8) shared(row,mYs,seuilCR,scanLine,mVProdutsOK)
                //#pragma omp parallel //num_threads(12)
                //           {
#pragma omp for
                for (int col : aVCol){
                    //int tid = omp_get_thread_num();
                    //printf("Hello world from omp thread %d\n", tid);
                    //std::cout << "row " << row << " , " << col << std::endl;
                    TS1Pos ts(row,col,&mYs,nb, mVProdutsOK);

                    for (const tuileS2OneDateSco * t : mVProdutsOK){
                        ts.add1Date(t->getymdPt(),t->getCodeLine(col));
                    }

                    ts.nettoyer();
                    ts.analyse();
                    //#pragma omp critical

                    //ts.writeIntermediateRes1pos(); //lui il prend beaucoup trop de temps. mais il n'y a pas mille solution?
                    writeRes1pos(&ts);
                    /*    j'avais fait un test chiant et non concluant décriture d'une ligne entière à la place de l'écriture pixel par pixel, mais en terme de temps de calcul ça ne change rien !!           }
                    for (int y : mYs){
                        mSL.at(y)[col] = ts.mVRes.at(y);
                    }*/
                    // end col
                }
            }// end pragma

            /*for (int y : mYs){
                //std::cout << "ts res pour y " << y << " est " << scanPix[0] << ", écriture à la position " << ts->mU << " , " << ts->mV << std::endl;
                mMapResults.at(y)->GetRasterBand(1)->RasterIO(GF_Write, 0,row,mX,1,mSL.at(y), mX, 1,GDT_Float32, 0, 0 );
                // pour réinitialiser les valeurs d'une ligne à l'autre
                for (int x(0) ; x < mY; x++){
                    mSL.at(y)[x]=0;
                }

                //CPLFree(mSL.at(y));
                //mSL.at(y)= (float *) CPLMalloc( sizeof( float ) * mY );
            }*/
            c++;
            if (c%step==0){
                count++;
                std::cout << count*nbStep << "%..." << std::endl;
             //   break;
            }
        }

    }
    closeDS();

}


void catalogueSco::traitement(){
    // récapitulatif;
    summary();
    createMaskForTuile();
    // check que le masque a bien été crée:
    if (boost::filesystem::exists(getNameMasque())){

        for (tuileS2OneDate * t : mVProduts){
            if (t->mCloudCover<globSeuilCC){
                if (std::find(mYs.begin(), mYs.end(), t->gety()) == mYs.end()){mYs.push_back(t->gety());}
                mVProdutsOK.push_back(new tuileS2OneDateSco(t));
            }
        }
        std::sort(mVProdutsOK.begin(), mVProdutsOK.end(), PointerCompare());


        // boucle sans multithread pour les traitements d'image
        std::for_each(
                    std::execution::seq,
                    mVProdutsOK.begin(),
                    mVProdutsOK.end(),
                    [](tuileS2OneDateSco * t)
        {
            // check que cloudcover est en dessous de notre seuil
            // if (t->mCloudCover<globSeuilCC){
            t->masque();
            t->resample();
            t->computeCR();
            t->masqueSpecifique();
            t->normaliseCR();
            t->createRastEtat();
            if (mDebug){std::cout << std::endl ;}
            // }
        });
        //std::cout << " boucle sur les tuiles done" << std::endl;

        // points pour visu de la série tempo et pour vérifier la fct harmonique
        if (XYtestFile!="toto"){
            if (globResXYTest=="toto"){
                globResXYTest=XYtestFile.substr(0,XYtestFile.size()-4);}
            globVPts=readPtsFile(XYtestFile);
        }
        analyseTSinit();

    } else {
        std::cout << "erreur, masque tuile pas créé : je ne fait pas les traitements " << std::endl;
    }
}


void catalogueSco::analyseTSTest1pixel(double X, double Y, std::string aFileOut){

    std::cout << "Test pour une position : " << X << "," << Y << " avec seuil ratio CRSWIR/CRSWIRtheorique(date) =" << seuilCR << " et nombre de jours de stress après lesquel on interdit un retour à la normal de " << nbDaysStress << ", tuile " << globTuile <<  std::endl;
    int nb = mVProdutsOK.size();
    pts pt(X,Y);

    TS1PosTest ts(&mYs,nb,mVProdutsOK,pt);
    //std::cout << "TS1PosTest créé." << std::endl;
    for (tuileS2OneDateSco * t : mVProdutsOK){
        //std::cout << " date " << t->getDate() << std::endl;
        // 0; ND
        // 1: valeur CR swir ok.
        // 2; valeur CRswir NOK
        // 3 ; sol nu.
        double crnorm=t->getCRSWIRNorm(pt);
        int solnu = t->getMaskSolNu(pt);
        int code=0;
        if (solnu==0){code=0;
        }else if(solnu==2 | solnu==3){
            code=3;
            // ci-dessous donc pour solnu=1 (zone oK)
        } else if (crnorm<=seuilCR) {
            code=1;
        } else if (crnorm>seuilCR) {
            code=2;
        }

        ts.add1Date(code,t);
    }
    // ça risque d'impacter le résultat de ne pas nettoyer tout ca.. ben oui formément
    if (docleanTS1pos){ts.nettoyer();
        ts.analyse();
    }

    ts.printDetail(aFileOut);
}

bool catalogueSco::openDS(){
    std::cout << "ouverture de tout les raster de la TS" << std::endl;
    bool aRes(1);

    if (exists(getNameMasqueEP())){
        const char *pszFormat = "MEM";
        GDALDriver *pDriver= GetGDALDriverManager()->GetDriverByName(pszFormat);
        GDALDataset * mask=(GDALDataset *) GDALOpen( getNameMasqueEP().c_str(), GA_ReadOnly );
        const char * ch="immem";
        if (mask==NULL){
            std::cout << "masque EP pas lu correctement" << std::endl;
            return 0;
        }
        mDSmaskR1= pDriver->CreateCopy( ch,mask,FALSE, NULL,NULL, NULL );
        mX=mask->GetRasterBand(1)->GetXSize();
        mY=mask->GetRasterBand(1)->GetYSize();
        GDALClose( mask );

        //mDSmaskEP= (GDALDataset *) GDALOpen( getNameMasqueEP().c_str(), GA_ReadOnly );
        scanPix=(float *) CPLMalloc( sizeof( float ) * 1 );
        //std::cout << "create scanline catalogue pour masq ep, longeur Y est " << mY << std::endl;
        scanLineR1=(float *) CPLMalloc( sizeof( float ) * mY );
        //sLR1=(float *) CPLMalloc( sizeof( float ) * mY );
        if( mDSmaskR1 == NULL){
            std::cout << "masque EP pas chargé correctement" << std::endl;
            //GDALClose( mDSmaskEP );
            return 0;
        }
    } else {
        std::cout << "masque EP pas trouvé " <<getNameMasqueEP() << std::endl;
    }

    int c(0);
    for (tuileS2OneDateSco * t : mVProdutsOK){
        // si une seule tuile pose un problème lors du chargement des dataset, on annule tout les traitement
        if (!t->openDS()){
            // fermer tout les datasets des tuiles déjà passée en revue précédemment
            for (int i(0);i<c;i++){
                mVProdutsOK.at(i)->closeDS();
            }

            aRes=0;
            break;
        }
        c++;
    }

    if (aRes==0){
        std::cout << "il manque certaines carte dans la série temporelle" << std::endl;
    } else {
        // création des raster résultats

        const char *pszFormat = "MEM";
        GDALDriver * pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        if( pDriver != NULL )
        {
            if (debugDetail){std::cout << " création des couches résultats" << std::endl;}
            for (int y : mYs){
                // MEM raster : le nom output ne sert à rien mais il faut en donner un
                std::string output="toto";
                const char *out=output.c_str();
                GDALDataset  * ds = pDriver->CreateCopy(out,mDSmaskR1,FALSE, NULL,NULL, NULL );
                mMapResults.emplace(std::make_pair(y,ds));

                mSL.emplace(std::make_pair(y,(float *) CPLMalloc( sizeof( float ) * mY )));
                // même chose pour les délais de coupe et la première date de détection d'un problème (cf besoin de Arthur)
                if (doDelaisCoupe){
                    if (debugDetail){std::cout << " delais de coupe année " << y << std::endl;}
                    GDALDataset  * ds2 = pDriver->CreateCopy(out,mDSmaskR1,FALSE, NULL,NULL, NULL );
                    ds2->GetRasterBand(1)->SetNoDataValue(255);
                    mMapDelaisCoupe.emplace(std::make_pair(y,ds2));
                }
                if (doFirstDateSco){
                    GDALDataset  * ds3 = pDriver->CreateCopy(out,mDSmaskR1,FALSE, NULL,NULL, NULL );
                    ds3->GetRasterBand(1)->SetNoDataValue(255);
                    mMapFirstDateSco.emplace(std::make_pair(y,ds3));
                }
            }
        }
        std::cout << "done" << std::endl;
    }
    return aRes;
}

void catalogueSco::writeRes1pos(TS1Pos * ts) const{
    // un scanpix qui est redéfini à chaque fois car sinon il sera partagé durant le calcul en parallèle et ça va provoquer des merdes par moment
    float * scanPix2=(float *) CPLMalloc( sizeof( float ) * 1 );
    for (int y : mYs){
        scanPix2[0]=ts->mVRes.at(y);
        //std::cout << "ts res pour y " << y << " est " << scanPix[0] << ", écriture à la position " << ts->mU << " , " << ts->mV << std::endl;
        mMapResults.at(y)->GetRasterBand(1)->RasterIO( GF_Write,  ts->mV,ts->mU,1,1,scanPix2 , 1, 1,GDT_Float32, 0, 0 );
        // délais de coupe
        if (doDelaisCoupe){
            scanPix2[0]=ts->getDelaisCoupe(y);
            mMapDelaisCoupe.at(y)->GetRasterBand(1)->RasterIO( GF_Write,  ts->mV,ts->mU,1,1,scanPix2 , 1, 1,GDT_Float32, 0, 0 );
        }
        if (doFirstDateSco){
            scanPix2[0]=ts->getDelaisCoupe(y,1);
            mMapFirstDateSco.at(y)->GetRasterBand(1)->RasterIO( GF_Write,  ts->mV,ts->mU,1,1,scanPix2 , 1, 1,GDT_Float32, 0, 0 );
        }
    }
    CPLFree(scanPix2);
}


// lors de la création initiale du catalogue pour une nouvelle tuile, il faut créer le masque pour cette zone.
void catalogueSco::createMaskForTuile(){
    if (mDebug){std::cout << " create Mask For tuile start" << std::endl;}
    std::string masqueRW(EP_mask_path);
    int epsg=getEPSG();
    if (mDebug){std::cout << " EPSG = " << epsg << std::endl;}

    std::string out=getNameMasque();

    if (boost::filesystem::exists(masqueRW)){
        if (!boost::filesystem::exists(out) | overw){
            std::cout << " create Mask For tuile " << std::endl;
            if (mVProduts.size()>0){
                std::string aCommand="gdalwarp -te "+std::to_string(mVProduts.at(0)->mXmin)+" "+std::to_string(mVProduts.at(0)->mYmin)+" "+std::to_string(mVProduts.at(0)->mXmax)+" "+std::to_string(mVProduts.at(0)->mYmax)+ " -t_srs EPSG:"+std::to_string(epsg)+" -ot Byte -overwrite -tr 10 10 "+ masqueRW+ " "+ out;
                std::cout << aCommand << std::endl;
                system(aCommand.c_str());
                aCommand="gdalwarp -ot Byte -overwrite -tr 20 20 -r max "+out+ " "+ getNameMasque(2);
                system(aCommand.c_str());
            }
        }

    } else { std::cout << "Je n'ai pas le masque EP pour la RW \n\n\n\n\n!!" << masqueRW << std::endl;}
}

// les choses sérieuses commencent ici
void catalogueSco::analyseTSinit(){
    // je commence par initialiser un vecteur de tuile avec uniquement les produits ok, cad sans trop de nuage
    // et identification des différentes années couvertes par la TS en mm temps
    // je peux choisir si je travaille année par année ou si je traite toute la série tempo d'un coup. C'était surtout utile durant le développement, car au début le temps de calcul était trop long pour toute la série tempo. Maintenant pas très utile comme option
    // c'est ici également que j'accède au mode "description de la TS pour un point donné"
    std::cout << "analyse TS init " << std::endl;
    std::vector<int> aVYs;

    //std::cout << "analyse pour toute les années d'un coup ------------------" << std::endl;


    if (Xdebug>0 && Ydebug >0){
        // mode test pour une position
        analyseTSTest1pixel(Xdebug,Ydebug,globResXYTest);
    } else if(globVPts.size()>0) {
        // mode test pour un ensemble de position (et sauver résultat dans fichiers)
        std::cout << " analyse en mode Test pour une liste de " << globVPts.size() << "points " << std::endl;
        for (pts & pt : globVPts){
            //analyseTSTest1pixel(pt.X(),pt.Y(),globResXYTest+std::to_string((int) pt.X())+"_"+std::to_string((int) pt.Y())+".txt");
            analyseTSTest1pixel(pt.X(),pt.Y(),globResXYTest+"_"+std::to_string(pt.getID())+".txt");
        }

    }else if (doAnaTS){
        analyseTS();
    }

}


void copyStyleES(std::string tifPath){

    std::string aOut=tifPath.substr(0,tifPath.size()-3)+"qml";
    if (boost::filesystem::exists(aOut)){boost::filesystem::remove(aOut);}
    std::string aIn("/home/gef/app/s2/documentation/etatSanitaire_.qml");
    if (boost::filesystem::exists(aIn)){
        // boost::filesystem::copy_file(aIn,aOut);
    }
}

void catalogueSco::closeDS(){
    std::cout << "fermeture de tout les raster de la TS" << std::endl;
    for (tuileS2OneDateSco * t : mVProdutsOK){
        t->closeDS();
    }
    if( mDSmaskR1 != NULL){ GDALClose( mDSmaskR1 );}
    CPLFree(scanPix);

    const char *pszFormat = "GTiff";
    GDALDriver * pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    // c'est à ce moment qu'on sauve au format tif nos résultats
    for (auto kv : mMapResults){
        GDALDataset  * ds = pDriver->CreateCopy(getNameES(kv.first).c_str(),kv.second,FALSE, NULL,NULL, NULL );
        // on va également copier le fichier de style qml
        copyStyleES(getNameES(kv.first));
        GDALClose( kv.second);
        GDALClose(ds);
    }
    if (doDelaisCoupe){
        for (auto kv : mMapDelaisCoupe){
            //const char *out2=getNameDelaisCoupe(kv.first).c_str(); // cette ligne est erronée ; le string retourné par la fonction est une variables temporaire, et donc un pointeur vers cette variable va être déréférencé
            //std::cout << "export date délais coupe " << getNameDelaisCoupe(kv.first) << std::endl;
            GDALDataset  * ds = pDriver->CreateCopy(getNameDelaisCoupe(kv.first).c_str(),kv.second,FALSE, NULL,NULL, NULL );
            GDALClose(kv.second);
            GDALClose(ds);
        }
    }
    if (doFirstDateSco){
        for (auto kv : mMapFirstDateSco){
            //std::cout << "export date première attaque " << getNameFirstDateSco(kv.first) << std::endl;
            GDALDataset  * ds = pDriver->CreateCopy(getNameFirstDateSco(kv.first).c_str(),kv.second,FALSE, NULL,NULL, NULL );
            GDALClose(kv.second);
            GDALClose(ds);
        }
    }
    CPLFree(scanLineR1);
    // CPLFree(sLR1);
    for (auto kv : mSL){
        CPLFree(kv.second);
    }
}
// copier le fichier de style QML pour Etat Sanitaire
void copyStyleES(std::string tifPath);
