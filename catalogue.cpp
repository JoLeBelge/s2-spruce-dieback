#include "catalogue.h"

extern std::string wd;
extern std::string buildDir;
extern std::string path_otb;
extern std::string EP_mask_path;
//extern std::string iprfwFile;
extern int globSeuilCC;
double seuilCR(1.7); // on est encore bien au dessus de ce que font les français
extern int year_analyse;
extern double Xdebug;
extern double Ydebug;

extern bool overw;
extern bool debugDetail;
extern int nbDaysStress;

extern std::string globTuile;
extern std::string globResXYTest;
extern std::string XYtestFile;

bool doAnaTS(1);
bool docleanTS1pos(1);// nettoyage TS d'un point. défaut oui. Mais si pt en dehors du masque, faut pas nettoyer^
extern bool mDebug;

std::vector<pts> globVPts;

catalogue::catalogue(std::string aJsonFile){
    // parse le json de la requete
    if ( !boost::filesystem::exists( aJsonFile ) )
    {
        std::cout << "Can't find file " << aJsonFile << std::endl;
    } else {
        std::cout << "création de la collection depuis le résultat de la requete serveur theia " << std::endl;
        std::ifstream in(aJsonFile);
        std::ostringstream sstream;
        sstream << in.rdbuf();
        const std::string str(sstream.str());
        const char* ptr = str.c_str();
        in.close();
        Document document;
        document.Parse(ptr);

        const Value& f = document["features"];
        if (f.IsArray()){
            for (SizeType i = 0; i < document["features"].Size(); i++){
                // je récupère les infos qui m'intéressent sur le produits
                tuileS2OneDate * t=new tuileS2OneDate();
                t->mProd =f[i]["properties"]["productIdentifier"].GetString();
                t->mFeature_id = f[i]["id"].GetString();
                if (f[i]["properties"]["cloudCover"].IsInt()){t->mCloudCover = f[i]["properties"]["cloudCover"].GetInt();}
                t->mAcqDate =f[i]["properties"]["startDate"].GetString();
                t->mDate=ymdFromString(t->mAcqDate);
                t->mPtrDate=& t->mDate;
                t->mProdDate =f[i]["properties"]["productionDate"].GetString();
                //t->mPubDate =f[i]["properties"]["published"].GetString();
                mVProduts.push_back(t);
            }
        }

        std::for_each(
                    std::execution::par_unseq,
                    mVProduts.begin(),
                    mVProduts.end(),
                    [](tuileS2OneDate * t)
        {

            // check que cloudcover est en dessous de notre seuil
            if (t->mCloudCover<globSeuilCC){
                //check si le produit existe déjà décompressé avant de le télécharger
                if (!t->pretraitementDone()){
                    t->download();
                    t->nettoyeArchive();
                    t->decompresse();
                    t->removeArchive();
                } else {
                    std::cout << t->mProd << " a déjà été téléchargé précédemment" << std::endl;
                }
                t->readXML();
                //t.catQual();

            }
        });

        traitement();

    }
}

catalogue::catalogue(){
    // listing des dossiers dans intermediate puis lecture des métadonnées XML
    std::cout << "création de la collection depuis les dossiers présent dans le répertoire " << wd << "intermediate/ " << std::endl;
    for(auto & p : boost::filesystem::directory_iterator(wd+"intermediate/")){
        std::string aDecompressDirName= p.path().filename().string();
        // création d'une tuile
        tuileS2OneDate * t=new tuileS2OneDate();
        t->decompressDirName =aDecompressDirName;
        t->readXML();

        mVProduts.push_back(t);
    }
    std::cout << " done .." << std::endl;


    traitement();
}

