#include "catalogueperiodpheno.h"


extern bool mDebug;

bool doAnaTS(1);
bool docleanTS1pos(1);// nettoyage TS d'un point. défaut oui. Mais si pt en dehors du masque, faut pas nettoyer
extern double Xdebug;
extern double Ydebug;

extern std::string wd;
extern std::string buildDir;
extern std::string path_otb;
extern int globSeuilCC;
extern bool overw;
extern bool debugDetail;

extern std::string globTuile;
extern std::string globResXYTest;

extern std::string EP_mask_path;

std::map<int,std::vector<double>> * cataloguePeriodPheno::getMeanRadByTri1Pt(double X, double Y){

    std::cout << "résumé radiation pour la position : " << X << "," << Y <<  std::endl;
    //std::cout << "-" <<  std::endl;
    int nb = mVProdutsOK.size();
    pts pt(X,Y);
    TS1PosTest ts(&mYs,nb,pt);
    for (tuileS2OneDate * t : mVProdutsOK){
        int code=0;
        ts.add1Date(code,t);
    }
    ts.nettoyer(); // nettoyage classic ; se base sur le masque. hors le masque (nuage, edge, sol nul) n'est pas calculé en dehors du masque épicéa..

    return ts.summaryByTri();
}



bool cataloguePeriodPheno::openDS(){
    std::cout << "ouverture de tout les raster de la TS" << std::endl;
    bool aRes(1);

    if (exists(getNameMasque())){
        const char *pszFormat = "MEM";
        GDALDriver *pDriver= GetGDALDriverManager()->GetDriverByName(pszFormat);
        GDALDataset * mask=(GDALDataset *) GDALOpen( getNameMasque().c_str(), GA_ReadOnly );
        const char * ch="immem";
        if (mask==NULL){
            std::cout << "masque pas lu correctement" << std::endl;
            return 0;
        }
        mDSmask= pDriver->CreateCopy( ch,mask,FALSE, NULL,NULL, NULL );
        mX=mask->GetRasterBand(1)->GetXSize();
        mY=mask->GetRasterBand(1)->GetYSize();
        GDALClose( mask );

        scanPix=(float *) CPLMalloc( sizeof( float ) * 1 );
        //std::cout << "create scanline catalogue pour masq ep, longeur Y est " << mY << std::endl;
        scanLine=(float *) CPLMalloc( sizeof( float ) * mY );
        if( mDSmask == NULL){
            std::cout << "masque pas chargé correctement" << std::endl;
            return 0;
        }
    } else {
        std::cout << "masque pas trouvé " <<getNameMasque() << std::endl;
    }

    int c(0);
    for (tuileS2OneDatePheno * t : mVProdutsOK){
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
                GDALDataset  * ds = pDriver->CreateCopy(out,mDSmask,FALSE, NULL,NULL, NULL );
                mMapResults.emplace(std::make_pair(y,ds));
            }
        }
        std::cout << "done" << std::endl;
    }
    return aRes;
}

void cataloguePeriodPheno::createMaskForTuile(){
    if (mDebug){std::cout << " create Mask For tuile start" << std::endl;}
    std::string masque(EP_mask_path+"tmp.tif");
    int epsg(32631);
    if (globTuile=="T32ULU"){
        masque=EP_mask_path+"masque_resineux.tif";
        epsg=32632;
    }
    std::string out=getNameMasque();

    if (boost::filesystem::exists(masque)){
        if (!boost::filesystem::exists(out) | overw){
            std::cout << " create Mask For tuile " << std::endl;
            if (mVProduts.size()>0){
                std::string aCommand="gdalwarp -te "+std::to_string(mVProduts.at(0)->mXmin)+" "+std::to_string(mVProduts.at(0)->mYmin)+" "+std::to_string(mVProduts.at(0)->mXmax)+" "+std::to_string(mVProduts.at(0)->mYmax)+ " -t_srs EPSG:"+std::to_string(epsg)+" -ot Byte -overwrite -tr 10 10 "+ masque+ " "+ out;
                std::cout << aCommand << std::endl;
                system(aCommand.c_str());
                aCommand="gdalwarp -ot Byte -overwrite -tr 20 20 -r max "+out+ " "+ getNameMasque(2);
                system(aCommand.c_str());
            }
        }

    } else { std::cout << "Je n'ai pas le masque pour la RW \n\n\n\n\n!!" << masque << std::endl;}
}

