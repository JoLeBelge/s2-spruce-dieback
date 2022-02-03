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

extern std::vector<std::string> vBR2; // variable partagée avec projet scolyte, donc redéfinir ici les valeurs dans le code
std::vector<std::string> vBR1{"2","3","4","8"};

std::string aheader="compo b2_1 b3_1 b4_1 b8_1 b11_1 b12_1 b2_2 b3_2 b4_2 b8_2 b11_2 b12_2 b2_3 b3_3 b4_3 b8_3 b11_3 b12_3 b2_4 b3_4 b4_4 b8_4 b11_4 b12_4";

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
    std::string out=getNameMasque(2);

    if (boost::filesystem::exists(masque)){
        if (!boost::filesystem::exists(out) | overw){
            std::cout << " create Mask For tuile " << std::endl;
            if (mVProduts.size()>0){
                // un effet indésirable à éviter c'est que les masques R1 et R2 n 'ai pas exactement la même emprise, du à cette résolution différente justement.
                // c'est pour éviter cela que je fait d'abors le masque R2 puis le R1 (du moins restrictif vers le plus restrictif)
                // pour la carte scolyte c'est pas fait, mais fonctionne pas pareil donc pas grave
                std::string aCommand="gdalwarp -te "+std::to_string(mVProduts.at(0)->mXmin)+" "+std::to_string(mVProduts.at(0)->mYmin)+" "+std::to_string(mVProduts.at(0)->mXmax)+" "+std::to_string(mVProduts.at(0)->mYmax)+ " -t_srs EPSG:"+std::to_string(epsg)+" -ot Byte -r max -overwrite -tr 20 20 "+ masque+ " "+ out;
                std::cout << aCommand << std::endl;
                system(aCommand.c_str());
                aCommand="gdalwarp -ot Byte -overwrite -tr 10 10 "+out+ " "+ getNameMasque();
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

    float * scanLineOut1=(float *) CPLMalloc( sizeof( float  ) * mY );
    float * scanLineOut2=(float *) CPLMalloc( sizeof( float  ) * mY );
    float * scanLineOut3=(float *) CPLMalloc( sizeof( float  ) * mY );
    float * scanLineOut4=(float *) CPLMalloc( sizeof( float  ) * mY );

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
    std::cout << " bandes résolution 1 : Bandes 2, 3, 4 et 8" << std::endl;
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
                double b2(0),b3(0),b4(0),b8(0),n(0);//,b8(0),b11(0),b12(0);
                for (tuileS2OneDatePheno * t : aVTuileS2){
                    if (t->scanLineMasq[col]==1){
                        b2+=t->scanLine1[col];
                        b3+=t->scanLine2[col];
                        b4+=t->scanLine3[col];
                        b8+=t->scanLine4[col];
                        n++;
                    }
                }
                if (n>0){
                    // calculer la moyenne ; quel type de donnée? c'est du float mais je peux le code sur 8bit. ça vaut la peine.
                    // pas de 0.33 %
                    b2=b2/(10000.0*n);
                    b3=b3/(10000.0*n);
                    b4=b4/(10000.0*n);
                    b8=b8/(10000.0*n);
                    // sauver les résultats
                    scanLineOut1[col]=b2/gain;
                    scanLineOut2[col]=b3/gain;
                    scanLineOut3[col]=b4/gain;
                    scanLineOut4[col]=b8/gain;
                    //std::cout << "b2 = " << b2*50 << std::endl;
                }
            } else {
                scanLineOut1[col]=255;
                scanLineOut2[col]=255;
                scanLineOut3[col]=255;
                scanLineOut4[col]=255;
            }
        }

        // sauver données dans raster résultats
        mMapResults.at("2")->GetRasterBand(1)->RasterIO( GF_Write, 0, row,mX,1,scanLineOut1, mX, 1,GDT_Float32, 0, 0 );
        mMapResults.at("3")->GetRasterBand(1)->RasterIO( GF_Write, 0, row,mX,1,scanLineOut2,mX, 1,GDT_Float32, 0, 0 );
        mMapResults.at("4")->GetRasterBand(1)->RasterIO( GF_Write, 0, row,mX,1,scanLineOut3, mX, 1,GDT_Float32, 0, 0 );
        mMapResults.at("8")->GetRasterBand(1)->RasterIO( GF_Write, 0, row,mX,1,scanLineOut4, mX, 1,GDT_Float32, 0, 0 );
        c++;
        if (c%step==0){
            count++;
            std::cout << count*5 << "%..." << std::endl;
        }
    }
    CPLFree(scanLineOut1);
    CPLFree(scanLineOut2);
    CPLFree(scanLineOut3);
    CPLFree(scanLineOut4);
    // on ferme toutes les couche raster de résultats
    for (auto kv: mMapResults){
        GDALClose(kv.second);
    }
    mMapResults.clear();

    // bandes R2 maintenant ---------------------------------------------------------
    int YR2=mY/2,XR2=mX/2;
    scanLineOut1=(float *) CPLMalloc( sizeof( float  ) * YR2 );
    scanLineOut2=(float *) CPLMalloc( sizeof( float  ) * YR2 );
    scanLineOut3=(float *) CPLMalloc( sizeof( float  ) * YR2 );
    scanLineOut4=(float *) CPLMalloc( sizeof( float  ) * YR2 );
    float * scanLineOut5=(float *) CPLMalloc( sizeof( float  ) * YR2 );
    float * scanLineOut6=(float *) CPLMalloc( sizeof( float  ) * YR2 );
    c=0;count=0;step=YR2/20;

    for (tuileS2OneDatePheno * t : aVTuileS2){
        //initialize les objects scanline et lecture masque global R2
        t->initLines(2);
    }

    for (std::string b : vBR2){
        std::string output=getNameBandPeriodPheno(aOutSuffix,b);
        const char *out=output.c_str();
        GDALDataset  * ds = pDriver->CreateCopy(out,mDSmaskR2,FALSE, NULL,NULL, NULL );
        ds->GetRasterBand(1)->SetNoDataValue(255);
        mMapResults.emplace(std::make_pair(b,ds));
    }
    std::cout << " bandes résolution R2 : 5, 6, 7, 8A, 11 et 12 " << std::endl;

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
                double b8(0),b11(0),b12(0),b5(0),b6(0),b7(0),n(0);
                for (tuileS2OneDatePheno * t : aVTuileS2){
                    if (t->scanLineMasq[col]==1){
                        b8+=t->scanLine1[col];
                        b11+=t->scanLine2[col];
                        b12+=t->scanLine3[col];
                        b5+=t->scanLine4[col];
                        b6+=t->scanLine5[col];
                        b7+=t->scanLine6[col];
                        n++;
                    }
                }
                if (n>0){
                    // calculer la moyenne ; quel type de donnée? c'est du float mais je peux le code sur 8bit. ça vaut la peine.
                    // pas de 0.33 %
                    b8=b8/(10000.0*n);
                    b11=b11/(10000.0*n);
                    b12=b12/(10000.0*n);
                    b5=b5/(10000.0*n);
                    b6=b6/(10000.0*n);
                    b7=b7/(10000.0*n);
                    // sauver les résultats
                    scanLineOut1[col]=b8/gain;
                    scanLineOut2[col]=b11/gain;
                    scanLineOut3[col]=b12/gain;
                    scanLineOut4[col]=b5/gain;
                    scanLineOut5[col]=b6/gain;
                    scanLineOut6[col]=b7/gain;
                }
            } else {
                scanLineOut1[col]=255;
                scanLineOut2[col]=255;
                scanLineOut3[col]=255;
                scanLineOut4[col]=255;
                scanLineOut5[col]=255;
                scanLineOut6[col]=255;
            }
        }

        // sauver données dans raster résultats
        mMapResults.at("8A")->GetRasterBand(1)->RasterIO( GF_Write, 0, row,XR2,1,scanLineOut1, XR2, 1,GDT_Float32, 0, 0 );
        mMapResults.at("11")->GetRasterBand(1)->RasterIO( GF_Write, 0, row,XR2,1,scanLineOut2,XR2, 1,GDT_Float32, 0, 0 );
        mMapResults.at("12")->GetRasterBand(1)->RasterIO( GF_Write, 0, row,XR2,1,scanLineOut3, XR2, 1,GDT_Float32, 0, 0 );
        mMapResults.at("5")->GetRasterBand(1)->RasterIO( GF_Write, 0, row,XR2,1,scanLineOut4, XR2, 1,GDT_Float32, 0, 0 );
        mMapResults.at("6")->GetRasterBand(1)->RasterIO( GF_Write, 0, row,XR2,1,scanLineOut5,XR2, 1,GDT_Float32, 0, 0 );
        mMapResults.at("7")->GetRasterBand(1)->RasterIO( GF_Write, 0, row,XR2,1,scanLineOut6, XR2, 1,GDT_Float32, 0, 0 );
        c++;
        if (c%step==0){
            count++;
            std::cout << count*5 << "%..." << std::endl;
        }
    }
    CPLFree(scanLineOut1);
    CPLFree(scanLineOut2);
    CPLFree(scanLineOut3);

    // on ferme toutes les couche raster de résultats
    for (auto kv: mMapResults){
        GDALClose(kv.second);
    }
    mMapResults.clear();

}