void catalogue::traitement(){
    // récapitulatif;
    summary();

    createMaskForTuile();

    // boucle sans multithread pour les traitements d'image
    std::for_each(
                std::execution::seq,
                mVProduts.begin(),
                mVProduts.end(),
                [](tuileS2OneDate* t)
    {
        // check que cloudcover est en dessous de notre seuil
        if (t->mCloudCover<globSeuilCC){
            t->masque();
            t->resample();
            t->computeCR();
            t->masqueSpecifique();
            t->normaliseCR();
            if (mDebug){std::cout << std::endl ;}
        }
    });
    //std::cout << " boucle sur les tuiles done" << std::endl;

    // points pour visu de la série tempo et pour vérifier la fct harmonique
    if (XYtestFile!="toto"){
         if (globResXYTest=="toto"){globResXYTest=XYtestFile.substr(0,XYtestFile.size-4);}
        globVPts=readPtsFile(XYtestFile);
    }

    if (doAnaTS){
        analyseTSinit();
    }
}

// comptage des produits avec cloudcover ok
int catalogue::countValid(){
    int aRes(0);
    for (const tuileS2OneDate * t : mVProduts){
        if (t->mCloudCover<globSeuilCC){aRes++;}
    }
    return aRes;
}

// les choses sérieuses commencent ici
void catalogue::analyseTSinit(){
    // je commence par initialiser un vecteur de tuile avec uniquement les produits ok, cad sans trop de nuage
    // et identification des différentes années couvertes par la TS en mm temps
    // je peux choisir si je travaille année par année ou si je traite toute la série tempo d'un coup. C'était surtout utile durant le développement, car au début le temps de calcul était trop long pour toute la série tempo. Maintenant pas très utile comme option
    // c'est ici également que j'accède au mode "description de la TS pour un point donné"
    std::cout << "analyse TS init " << std::endl;
    std::vector<int> aVYs;

    std::cout << "analyse pour toute les années d'un coup ------------------" << std::endl;
    for (tuileS2OneDate * t : mVProduts){
        if (t->mCloudCover<globSeuilCC){
            if (std::find(mYs.begin(), mYs.end(), t->gety()) == mYs.end()){mYs.push_back(t->gety());}
            mVProdutsOK.push_back(t);
        }
    }
    std::sort(mVProdutsOK.begin(), mVProdutsOK.end(), PointerCompare());

    if (Xdebug>0 && Ydebug >0){
        // mode test pour une position
        analyseTSTest1pixel(Xdebug,Ydebug,globResXYTest);
    } else if(globVPts.size()>0) {
        // mode test pour un ensemble de position (et sauver résultat dans fichiers)
        std::cout << " analyse en mode Test pour une liste de " << globVPts.size() << "points " << std::endl;
        for (pts & pt : globVPts){
            //analyseTSTest1pixel(pt.X(),pt.Y(),globResXYTest+std::to_string((int) pt.X())+"_"+std::to_string((int) pt.Y())+".txt");
            analyseTSTest1pixel(pt.X(),pt.Y(),globResXYTest+"_"+std::to_string(pt.ID())+".txt");
        }

    }else{
        analyseTS();
    }

}

void catalogue::analyseTS(){

    if (openDS()){

        int nb = mVProdutsOK.size();
        int c(0);
        int step=mY/20;
        int count(0);

        // Le calcul parallèle pixel par pixel nécessite de charger toute les cartes en Mémoire. fonctionne pour une année, mais pas pour la série tempo complête --> pas assez de mémoire vive.
        // traitement ligne par ligne pour pouvoir travailler en parallèle.
        for ( int row = 0; row < mY; row++ ){
            // lecture masque ep
            readMasqLine(row);

#pragma omp parallel num_threads(8)
            {
#pragma omp for
                for (tuileS2OneDate * t : mVProdutsOK){
                    //std::cout << "tuiles  " << t->getDate()<< std::endl;
                    t->readCRnormLine(row);
                    t->readMasqLine(row);
                    t->computeCodeLine();
                }
            }

            std::vector<int> aVCol;
            for (int col=0 ; col<mX;col++){
                if (scanLine[col]==1){aVCol.push_back(col);}
            }

            //num_threads(8) shared(row,mYs,seuilCR,scanLine,mVProdutsOK)
#pragma omp parallel num_threads(12)
            {
#pragma omp for
                for (int col : aVCol){
                    //int tid = omp_get_thread_num();
                    //printf("Hello world from omp thread %d\n", tid);
                    TS1Pos ts(row,col,&mYs,nb);

                    for (const tuileS2OneDate * t : mVProdutsOK){
                        ts.add1Date(t->getymdPt(),t->getCodeLine(col));
                    }

                    ts.nettoyer();
                    ts.analyse();
                    //#pragma omp critical
                    //                         {
                    writeRes1pos(&ts);
                    //                   }
                }
            } // end pragma
            c++;
            if (c%step==0){
                count++;
                std::cout << count*5 << "%..." << std::endl;
            }
        }

        closeDS();
    }
}

