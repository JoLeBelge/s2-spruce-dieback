#include "catalogue.h"

extern std::string wd;
extern std::string buildDir;
extern std::string path_otb;
extern std::string EP_mask_path;
//extern std::string iprfwFile;
extern int globSeuilCC;
double seuilCR(1.7);
extern int year_analyse;
extern double Xdebug;
extern double Ydebug;

extern std::string globTuile;

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
                tuileS2 * t=new tuileS2();
                t->mProd =f[i]["properties"]["productIdentifier"].GetString();
                t->mFeature_id = f[i]["id"].GetString();
                if (f[i]["properties"]["cloudCover"].IsInt()){t->mCloudCover = f[i]["properties"]["cloudCover"].GetInt();}
                t->mAcqDate =f[i]["properties"]["startDate"].GetString();
                t->mDate=ymdFromString(t->mAcqDate);
                t->mProdDate =f[i]["properties"]["productionDate"].GetString();
                //t->mPubDate =f[i]["properties"]["published"].GetString();
                mVProduts.push_back(t);
            }
        }

        std::for_each(
                    std::execution::par_unseq,
                    mVProduts.begin(),
                    mVProduts.end(),
                    [](tuileS2 * t)
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
        tuileS2 * t=new tuileS2();
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

    // boucle sans multithread pour les traitements d'image
    std::for_each(
                std::execution::seq,
                mVProduts.begin(),
                mVProduts.end(),
                [](tuileS2* t)
    {
        // check que cloudcover est en dessous de notre seuil
        if (t->mCloudCover<globSeuilCC){
            t->masque();
            t->resample();
            t->computeCR();
            t->masqueSpecifique();
            t->normaliseCR();
            std::cout << std::endl ;
        }
    });

    // points pour visu de la série tempo et pour vérifier la fct harmonique
    // attention, ce code n'a jamais été utilisé, finalement j'ai récupéré la fct harmonique de dutrieux
    //std::vector<pts> aVPts=readPtsFile(iprfwFile);
    //extractRatioForPts(& aVPts);

    analyseTSinit();
}

// comptage des produits avec cloudcover ok
int catalogue::countValid(){
    int aRes(0);
    for (tuileS2 * t : mVProduts){
        if (t->mCloudCover<globSeuilCC){aRes++;}
    }
    return aRes;
}

/*
void catalogue::extractRatioForPts(std::vector<pts> * aVpts){
    std::cout << " extractRatioForPts : " << aVpts->size() << " pts " << std::endl;

    std::for_each(
                std::execution::seq,
                mVProduts.begin(),
                mVProduts.end(),
                [&](tuileS2* t)
    {
        // check que cloudcover est en dessous de notre seuil
        if (t->mCloudCover<globSeuilCC){
            std::cout << " extraction valeurs pour " << t->getDate() << std::endl;
            for (pts & pt : *aVpts){
                pt.mCRVals.emplace(std::make_pair(t->getDate(),t->getCRSWIR(pt)));
                pt.mVMasqVals.emplace(std::make_pair(t->getDate(),t->getMaskSolNu(pt)));
            }
        }
    });

    // j'exporte les résulats

    std::ofstream aOut(iprfwFile.substr(0,iprfwFile.size()-4)+"_s2_ts.csv");
    aOut.precision(3);


    bool testFirstL(1);
    for (pts & pt : *aVpts){
        if (testFirstL){
            // header : les dates
            aOut << pt.catHeader() << "\n";
            testFirstL=0;
        }
        aOut << pt.catVal() << "\n";
    }
    aOut.close();

}
*/