void cataloguePeriodPheno::closeDS(){
    std::cout << "fermeture de tout les raster de la TS" << std::endl;
    for (tuileS2OneDatePheno * t : mVProdutsOK){
        t->closeDS();
    }
    if( mDSmaskR1 != NULL){ GDALClose( mDSmaskR1 );}
    if( mDSmaskR2 != NULL){ GDALClose( mDSmaskR2 );}
    CPLFree(scanPix);

    CPLFree(scanLineR1);
    CPLFree(scanLineR2);
}


void cataloguePeriodPheno::getMeanRadByTriMultiPt(std::vector<mpt*> aVPt,std::string aOut){

    if (openDS4RF()){

        // lecture input
        std::vector<std::unique_ptr<periodePhenoRasters>> aTS;
        for (int tri(1); tri<5;tri++){
            std::vector<std::string> aV;
            for (std::string b : vBR1){
                aV.push_back(getNameBandPeriodPheno("tri"+std::to_string(tri),b));
            }
            for (std::string b : vBR2){
                aV.push_back(getNameBandPeriodPheno("tri"+std::to_string(tri),b));
            }
            aTS.push_back(std::make_unique<periodePhenoRasters>(aV));
        }

        // crée un fichier txt pour l'export des résultats
        std::ofstream out;
        out.open(aOut);
        std::vector<std::string> vB{"b2","b3","b4","b8","b11","b12"};
        if (mDebug) {out << "compo;X;Y";} else {out << "compo";}
        // créer les headers des bandes
        for (int tri(1);tri<5;tri++){
            for (std::string b : vB){
                out << " " << b<< "_" <<std::to_string(tri) ;
            }
        }
        out <<"\n" ;
        int c(0),count(0);
        OGRSpatialReference oSourceSRS, oTargetSRS;
        oSourceSRS.importFromEPSG(31370);
        oTargetSRS.importFromEPSG(32631);
        OGRCoordinateTransformation * poCT = OGRCreateCoordinateTransformation( &oSourceSRS, &oTargetSRS );

        for (mpt * p : aVPt){
            //if (c==10){break;}
            p->transform(poCT);

            Pt2di pt=getUV(p->getX(),p->getY());

            if (pt.x>0 && pt.y>0){
                if (mDebug) {
                    out << p->Code() << ";"  << roundDouble(p->getX(),0) << ";" << roundDouble(p->getY(),0) ;
                } else if (isInMasq(pt)){
                    // check que le point a des valeurs spectrale qui sont ok. une incohérence entre le masque utilisé pour générer les bande trimetrielles et les carte de prob fait qu'il existe quelques points qui sont en dehors du masque, valeur spectrale 255
                    out << p->Code()  ;
                }

                if (mDebug | isInMasq(pt)){
                    for (std::unique_ptr<periodePhenoRasters> & t : aTS){
                        std::vector<int> vVal=t->getVal(pt);
                        for (int v : vVal){
                            //vMetrics.push_back(v);
                            out << " "  << v<<".00" ;
                        }
                    }
                    out <<"\n" ;
                } else {
                    std::cout << " le point n'est pas dans le masque, je le vire" << std::endl;
                }
            }

            c++;
            if (c%100==0){
                count++;
                std::cout << count << "% ..." << std::endl;
            }
        }
        out.close();
        closeDS();
    }

}

