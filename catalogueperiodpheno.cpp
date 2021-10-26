#include "catalogueperiodpheno.h"

extern int yMax;
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

extern std::vector<std::string> vBR2;

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
        // pourquoi créer une copie? je ne le modifie pas ? peut-Etre que ça n'as pas de raison d'être
        mDSmaskR1= pDriver->CreateCopy( ch,mask,FALSE, NULL,NULL, NULL );
        mDSmaskR2=(GDALDataset *) GDALOpen( getNameMasque(2).c_str(), GA_ReadOnly );
        mX=mask->GetRasterBand(1)->GetXSize();
        mY=mask->GetRasterBand(1)->GetYSize();
        GDALClose( mask );

        scanPix=(float *) CPLMalloc( sizeof( float ) * 1 );
        //std::cout << "create scanline catalogue pour masq ep, longeur Y est " << mY << std::endl;
        scanLineR1=(float *) CPLMalloc( sizeof( float ) * mY );
        scanLineR2=(float *) CPLMalloc( sizeof( float ) * mY/2 );
        if( mDSmaskR1 == NULL){
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
    } /*else {
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
    }*/
    return aRes;
}

void cataloguePeriodPheno::createMaskForTuile(){
    if (mDebug){std::cout << " create Mask For tuile start" << std::endl;}
    std::string masque(EP_mask_path);
    int epsg(32631);
    if (globTuile=="T32ULU"){

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
        if (t->mCloudCover<globSeuilCC && t->gety()<2018) {
            std::cout << "ajout tuile " <<  t->getDate() <<std::endl;
            //if (std::find(mYs.begin(), mYs.end(), t->gety()) == mYs.end()){mYs.push_back(t->gety());}

            mVProdutsOK.push_back(new tuileS2OneDatePheno(t, getNameMasque(1), getNameMasque(2)));
        }
    }
    std::sort(mVProdutsOK.begin(), mVProdutsOK.end(), PointerCompare());
    //std::cout << "fait " <<std::endl;
    // boucle sans multithread pour les traitements d'image
    std::for_each(
                std::execution::seq,
                mVProdutsOK.begin(),
                mVProdutsOK.end(),
                [](tuileS2OneDatePheno * t)
    {
        //std::cout << "prétraitement " <<  t->getDate() <<std::endl;
        t->masque();
        if (mDebug){std::cout << std::endl ;}

    });
    //std::cout << " prétraitement fait" << std::endl;

    // on travaille avec deux résolutions différentes pour cette appli, une R1 et une R2

    // sélectionne pour chaque trimestre les tuileS2OneDatePheno, puis calcul moyenne de réflectance
    if (openDS()){
        for (int tri(1);tri<5;tri++){
            std::vector<tuileS2OneDatePheno *> aVTuileS2 = getTuileS2ForTri(tri);
            syntheseTempoRadiation(aVTuileS2,"tri"+std::to_string(tri));
        }
        closeDS();
    }
}

std::vector<tuileS2OneDatePheno *> cataloguePeriodPheno::getTuileS2ForTri(int trimestre){
    std::vector<tuileS2OneDatePheno *> aRes;
    std::cout << " Sélection de prise de vue pour trimetre " << trimestre << std::endl;
    int m0(((trimestre-1)*3)+1);
    int m1(((trimestre-1)*3)+3);
    for (tuileS2OneDatePheno * t : mVProdutsOK){
        year_month_day * d=t->getymdPt();
        if (d->month()>=month{m0} && d->month()<=month{m1}){
            aRes.push_back(t);
            //std::cout << t->getDate() << std::endl;
        }
    }
    std::cout << "nombre de dates " << aRes.size() << std::endl;
    return aRes;
}