void catalogue::analyseTSTest1pixel(double X, double Y, std::string aFileOut){

    std::cout << "Test pour une position : " << X << "," << Y << " avec seuil ratio CRSWIR/CRSWIRtheorique(date) =" << seuilCR << " et nombre de jours de stress après lesquel on interdit un retour à la normal de " << nbDaysStress << ", tuile " << globTuile <<  std::endl;
    int nb = mVProdutsOK.size();
    pts pt(X,Y);

    TS1PosTest ts(&mYs,nb,pt);
    //std::cout << "TS1PosTest créé." << std::endl;
    for (tuileS2OneDate * t : mVProdutsOK){
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
    if (docleanTS1pos){ts.nettoyer();}

    ts.analyse();
    ts.printDetail(aFileOut);
}

int catalogue::getMasqEPVal(int aCol, int aRow){
    //std::cout << "getMasqEpVal " << std::endl;
    int aRes=0;

    if( mDSmaskEP != NULL && mDSmaskEP->GetRasterBand(1)->GetXSize() > aCol && mDSmaskEP->GetRasterBand(1)->GetYSize() > aRow && aRow >=0 && aCol >=0){
        mDSmaskEP->GetRasterBand(1)->RasterIO( GF_Read, aCol, aRow, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
        aRes=scanPix[0];
    }

    return aRes;
}

void catalogue::closeDS(){
    std::cout << "fermeture de tout les raster de la TS" << std::endl;
    for (tuileS2OneDate * t : mVProdutsOK){
        t->closeDS();
    }
    if( mDSmaskEP != NULL){ GDALClose( mDSmaskEP );}
    CPLFree(scanPix);

    const char *pszFormat = "GTiff";
    GDALDriver * pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);

    for (auto kv : mMapZScolTS){
        // c'est à ce moment qu'on sauve au format tif no petit résultats chéri
        std::string output(wd+"etatSanitaire_"+globTuile+"_"+std::to_string(kv.first)+".tif");
        const char *out=output.c_str();
        GDALDataset  * ds = pDriver->CreateCopy(out,kv.second,FALSE, NULL,NULL, NULL );

        // on va également copier le fichier de style qml
        copyStyleES(output);
        GDALClose( kv.second);
        GDALClose(ds);
    }
    CPLFree(scanLine);
}

bool catalogue::openDS(){
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
        mDSmaskEP= pDriver->CreateCopy( ch,mask,FALSE, NULL,NULL, NULL );
        mX=mask->GetRasterBand(1)->GetXSize();
        mY=mask->GetRasterBand(1)->GetYSize();
        GDALClose( mask );

        //mDSmaskEP= (GDALDataset *) GDALOpen( getNameMasqueEP().c_str(), GA_ReadOnly );
        scanPix=(float *) CPLMalloc( sizeof( float ) * 1 );
        //std::cout << "create scanline catalogue pour masq ep, longeur Y est " << mY << std::endl;
        scanLine=(float *) CPLMalloc( sizeof( float ) * mY );
        if( mDSmaskEP == NULL){
            std::cout << "masque EP pas chargé correctement" << std::endl;
            //GDALClose( mDSmaskEP );
            return 0;
        }
    } else {
        std::cout << "masque EP pas trouvé " <<getNameMasqueEP() << std::endl;
    }

    int c(0);
    for (tuileS2OneDate * t : mVProdutsOK){
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
            for (int y : mYs){
                // MEM raster
                std::string output(wd+"output/etatSanitaire_"+globTuile+"_"+std::to_string(y));
                const char *out=output.c_str();
                GDALDataset  * ds = pDriver->CreateCopy(out,mDSmaskEP,FALSE, NULL,NULL, NULL );
                mMapZScolTS.emplace(std::make_pair(y,ds));
            }
        }
        std::cout << "done" << std::endl;
    }
    return aRes;
}