void cataloguePeriodPheno::applyRF(std::string pathRFmodel){

    // lecture des couches inputs
    if (openDS4RF()){
        // création output
        const char *pszFormat = "GTiff";
        GDALDriver * pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        //GDALDataset  * ds = pDriver->CreateCopy(getNameOutputRF().c_str(),mDSmaskR1,FALSE, NULL,NULL, NULL );
        GDALDataset  * ds = pDriver->CreateCopy(getNameOutputRF().c_str(),mDSmaskR2,FALSE, NULL,NULL, NULL );
        ds->GetRasterBand(1)->SetNoDataValue(255);

        // lecture input
        std::vector<std::unique_ptr<periodePhenoRasters>> aTS;
        for (int tri(1); tri<5;tri++){
            std::vector<std::string> aV;
            for (std::string b : vBR1){
                aV.push_back(getNameBandPeriodPheno("tri"+std::to_string(tri),b));
            }
            for (std::string b : vBR2){
                aV.push_back(getNameBandPeriodPheno("tri"+std::to_string(tri),b));
            }
            aTS.push_back(std::make_unique<periodePhenoRasters>(aV));
        }
        int YR2=mY/2;
        int XR2=mX/2;

        int c(0);
        int step=YR2/100;
        int count(0);
        //double gain(0.5/255.0);
        initRF(pathRFmodel);

        for ( int row = 0; row < YR2; row++ ){
            // lecture masque global dans scanLine
            readMasqLine(row,2);
            double compo(255);
            for (int col=0 ; col<XR2;col++){
                if (scanLineR2[col]==1){

                    Pt2di pt(col,row);
                    // lecture des 4 trimestres
                    std::vector<double> vMetrics;
                    for (std::unique_ptr<periodePhenoRasters> & t : aTS){

                        std::vector<int> vVal=t->getVal(pt,2);
                        for (int v : vVal){
                            //vMetrics.push_back((double) v*gain);
                            vMetrics.push_back(v);
                        }
                    }
                    // call RF
                    compo=((double) callRF(vMetrics));

                } else {
                    compo=255;
                }
                scanPix[0]=compo;
                ds->GetRasterBand(1)->RasterIO( GF_Write, col, row,1,1,scanPix,1, 1,GDT_Float32, 0, 0 );

            }

            c++;
            if (c%step==0){
                count++;
                std::cout << count << "%..." << std::endl;
            }
        }
        GDALClose(ds);

        closeDS();
    }
}