void cataloguePeriodPheno::syntheseTempoRadiation(std::vector<tuileS2OneDatePheno *> aVTuileS2, std::string aOutSuffix){

    int c(0);
    int step=mY/20;
    int count(0);
    double gain(0.5/255.0);// j'estime que le max de radiation moyenne est 0.5

    std::vector<std::string> vBR1{"2", "3", "4"};

    float * scanLineOut1=(float *) CPLMalloc( sizeof( float  ) * mY );
    float * scanLineOut2=(float *) CPLMalloc( sizeof( float  ) * mY );
    float * scanLineOut3=(float *) CPLMalloc( sizeof( float  ) * mY );

    mMapResults.clear();
    const char *pszFormat = "GTiff";
    GDALDriver * pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    if( pDriver != NULL )
    {
        for (std::string b : vBR1){
            // MEM raster
            std::string output=getNameBandPeriodPheno(aOutSuffix,b);

            const char *out=output.c_str();
            GDALDataset  * ds = pDriver->CreateCopy(out,mDSmaskR1,FALSE, NULL,NULL, NULL );
            mMapResults.emplace(std::make_pair(b,ds));
        }
    }
    std::cout << " bandes résolution 1 " << std::endl;
    for ( int row = 0; row < mY; row++ ){
        // lecture masque global dans scanLine
        readMasqLine(row);
        // lecture, pour chaque tuile, de la ligne pour le masque et des lignes pour les bandes R1
        //donc 4 scanlignes par tuiles
        for (tuileS2OneDatePheno * t : aVTuileS2){
            t->readLines(1,row);
        }

        for (int col=0 ; col<mX;col++){
            if (scanLineR1[col]==1){
                double b2(0),b3(0),b4(0),n(0);//,b8(0),b11(0),b12(0);
                for (tuileS2OneDatePheno * t : aVTuileS2){
                    if (t->scanLineMasq[col]==1){
                        b2+=t->scanLine1[col];
                        b3+=t->scanLine2[col];
                        b4+=t->scanLine3[col];
                        n++;
                    }
                }
                if (n>0){
                    // calculer la moyenne ; quel type de donnée? c'est du float mais je peux le code sur 8bit. ça vaut la peine.
                    // pas de 0.33 %
                    b2=b2/(10000.0*n);
                    b3=b3/(10000.0*n);
                    b4=b4/(10000.0*n);
                    // sauver les résultats
                    scanLineOut1[col]=b2/gain;
                    scanLineOut2[col]=b3/gain;
                    scanLineOut3[col]=b4/gain;
                    //std::cout << "b2 = " << b2*50 << std::endl;
                }
            } else {
                scanLineOut1[col]=255;
                scanLineOut2[col]=255;
                scanLineOut3[col]=255;
            }
        }

        // sauver données dans raster résultats
        mMapResults.at("2")->GetRasterBand(1)->RasterIO( GF_Write, 0, row,mX,1,scanLineOut1, mX, 1,GDT_Float32, 0, 0 );
        mMapResults.at("3")->GetRasterBand(1)->RasterIO( GF_Write, 0, row,mX,1,scanLineOut2,mX, 1,GDT_Float32, 0, 0 );
        mMapResults.at("4")->GetRasterBand(1)->RasterIO( GF_Write, 0, row,mX,1,scanLineOut3, mX, 1,GDT_Float32, 0, 0 );
        c++;
        if (c%step==0){
            count++;
            std::cout << count*5 << "%..." << std::endl;
        }
    }
    CPLFree(scanLineOut1);
    CPLFree(scanLineOut2);
    CPLFree(scanLineOut3);
    GDALClose(mMapResults.at("2"));
    GDALClose(mMapResults.at("3"));
    GDALClose(mMapResults.at("4"));


    // bandes R2 maintenant ---------------------------------------------------------
    int YR2=mY/2,XR2=mX/2;
    scanLineOut1=(float *) CPLMalloc( sizeof( float  ) * YR2 );
    scanLineOut2=(float *) CPLMalloc( sizeof( float  ) * YR2 );
    scanLineOut3=(float *) CPLMalloc( sizeof( float  ) * YR2 );
    c=0;count=0;step=YR2/20;

    for (tuileS2OneDatePheno * t : aVTuileS2){
        //initialize les objects scanline et lecture masque global R2
        t->initLines(2);

    }

    mMapResults.clear();

    for (std::string b : vBR2){
        // MEM raster
        std::string output=getNameBandPeriodPheno(aOutSuffix,b);
        const char *out=output.c_str();
        GDALDataset  * ds = pDriver->CreateCopy(out,mDSmaskR2,FALSE, NULL,NULL, NULL );
        ds->GetRasterBand(1)->SetNoDataValue(255);
        mMapResults.emplace(std::make_pair(b,ds));
    }
    std::cout << " bandes résolution 2 " << std::endl;

    for ( int row = 0; row < YR2; row++ ){
        // lecture masque global scanLine
        readMasqLine(row,2);
        // lecture, pour chaque tuile, de la ligne pour le masque et des lignes pour les bandes R1
        //donc 4 scanlignes par tuiles
        for (tuileS2OneDatePheno * t : aVTuileS2){
            t->readLines(2,row);
        }

        for (int col=0 ; col<XR2;col++){
            if (scanLineR2[col]==1){
                double b8(0),b11(0),b12(0),n(0);
                for (tuileS2OneDatePheno * t : aVTuileS2){
                    if (t->scanLineMasq[col]==1){
                        b8+=t->scanLine1[col];
                        b11+=t->scanLine2[col];
                        b12+=t->scanLine3[col];
                        n++;
                    }
                }
                if (n>0){
                    // calculer la moyenne ; quel type de donnée? c'est du float mais je peux le code sur 8bit. ça vaut la peine.
                    // pas de 0.33 %
                    b8=b8/(10000.0*n);
                    b11=b11/(10000.0*n);
                    b12=b12/(10000.0*n);
                    // sauver les résultats
                    scanLineOut1[col]=b8/gain;
                    scanLineOut2[col]=b11/gain;
                    scanLineOut3[col]=b12/gain;
                }
            } else {
                scanLineOut1[col]=255;
                scanLineOut2[col]=255;
                scanLineOut3[col]=255;
            }
        }

        // sauver données dans raster résultats
        mMapResults.at("8A")->GetRasterBand(1)->RasterIO( GF_Write, 0, row,XR2,1,scanLineOut1, XR2, 1,GDT_Float32, 0, 0 );
        mMapResults.at("11")->GetRasterBand(1)->RasterIO( GF_Write, 0, row,XR2,1,scanLineOut2,XR2, 1,GDT_Float32, 0, 0 );
        mMapResults.at("12")->GetRasterBand(1)->RasterIO( GF_Write, 0, row,XR2,1,scanLineOut3, XR2, 1,GDT_Float32, 0, 0 );
        c++;
        if (c%step==0){
            count++;
            std::cout << count*5 << "%..." << std::endl;
        }
    }
    CPLFree(scanLineOut1);
    CPLFree(scanLineOut2);
    CPLFree(scanLineOut3);
    GDALClose(mMapResults.at("8A"));
    GDALClose(mMapResults.at("11"));
    GDALClose(mMapResults.at("12"));
}


void cataloguePeriodPheno::closeDS(){
    std::cout << "fermeture de tout les raster de la TS" << std::endl;
    for (tuileS2OneDatePheno * t : mVProdutsOK){
        t->closeDS();
    }
    if( mDSmaskR1 != NULL){ GDALClose( mDSmaskR1 );}
    if( mDSmaskR2 != NULL){ GDALClose( mDSmaskR2 );}
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
    CPLFree(scanLineR1);
    CPLFree(scanLineR2);
}