// les choses sérieuses commencent ici
void catalogue::analyseTSinit(){
    // je commence par initialiser un vecteur de tuile avec uniquement les produits ok, cad sans trop de nuage
    // et identification des différentes années couvertes par la TS en mm temps
    // je pourrais étudier année par année histoire de soulager le temps de calcul? ici je ne garderai que l'année de mon choix
    // ok si fait année par année le traitement est très rapide. 10 minute. Là ou c'est galère si je fait les 4 ans d'un coup.
    // donc je travaille année par année
    // c'est ici également que j'accède au mode "description de la TS pour un point donné"


    std::vector<int> aVYs;
    // tentative de tout traiter d'un coup, toute les années
    if (year_analyse==666){
        std::cout << "analyse pour toute les années d'un coup ------------------" << std::endl;
        for (tuileS2 * t : mVProduts){
            if (t->mCloudCover<globSeuilCC){
                if (std::find(mYs.begin(), mYs.end(), t->gety()) == mYs.end()){mYs.push_back(t->gety());}
                mVProdutsOK.push_back(t);
            }
        }
        std::sort(mVProdutsOK.begin(), mVProdutsOK.end(), PointerCompare());

        if (Xdebug>0 && Ydebug >0){
            // mode test pour une position
            analyseTSTest1pixel();
        } else {
            analyseTS();
        }

    } else if (year_analyse==0){

        for (tuileS2 * t : mVProduts){
            //if (t->mCloudCover<globSeuilCC && t->gety()==2018){
            if (t->mCloudCover<globSeuilCC){
                if (std::find(aVYs.begin(), aVYs.end(), t->gety()) == aVYs.end()){aVYs.push_back(t->gety());}

            }
        }
    } else { aVYs.push_back(year_analyse);}
    std::sort (aVYs.begin(), aVYs.end());
    // je passe mYs comme argument à TS1Pos et vu que je veux faire date par date, je dois le modifier
    // attention, j'avais deux variable y!!
    for (int year : aVYs){
        mYs.clear();
        mYs.push_back(year);
        std::cout << "analyse pour l'année " << year << "------------------" << std::endl;

        for (tuileS2 * t : mVProduts){
            //if (t->mCloudCover<globSeuilCC && t->gety()==2018){
            if (t->mCloudCover<globSeuilCC && t->gety()==year){
                mVProdutsOK.push_back(t);
                //if (std::find(mYs.begin(), mYs.end(), t->gety()) == mYs.end()){mYs.push_back(t->gety());}
            }
        }
        std::sort(mVProdutsOK.begin(), mVProdutsOK.end(), PointerCompare());
        analyseTS();
    }
}

void catalogue::analyseTS(){

    if (openDS()){
        x=mVProdutsOK.at(0)->getXSize();
        y=mVProdutsOK.at(0)->getYSize();
        int nb = mVProdutsOK.size();
        // boucle pixel par pixel - en prenant le masque sol nu comme raster de base - pas bonne idée, je devrais ouvrir la carte ou j'ai le masque forestier car dans masque sol nu sont déjà intégré les no data nuage et edge
        //bool test(0);
        int c(0);
        int step=y/20;
        int count(0);

        /* Le calcul parallèle pixel par pixel nécessite de charger toute les cartes en Mémoire. fonctionne pour une année, mais pas pour la série tempo complête --> pas assez de mémoire vive.
        for ( int row = 0; row < y; row++ ){
            //for (int col = 0; col < x; col++)
            //{
            std::vector<int> r(x);
            std::iota(std::begin(r), std::end(r), 0);
            std::for_each(std::execution::par, std::begin(r), std::end(r), [&](int col) {
                // check carte 1 pour voir si no data ou pas
                int m =getMasqEPVal(col,row);
                if (m==1){
                    TS1Pos ts(row,col,&mYs,nb);
                    for (tuileS2 * t : mVProdutsOK){

                        // 0; ND
                         // 1: valeur CR swir ok.
                         // 2; valeur CRswir NOK
                           3 ; sol nu.

                        double crnorm=t->getCRnormVal(col,row);
                        int solnu = t->getMasqVal(col,row);
                        int code=0;
                        if (solnu==0){code=0;
                        }else if(solnu==2 | solnu==3){
                            code=3;
                        } else if (crnorm<=seuilCR) {
                            code=1;
                        } else if (crnorm>seuilCR) {
                            code=2;
                        }
                        ts.add1Date(t->getymdPt(),code);
                    }
                    ts.analyse();
                    writeRes1pos(&ts);
                }
            //}
            });
            c++;
            if (c%step==0){
                count++;
                std::cout << count*5 << "%..." << std::endl;
                //break;
            }
        }
        //    });

        */
        // traitement ligne par ligne pour pouvoir travailler en parallèle. Réduction extra du temps de calcul.

        for ( int row = 0; row < y; row++ ){
            // lecture masque ep
            readMasqLine(row);
            for (tuileS2 * t : mVProdutsOK){
                t->readCRnormLine(row);
                t->readMasqLine(row);
            }
            std::vector<int> r(x);
            std::iota(std::begin(r), std::end(r), 0);
            std::for_each(std::execution::par, std::begin(r), std::end(r), [&](int col) {

                if (scanLine[col]==1){

                    TS1Pos ts(row,col,&mYs,nb);
                    for (tuileS2 * t : mVProdutsOK){
                        // lit la valeur depuis la scanline
                        double crnorm=t->getCRnormVal(col);
                        int solnu = t->getMasqVal(col);
                        //std::cout << " crnorm " << crnorm << " , sol nu " << solnu << std::endl;
                        int code=0;
                        if (solnu==0){code=0;
                        }else if(solnu==2 | solnu==3){
                            code=3;
                        } else if (crnorm<=seuilCR) {
                            code=1;
                        } else if (crnorm>seuilCR) {
                            code=2;
                        }

                        ts.add1Date(t->getymdPt(),code);
                    }
                    ts.nettoyer();
                    ts.analyse();
                    writeRes1pos(&ts);

                }
            });
            //}
            c++;
            if (c%step==0){
                count++;
                std::cout << count*5 << "%..." << std::endl;
            }
        }

        closeDS();
    }

}