int cataloguePeriodPheno::callRF(std::vector<double> aV){
    if (mDebug) {std::cout << "call RF " << std::endl;}
    /*std::cout << " valeurs utilisée par RF" << std::endl;
    for (double v : aV){
    std::cout << roundDouble(v,3) << " " << std::endl;
    }*/
    forest->setDataForPrediction(aV,aheader);
    forest->myrun();
    return forest->getClassMaj();
}

void cataloguePeriodPheno::initRF(std::string pathRFmodel){

    if (mDebug){std::cout << " init RF with file :" << pathRFmodel <<"." <<std::endl;}
    forest=std::make_unique<ranger::ForestClassification>() ;
    std::vector<std::string> bidon1;
    std::vector<double> vMetrics{0.0002,0.026,0.034,0.150,0.00131,0.074,0.018,0.037,0.026,0.278,0.000260,0.077,0.012,0.25,0.011,0.00358,0.139,0.50,0.014,0.025,0.025,0.416,0.122,0.015};
    std::ostream nullstream(0);
    forest->initCpp(
                std::string(""),        //arg_handler.depvarname
                MEM_DOUBLE,//arg_handler.memmode
                std::string(""),// arg_handler.file
                5,// arg_handler.mtry,
                "rf",//arg_handler.outprefix
                500, // arg_handler.ntree
                //&std::cout,// &verbose_out
                &nullstream,// &verbose_out
                0,// arg_handler.seed
                //9,//9 sur 12 que j'ai sur le pc s2-jo
                DEFAULT_NUM_THREADS, //arg_handler.nthreads,
                pathRFmodel,//arg_handler.predict
                DEFAULT_IMPORTANCE_MODE,//arg_handler.impmeasure
                0,// arg_handler.targetpartitionsize
                std::string(""),//arg_handler.splitweights,
                bidon1,//arg_handler.alwayssplitvars
                std::string(""),//arg_handler.statusvarname,
                true,//arg_handler.replace
                bidon1,// arg_handler.catvars
                false,// arg_handler.savemem
                DEFAULT_SPLITRULE,// arg_handler.splitrule
                std::string(""),// arg_handler.caseweights
                true,// arg_handler.predall
                0,// arg_handler.fraction
                DEFAULT_ALPHA,//arg_handler.alpha
                DEFAULT_MINPROP,// arg_handler.minprop
                false,// arg_handler.holdout
                DEFAULT_PREDICTIONTYPE, //arg_handler.predictiontype,
                DEFAULT_NUM_RANDOM_SPLITS,//    arg_handler.randomsplits
                DEFAULT_MAXDEPTH, //arg_handler.maxdepth
                vMetrics,//arg_handler.mVCodeEsp);
                aheader
                );
}


