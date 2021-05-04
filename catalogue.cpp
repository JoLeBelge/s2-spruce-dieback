#include "catalogue.h"

extern std::string wd;
extern std::string buildDir;
extern std::string path_otb;
extern std::string EP_mask_path;
extern std::string iprfwFile;
extern int globSeuilCC;

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

    analyseTS();
}

// comptage des produits avec cloudcover ok
int catalogue::countValid(){
    int aRes(0);
    for (tuileS2 * t : mVProduts){
        if (t->mCloudCover<globSeuilCC){aRes++;}
    }
    return aRes;
}

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

// les choses sérieuses commencent ici
void catalogue::analyseTS(){
    // je commence par initialiser un vecteur de tuile avec uniquement les produits ok, cad sans trop de nuage
    for (tuileS2 * t : mVProduts){
        if (t->mCloudCover<globSeuilCC){
            mVProdutsOK.push_back(t);
        }
    }

    std::sort(mVProdutsOK.begin(), mVProdutsOK.end(), PointerCompare());

    /*for (tuileS2 * t : mVProdutsOK){
           std::cout <<  " date " << t->getDate() << std::endl;
    }*/

    if (openDS()){

        int x=mVProdutsOK.at(0)->getXSize();
        int y=mVProdutsOK.at(0)->getYSize();
        // boucle pixel par pixel - en prenant le masque sol nu comme raster de base - pas bonne idée, je devrais ouvrir la carte ou j'ai le masque forestier car dans masque sol nu sont déjà intégré les no data nuage et edge
        bool test(0);

        for ( int row = 0; row < y; row++ )
        {
            for (int col = 0; col < x; col++)
            {
                // check carte 1 pour voir si no data ou pas
                int m =getMasqEPVal(col,row);
                if (m==1){

                    //std::vector<pDateEtat> mVTS;
                    TS1Pos ts(row,col);
                    for (tuileS2 * t : mVProdutsOK){

                        /* 0; ND
                         * 1: valeur CR swir ok.
                         * 2; valeur CRswir NOK
                           3 ; sol nu.*/
                        // je pourrais stoquer les val dans un vecteur. Puis coder des fct qui travaillent sur cette map, pour en tirer les info pertinente (ex ; état en 2018)
                        //std::cout << t->getDate() << ";" << t->getCRnormVal(col,row) << std::endl;
                        double crnorm=t->getCRnormVal(col,row);
                        int solnu = t->getMasqVal(col,row);
                        int code=0;
                        if (solnu==0){code=0;
                        }else if(solnu==2 | solnu==3){
                            code=3;
                        } else if (crnorm<1.5) {
                            code=1;
                        } else if (crnorm>1.5) {
                            code=2;
                        }
                        //mVTS.push_back(pDateEtat(t->getymd(),code));
                        ts.add1Date(t->getymd(),code);
                    }
                    //anaTSOnePosition(&mVTS);
                    ts.analyse();

                    test=1;
                    break;
                }
                if (test){break;}
            }
            if (test){break;}
        }

        closeDS();
    }
}

int catalogue::getMasqEPVal(int aCol, int aRow){
    //std::cout << "getMasqEpVal " << std::endl;
    int aRes=0;
    float * scanPix=(float *) CPLMalloc( sizeof( float ) * 1 );
    if( mDSmaskEP != NULL && mDSmaskEP->GetRasterBand(1)->GetXSize() > aCol && mDSmaskEP->GetRasterBand(1)->GetYSize() > aRow && aRow >=0 && aCol >=0){
        mDSmaskEP->GetRasterBand(1)->RasterIO( GF_Read, aCol, aRow, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
        aRes=scanPix[0];
    }
    CPLFree(scanPix);
    return aRes;
}

void catalogue::closeDS(){
    std::cout << "fermeture de tout les raster de la TS" << std::endl;
    for (tuileS2 * t : mVProdutsOK){
        t->closeDS();
    }
    if( mDSmaskEP != NULL){ GDALClose( mDSmaskEP );}
}


bool catalogue::openDS(){
    std::cout << "ouverture de tout les raster de la TS" << std::endl;
    bool aRes(1);

    if (exists(getNameMasqueEP())){
        mDSmaskEP= (GDALDataset *) GDALOpen( getNameMasqueEP().c_str(), GA_ReadOnly );
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
         std::cout << "done" << std::endl;
    }
    return aRes;
}

/*
void catalogue::anaTSOnePosition(std::vector<pDateEtat> * aVTS){

    // pour commencer, on analyse la TS dans son ensemble. une fois atteinte, la parcelle est notée comme atteinte pour toute les dates ultérieures jusqu'au moment ou celle-ci est éventuellement coupée
    int  nb (0);
    for (pDateEtat & p : *aVTS){
        if (p.getEtat()==2) {nb++;} else {nb=0;}

        //if (nb>2) {
            // on considère la parcelle comme touchée irrévocablement
            //p.SetStress(1);
            //noteStress();
            //break;
        //}         p.cat();
    }
}
*/