void catalogue::writeRes1pos(TS1Pos * ts) const{
    // un scanpix qui est redéfini à chaque fois car sinon il sera partagé durant le calcul en parallèle et ça va provoquer des merdes par moment
    float * scanPix2=(float *) CPLMalloc( sizeof( float ) * 1 );
    for (int y : mYs){
        scanPix2[0]=ts->mVRes.at(y);
        //std::cout << "ts res pour y " << y << " est " << scanPix[0] << ", écriture à la position " << ts->mU << " , " << ts->mV << std::endl;
        mMapZScolTS.at(y)->GetRasterBand(1)->RasterIO( GF_Write,  ts->mV,ts->mU,1,1,scanPix2 , 1, 1,GDT_Float32, 0, 0 );
    }
    CPLFree(scanPix2);
}

void catalogue::readMasqLine(int aRow){
    if( mDSmaskEP != NULL && mDSmaskEP->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){

        mDSmaskEP->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, mX, 1, scanLine, mX,1, GDT_Float32, 0, 0 );
    }else {
        std::cout << "readMasqLine ; failed " << std::endl;
    }

}

// lors de la création initiale du catalogue pour une nouvelle tuile, il faut créer le masque pour cette zone.
void catalogue::createMaskForTuile(){
    if (mDebug){std::cout << " create Mask For tuile start" << std::endl;}
    std::string masqueRW(EP_mask_path+"masque_EP.tif");
    int epsg(32631);
    if (globTuile=="T32ULU"){
        masqueRW=EP_mask_path+"masque_resineux.tif";
        epsg=32632;
    }
    std::string out=getNameMasqueEP();

    if (boost::filesystem::exists(masqueRW)){
        if (!boost::filesystem::exists(out) | overw){
            std::cout << " create Mask For tuile " << std::endl;
            if (mVProduts.size()>0){
                std::string aCommand="gdalwarp -te "+std::to_string(mVProduts.at(0)->mXmin)+" "+std::to_string(mVProduts.at(0)->mYmin)+" "+std::to_string(mVProduts.at(0)->mXmax)+" "+std::to_string(mVProduts.at(0)->mYmax)+ " -t_srs EPSG:"+std::to_string(epsg)+" -ot Byte -overwrite -tr 10 10 "+ masqueRW+ " "+ out;
                std::cout << aCommand << std::endl;
                system(aCommand.c_str());
                aCommand="gdalwarp -ot Byte -overwrite -tr 20 20 -r max "+out+ " "+ getNameMasqueEP(2);
                system(aCommand.c_str());
            }
        }

    } else { std::cout << "Je n'ai pas le masque EP pour la RW \n\n\n\n\n!!" << masqueRW << std::endl;}
}

void catalogue::summary(){
        if (mDebug){for (tuileS2OneDate * t : mVProduts){t->cat();}}
        std::cout << " Nombre de produits ok ; " << countValid() << std::endl;
}

void copyStyleES(std::string tifPath){

    std::string aOut=tifPath.substr(0,tifPath.size()-3)+"qml";
    if (boost::filesystem::exists(aOut)){boost::filesystem::remove(aOut);}
    std::string aIn("../s2/documentation/etatSanitaire_.qml");
    if (boost::filesystem::exists(aIn)){
        boost::filesystem::copy_file(aIn,aOut);
    }
}