bool cataloguePeriodPheno::openDS4RF(){
    std::cout << "ouverture des bandes moyennes par trimestre" << std::endl;
    createMaskForTuile();
    bool aRes(1);

    for (int tri(1); tri<5;tri++){
        for (std::string b : vBR2){
            if (!exists(getNameBandPeriodPheno("tri"+std::to_string(tri),b))){ aRes=0; std::cout << " fichier " << getNameBandPeriodPheno("tri"+std::to_string(tri),b) << std::endl;break;}
        }
        for (std::string b : vBR1){
            if (!exists(getNameBandPeriodPheno("tri"+std::to_string(tri),b))){ aRes=0; std::cout << " fichier " << getNameBandPeriodPheno("tri"+std::to_string(tri),b) << std::endl;break;}
        }
    }

    if (aRes && exists(getNameMasque())){
        mDSmaskR1=(GDALDataset *) GDALOpen( getNameMasque().c_str(), GA_ReadOnly );
        mDSmaskR2=(GDALDataset *) GDALOpen( getNameMasque(2).c_str(), GA_ReadOnly );
        mX=mDSmaskR1->GetRasterBand(1)->GetXSize();
        mY=mDSmaskR1->GetRasterBand(1)->GetYSize();
        scanLineR1=(float *) CPLMalloc( sizeof( float ) * mY );
        scanLineR2=(float *) CPLMalloc( sizeof( float ) * mY/2 );
        scanPix=(float *) CPLMalloc( sizeof( float ) * 1 );
    }

    return aRes;
}



periodePhenoRasters::periodePhenoRasters(std::vector<std::string> aVBandsPath){

    //ouverture des bandes
    std::cout << " charge en mémoire toutes les bandes d'une période phéno" << std::endl;
    if (aVBandsPath.size()==10){
    b2= std::make_unique<Im2D_U_INT1>(Im2D_U_INT1::FromFileStd(aVBandsPath.at(0)));
    b3= std::make_unique<Im2D_U_INT1>(Im2D_U_INT1::FromFileStd(aVBandsPath.at(1)));
    b4= std::make_unique<Im2D_U_INT1>(Im2D_U_INT1::FromFileStd(aVBandsPath.at(2)));
    b8= std::make_unique<Im2D_U_INT1>(Im2D_U_INT1::FromFileStd(aVBandsPath.at(3)));
    b5= std::make_unique<Im2D_U_INT1>(Im2D_U_INT1::FromFileStd(aVBandsPath.at(4)));
    b6= std::make_unique<Im2D_U_INT1>(Im2D_U_INT1::FromFileStd(aVBandsPath.at(5)));
    b7= std::make_unique<Im2D_U_INT1>(Im2D_U_INT1::FromFileStd(aVBandsPath.at(6)));
    b8A= std::make_unique<Im2D_U_INT1>(Im2D_U_INT1::FromFileStd(aVBandsPath.at(7)));
    b11= std::make_unique<Im2D_U_INT1>(Im2D_U_INT1::FromFileStd(aVBandsPath.at(8)));
    b12= std::make_unique<Im2D_U_INT1>(Im2D_U_INT1::FromFileStd(aVBandsPath.at(9)));
    } else { std::cout << "il me manque des bandes (10 en tout) pour créer la préiode phéno" << std::endl;}

    // le prob c'est la gestion des no data, je sais plus comment faire dans micmac
    /*
    b2R2 =  std::make_unique<Im2D_U_INT1>(b8A->sz().x,b8A->sz().y,0);
    b3R2 =  std::make_unique<Im2D_U_INT1>(b8A->sz().x,b8A->sz().y,0);
    b4R2 =  std::make_unique<Im2D_U_INT1>(b8A->sz().x,b8A->sz().y,0);
     Fonc_Num resample(Fonc_Num aIn) = StdFoncChScale(aIn,Pt2dr(0,0),Pt2dr(2,2));
     {
     }*/
    resampleR1toR2(aVBandsPath.at(0));
    resampleR1toR2(aVBandsPath.at(1));
    resampleR1toR2(aVBandsPath.at(2));
    resampleR1toR2(aVBandsPath.at(3));

    b2R2= std::make_unique<Im2D_U_INT1>(Im2D_U_INT1::FromFileStd(getR2Name(aVBandsPath.at(0))));
    b3R2= std::make_unique<Im2D_U_INT1>(Im2D_U_INT1::FromFileStd(getR2Name(aVBandsPath.at(1))));
    b4R2= std::make_unique<Im2D_U_INT1>(Im2D_U_INT1::FromFileStd(getR2Name(aVBandsPath.at(2))));
    b8R2= std::make_unique<Im2D_U_INT1>(Im2D_U_INT1::FromFileStd(getR2Name(aVBandsPath.at(3))));
}