void cataloguePeriodPheno::traitement(){
    // récapitulatif;
    summary();
    createMaskForTuile();

    for (tuileS2OneDate * t : mVProduts){
        if (t->mCloudCover<globSeuilCC){
            if (std::find(mYs.begin(), mYs.end(), t->gety()) == mYs.end()){mYs.push_back(t->gety());}
            mVProdutsOK.push_back(new tuileS2OneDatePheno(t, getNameMasque(1), getNameMasque(2)));
        }
    }
    std::sort(mVProdutsOK.begin(), mVProdutsOK.end(), PointerCompare());

    // boucle sans multithread pour les traitements d'image
    std::for_each(
                std::execution::seq,
                mVProdutsOK.begin(),
                mVProdutsOK.end(),
                [](tuileS2OneDatePheno * t)
    {
        t->masque();
        if (mDebug){std::cout << std::endl ;}

    });
    std::cout << " prétraitement fait" << std::endl;

    // on travaille avec deux résolutions différentes pour cette appli, une R1 et une R2

    // sélectionne pour chaque trimestre les tuileS2OneDatePheno, puis calcul moyenne de réflectance
    if (openDS()){
        for (int tri(1);tri<5;tri++){
            std::vector<tuileS2OneDatePheno *> aVTuileS2 = getTuileS2ForTri(tri);
            syntheseTempoRadiation(aVTuileS2,std::to_string(tri));
        }
        closeDS();
    }
}

std::vector<tuileS2OneDatePheno *> cataloguePeriodPheno::getTuileS2ForTri(int trimestre){
    std::vector<tuileS2OneDatePheno *> aRes;
    int m0(((trimestre-1)*3)+1);
    int m1(((trimestre-1)*3)+3);
    for (tuileS2OneDatePheno * t : mVProdutsOK){
        year_month_day * d=t->getymdPt();
        if (d->month()>=month{m0} && d->month()<=month{m1}){
            aRes.push_back(t);
        }
    }
    return aRes;
}

void cataloguePeriodPheno::syntheseTempoRadiation(std::vector<tuileS2OneDatePheno *> aVTuileS2, std::string aOutSuffix){

    int c(0);
    int step=mY/20;
    int count(0);

    for ( int row = 0; row < mY; row++ ){
        // lecture masque global dans scanLine
        readMasqLine(row);
        // lecture, pour chaque tuile, de la ligne pour le masque et des lignes pour les bandes R1
        //donc 4 scanlignes par tuiles
        for (tuileS2OneDatePheno * t : aVTuileS2){
            t->readLines(1,row);
        }

        for (int col=0 ; col<mX;col++){
            if (scanLine[col]==1){

                //



            }
        }

        c++;
        if (c%step==0){
            count++;
            std::cout << count*5 << "%..." << std::endl;
        }
    }
}


void cataloguePeriodPheno::closeDS(){
    std::cout << "fermeture de tout les raster de la TS" << std::endl;
    for (tuileS2OneDatePheno * t : mVProdutsOK){
        t->closeDS();
    }
    if( mDSmask != NULL){ GDALClose( mDSmask );}
    CPLFree(scanPix);

    const char *pszFormat = "GTiff";
    GDALDriver * pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    /*
    for (auto kv : mMapResults){
        // c'est à ce moment qu'on sauve au format tif no petit résultats chéri
        std::string output(wd+"etatSanitaire_"+globTuile+"_"+std::to_string(kv.first)+".tif");
        const char *out=output.c_str();
        GDALDataset  * ds = pDriver->CreateCopy(out,kv.second,FALSE, NULL,NULL, NULL );
        GDALClose( kv.second);
        GDALClose(ds);
    }*/
    CPLFree(scanLine);
}