void catalogue::analyseTSTest1pixel(){
    // attention, openDS va avoir pour effet d'écraser les couches résultats (c'est pensé comme ça)
    std::cout << "Test pour une position : " << Xdebug << "," << Ydebug << " avec seuil ratio CRSWIR/CRSWIRtheorique(date) =" << seuilCR << std::endl;

    //std::cout << "attention, les couches d'états de résultats seront vides à la fin du test pour une position donnée." << std::endl;
    // pourquoi je dois ouvrir les dataset en fait? pour vérifier que tout les raster existent? ou c'est pour les raster résultats?
    // if (openDS()){

    int nb = mVProdutsOK.size();
    pts pt(Xdebug,Ydebug);

    TS1PosTest ts(&mYs,nb,pt);
    std::cout << "TS1PosTest créé." << std::endl;
    for (tuileS2 * t : mVProdutsOK){

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
        } else if (crnorm<=seuilCR) {
            code=1;
        } else if (crnorm>seuilCR) {
            code=2;
        }
        ts.add1Date(code,t);
    }
    ts.nettoyer();
    ts.analyse();
    ts.printDetail();
    //  closeDS();
    //}
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
    for (tuileS2 * t : mVProdutsOK){
        t->closeDS();
    }
    if( mDSmaskEP != NULL){ GDALClose( mDSmaskEP );}
    CPLFree(scanPix);

    const char *pszFormat = "GTiff";
    GDALDriver * pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);

    for (auto kv : mMapZScolTS){
        // c'est à ce moment qu'on sauve au format tif no petit résultats chéri
        std::string output(wd+"output/etatSanitaire_"+std::to_string(kv.first)+".tif");
        const char *out=output.c_str();
        GDALDataset  * ds = pDriver->CreateCopy(out,kv.second,FALSE, NULL,NULL, NULL );
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

        mDSmaskEP= pDriver->CreateCopy( ch,mask,FALSE, NULL,NULL, NULL );
        GDALClose( mask );

        //mDSmaskEP= (GDALDataset *) GDALOpen( getNameMasqueEP().c_str(), GA_ReadOnly );
        scanPix=(float *) CPLMalloc( sizeof( float ) * 1 );
        scanLine=(float *) CPLMalloc( sizeof( float ) * y );
        if( mDSmaskEP == NULL){
            std::cout << "masque EP pas chargé correctement" << std::endl;
            //GDALClose( mDSmaskEP );
            return 0;
        }
    } else {
        std::cout << "masque EP pas trouvé " <<getNameMasqueEP() << std::endl;
    }

    int c(0);
    for (tuileS2 * t : mVProdutsOK){
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
        //const char *pszFormat = "GTiff";

        const char *pszFormat = "MEM";

        GDALDriver * pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        if( pDriver != NULL )
        {
            for (int y : mYs){
                std::string output(wd+"output/etatSanitaire_"+globTuile+"_"+std::to_string(y)+".tif");
                const char *out=output.c_str();
                GDALDataset  * ds = pDriver->CreateCopy(out,mDSmaskEP,FALSE, NULL,NULL, NULL );
                /* je referme pour réouvrir en mode édition
            if( ds != NULL ){ GDALClose( ds );}
            GDALDataset  * ds2 = (GDALDataset *) GDALOpen( out, GA_Update);
            */
                mMapZScolTS.emplace(std::make_pair(y,ds));
            }
        }
        std::cout << "done" << std::endl;
    }
    return aRes;
}

void catalogue::writeRes1pos(TS1Pos * ts){
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
        //std::cout << "readMasqLine" << std::endl;
        mDSmaskEP->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, x, 1, scanLine, x,1, GDT_Float32, 0, 0 );
    }
}