void periodePhenoRasters::resampleR1toR2(std::string aR1){
    std::string aR2=getR2Name(aR1);
    if (!exists(aR2)){
        std::string aCommand="gdalwarp -tr 20 20 -r average -overwrite -srcnodata 255 -co 'COMPRESS=NONE' "+ aR1+ " " + aR2;
        system(aCommand.c_str());
    }
}

std::string periodePhenoRasters::getR2Name(std::string aR1){
    return aR1.substr(0,aR1.size()-4)+"R2.tif";
}

std::vector<int> periodePhenoRasters::getVal(const Pt2di & pt,int resol){
    std::vector<int> aRes;
    //std::cout << " pt " << pt << std::endl;
    switch (resol) {
    case 1:{
        aRes.push_back(b2->GetI(pt));
        aRes.push_back(b3->GetI(pt));
        aRes.push_back(b4->GetI(pt));
        aRes.push_back(b8->GetI(pt));
        Pt2di ptR2(pt.x/2, pt.y/2);
        aRes.push_back(b5->GetI(ptR2));
        aRes.push_back(b6->GetI(ptR2));
        aRes.push_back(b7->GetI(ptR2));
        aRes.push_back(b8A->GetI(ptR2));
        aRes.push_back(b11->GetI(ptR2));
        aRes.push_back(b12->GetI(ptR2));
        break;}
    case 2:{
        // je travaille à la résolution R2
        aRes.push_back(b2R2->GetI(pt));
        aRes.push_back(b3R2->GetI(pt));
        aRes.push_back(b4R2->GetI(pt));
        aRes.push_back(b8R2->GetI(pt));

        aRes.push_back(b5->GetI(pt));
        aRes.push_back(b6->GetI(pt));
        aRes.push_back(b7->GetI(pt));
        aRes.push_back(b8A->GetI(pt));
        aRes.push_back(b11->GetI(pt));
        aRes.push_back(b12->GetI(pt));
        break;}
    default:
        break;
    }

    return aRes;
}

Pt2di cataloguePeriodPheno::getUV(double x, double y){
    double transform[6];
    mDSmaskR1->GetGeoTransform(transform);
    double xOrigin = transform[0];
    double yOrigin = transform[3];
    double pixelWidth = transform[1];
    double pixelHeight = -transform[5];
    int col = int((x - xOrigin) / pixelWidth);
    int row = int((yOrigin - y ) / pixelHeight);
    if (col<mX && row<mY && col>-1 && row >-1){return Pt2di(col,row);} else {

        std::cout << "col =" << col << ", row " << row << std::endl;
        return Pt2di(0,0);
    }

}
