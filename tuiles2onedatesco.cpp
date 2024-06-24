#include "tuiles2onedatesco.h"

int tuileS2OneDateSco::getCodeLine(int aCol) const{
    return scanLineCode[aCol];
}


void tuileS2OneDateSco::computeCR(){
    if (mDebug){std::cout << "computeCR .." << std::endl;}

    std::string out=getRasterCRName();
    std::string NIRa=getRasterR2Name("8A");
    std::string SWIR1=getRasterR2Name("11");
    std::string SWIR2=getRasterR2Name("12");
    // check que le fichier n'existe pas
    if (boost::filesystem::exists(NIRa) && boost::filesystem::exists(SWIR1) && boost::filesystem::exists(SWIR2)){
        if ((!boost::filesystem::exists(out) | overw) ){
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
         *
         */

            // si je tente de mettre sur 8 bit ; il me dis "error complex number". mais en double ça passe
            // j'ai des overflow mais pas beauoup. Le range de valeur attendu, c'est entre 0 et 2 (voir graph de Raphael) mais j'ai des valeurs qui dépassent 2.
            //std::string exp("im2b1!=0 ? im2b1/(im1b1+(1610-865)* ((im3b1-im1b1)/(2190-865))) : 0");
            std::string exp("im2b1/(im1b1+(1610-865)* ((im3b1-im1b1)/(2190-865)))");
            std::string aCommand(path_otb+"otbcli_BandMathX -il "+NIRa+" "+SWIR1+" "+SWIR2+" -out '"+ out + compr_otb+"' double -exp '"+exp+"' -ram 5000 -progress 0");
            std::cout << aCommand << std::endl;
            system(aCommand.c_str());
        }
    } else { std::cout << "fichiers introuvables " << NIRa << " , " << SWIR1 << " , " << SWIR2 << std::endl;}

    r_crswir = std::make_unique<rasterFiles>(out);
}

//crée une couche qui normalise le CR par le CR sensé être ok pour cette date ; sera plus facile à manipuler
void tuileS2OneDateSco::normaliseCR(){
    if (mDebug){std::cout << "normalize CR " ;}

    std::string out=getRasterCRnormName();
    std::string in=getRasterCRName();
    // check que le fichier n'existe pas
    if (boost::filesystem::exists(in)){
        if (!boost::filesystem::exists(out) | overw){

            // CRSWIRnorm = CRSWIR/theoreticalCR(t)


            double cr = getCRth();

            // gain de 1/127, comme cela je stoque des valeurs de 0 à 2 sur du 8 bits
            std::string exp("im1b1>0 ? 127*im1b1/"+std::to_string(cr)+" : 0");
            std::string aCommand(path_otb+"otbcli_BandMathX -il "+in+" -out '"+ out + compr_otb+"' uint8 -exp '"+exp+"' -ram 5000 -progress 0");
            std::cout << aCommand << std::endl;
            system(aCommand.c_str());
        }
    } else { std::cout << "fichier introuvable " << in << std::endl;}
    r_crswirNorm = std::make_unique<rasterFiles>(out);

}

std::string getNameMasqueEP(int i){
    return wd+"input/masque_EP"+globSuffix+"_"+globTuile+"_R"+std::to_string(i)+".tif";
}

void tuileS2OneDateSco::computeCodeLine(){
    for (int col(0); col< mYSize ; col++){
        int solnu = getMasqVal(col);
        if (solnu==0){scanLineCode[col]=0;
        }else if(solnu==2 | solnu==3){
            scanLineCode[col]=3;
            // ci-dessous donc pour solnu=1 (zone oK)
        } else {
            double crnorm=getCRnormVal(col);
            if (crnorm<=seuilCR) {
                scanLineCode[col]=1;
            } else  {
                scanLineCode[col]=2;
            }
        }
    }
}


void tuileS2OneDateSco::readMasqLine(int aRow) const{
    if( mDSsolnu != NULL && mDSsolnu->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){
        mDSsolnu->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, mXSize, 1, scanLineSolNu, mXSize,1, GDT_Float32, 0, 0 );
    }else {
        std::cout << "readMasqLine ; failed for " << getRasterMasqSecName() <<  std::endl;
    }
}
double tuileS2OneDateSco::getCRnormVal(int aCol) const{
    // applique le gain
    return scanLineCR[aCol]*1.0/127.0;
}
int tuileS2OneDateSco::getMasqVal(int aCol) const{
    return scanLineSolNu[aCol];
}


void tuileS2OneDateSco::readCRnormLine(int aRow) const{

    if( mDScrnom != NULL && mDScrnom->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){
        mDScrnom->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, mXSize, 1, scanLineCR, mXSize,1, GDT_Float32, 0, 0 );
    } else {
        std::cout << "readCRnormLine ; failed for " << getRasterCRnormName() <<  std::endl;
    }
}

bool tuileS2OneDateSco::openDS(){

    //std::cout << "ouverture dataset pour " << getRasterCRnormName() << std::endl;
    bool aRes(0);
    if (exists(getRasterCRnormName()) && exists(getRasterMasqSecName())){
        mDScrnom= (GDALDataset *) GDALOpen( getRasterCRnormName().c_str(), GA_ReadOnly );
        mDSsolnu= (GDALDataset *) GDALOpen( getRasterMasqSecName().c_str(), GA_ReadOnly );

        mDSetatInit= (GDALDataset *) GDALOpen( getRasterEtatIName().c_str(), GA_Update );
        mDSetatFinal= (GDALDataset *) GDALOpen( getRasterEtatFName().c_str(), GA_Update );

        // tout ça c'est uniquement pour s2_carteEss
        // fonctionne uniquement si j'augmente le nombre de fichiers descriptors, limitation du systèmes
        //https://askubuntu.com/questions/1049058/how-to-increase-max-open-files-limit-on-ubuntu-18-04
        std::vector<std::string> b1R{"2","3","4"};
        std::vector<std::string> b2R{"8A","11","12"};
        for (std::string b : b1R){
            GDALDataset * DSpt= (GDALDataset *) GDALOpen( getRasterR1Name(b).c_str(), GA_ReadOnly );
            vDS.emplace(std::make_pair(b,DSpt));
        }
        for (std::string b : b2R){
            GDALDataset * DSpt= (GDALDataset *) GDALOpen( getOriginalRasterR2Name(b).c_str(), GA_ReadOnly );
            vDS.emplace(std::make_pair(b,DSpt));
        }
        //------

        if( mDSsolnu == NULL |  mDScrnom == NULL)
        {
            std::cout << "dataset pas ouvert correctement pour tuile " << mProd << std::endl;
            closeDS();
        } else {
            aRes=1;
            scanPix=(float *) CPLMalloc( sizeof( float ) * 1 );
            scanLineSolNu=(float *) CPLMalloc( sizeof( float ) * mYSize );
            scanLineCR=(float *) CPLMalloc( sizeof( float ) * mYSize );
            scanLineCode=(int *) CPLMalloc( sizeof( int ) * mYSize );
        }
    } else {
        std::cout << "ne trouve pas " << getRasterCRnormName() << " \n" << " ou bien " << getRasterMasqSecName() << std::endl;
    }
    return aRes;
}
void tuileS2OneDateSco::closeDS(){
    if (mDScrnom != NULL){GDALClose( mDScrnom );}
    if (mDSsolnu != NULL){GDALClose( mDSsolnu );}
    if (scanPix!=NULL){ CPLFree(scanPix);}
    if (scanLineSolNu!=NULL){ CPLFree(scanLineSolNu);}
    if (scanLineCR!=NULL){ CPLFree(scanLineCR);}
    if (scanLineCode!=NULL){ CPLFree(scanLineCode);}
    if (mDSetatInit != NULL){GDALClose( mDSetatInit );}
    if (mDSetatFinal != NULL){GDALClose( mDSetatFinal );}
    for (auto kv : vDS){
        GDALDataset * DSpt=kv.second;
        if (DSpt != NULL){GDALClose( DSpt );}
    }
}


int tuileS2OneDateSco::getMasqVal(int aCol,int aRow){
    int aRes=0;
    if( mDSsolnu != NULL && mDSsolnu->GetRasterBand(1)->GetXSize() > aCol && mDSsolnu->GetRasterBand(1)->GetYSize() > aRow && aRow >=0 && aCol >=0){
        mDSsolnu->GetRasterBand(1)->RasterIO( GF_Read, aCol, aRow, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
        aRes=scanPix[0];
    }
    return aRes;
}
double tuileS2OneDateSco::getCRnormVal(int aCol, int aRow){
    double aRes=0.0;
    if( mDScrnom != NULL && mDScrnom->GetRasterBand(1)->GetXSize() > aCol && mDScrnom->GetRasterBand(1)->GetYSize() > aRow && aRow >=0 && aCol >=0){
        mDScrnom->GetRasterBand(1)->RasterIO( GF_Read, aCol, aRow, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
        // applique le gain
        aRes=scanPix[0]*1.0/127.0;
    }
    return aRes;
}

double tuileS2OneDateSco::getCRSWIR(pts & pt){
    return r_crswir->getValueDouble(pt.X(),pt.Y());
}

double tuileS2OneDateSco::getCRSWIRNorm(pts & pt){

    if (r_crswirNorm!=NULL){
        return r_crswirNorm->getValueDouble(pt.X(),pt.Y())*1.0/127.0;
    } else {
        std::cout << "Attention, r_crswirNorm est null pour " << getDate() <<std::endl;
        return 0;
    }
}

int tuileS2OneDateSco::getMaskSolNu(pts & pt){
    if (r_solnu!=NULL){
        return r_solnu->getValue(pt.X(),pt.Y());
    } else {
        std::cout << "Attention, r_solnu est null pour " << getDate() <<std::endl;
        return 0;
    }
}

//https://gis.stackexchange.com/questions/233874/what-is-the-range-of-values-of-sentinel-2-level-2a-images
// surface _reflectance = DN/ 10 000
// varie souvent entre 0 et 1 mais on peut avoir des valeurs supérieures à 1
void tuileS2OneDateSco::masqueSpecifique(){
    if (mDebug){std::cout << "detection sol nu .." ;}
    /* voir slide de Raphael D.
    1) détection sol nu
    swir1>12.5% et r+v >8% ça aurait pu être OU mais alors ça marche pas bien. attention, R+V>8 pct c'est plutot 16 pct car on additionne la réflectance
    2) nuage
    NG= G/(NIRa+Red+Green) > 0.15
    B>4%
    ainsi que tout les pixels à moins de 30 m (3 pixes donc)
    */
    std::string SWIR1=getRasterR2Name("11");
    std::string m=getRasterMasqGenName();
    std::string b=getRasterR1Name("2");
    std::string v=getRasterR1Name("3");
    std::string r=getRasterR1Name("4");
    std::string NIRa=getRasterR2Name("8A");
    std::string out=getRasterMasqSecName();
    //std::string out=interDirName+"mask_R1_solnu.tif";
    // check que le fichier n'existe pas
    if  (boost::filesystem::exists(m) && boost::filesystem::exists(SWIR1) && boost::filesystem::exists(r)&& boost::filesystem::exists(v)){
       //bool test(0);
       if (boost::filesystem::exists(out)){
             std::time_t t = boost::filesystem::last_write_time(out) ;

             /*tm local_tm = *gmtime(&t);
             year_month_day ymd(year{local_tm.tm_year+1900},month{local_tm.tm_mon+1}, day{local_tm.tm_mday});
             if (ymd.year() != year{2022} | ymd.month() < month{10}){test=1;
             std::cout << " last_write_time " << t << ", " << ymd.year() << " -" << ymd.month() << std::endl;
             }*/
       }

        if (!boost::filesystem::exists(out) | overw) {
            //im 1 = masque général
            //im 2 = swir1
            //im 3 = r
            //im 4 = v

            //std::string exp("im1b1!=1 ? 0 : im2b1/10000<0.125 and ((im3b1+im4b1)/10000)<0.08 ? 1 : im1b1==1 and im2b1/10000.0>0.125 ? 2 : im1b1==1 and ((im3b1+im4b1)/10000.0)>0.08 ? 3 : 0");
            std::string exp("im1b1!=1 ? 0 : im2b1>1250 and im5b1 < 600 and (im3b1+im4b1)>800 ? 2 : 1");

            // valeur 1 : ok - 2 : sol nu détection swir - 3 sol nul détection r+v
            // nouvelle méthode ; je me base sur la formule du code de fordead
            std::string aCommand(path_otb+"otbcli_BandMathX -il "+m+ " " + SWIR1 + " " + v + " " + r + " " + b + " -out '"+ out + compr_otb+"' uint8 -exp '"+exp+"' -ram 5000 -progress 0");
            //std::cout << aCommand << std::endl;
            system(aCommand.c_str());
        }

    } else { std::cout << "fichiers introuvables " << m << " , " << SWIR1 << " , " << v << std::endl;}

    if (boost::filesystem::exists(out)){ r_solnu = std::make_unique<rasterFiles>(out);}

    if (0){
        // nuage - mais trop restrictif pour moi, je ne vais pas l'utiliser tout de suite.
        // après c'est peut-être une combinaison des nuages et des ombres qui sont détectés.
        // Nicolas me dis que les masques nuages de théia sont pas parfait, sourtout pour les anciennes dates.
        std::string out2=interDirName+"mask_R1_cloudINRAE.tif";

        // check que le fichier n'existe pas
        if ((!boost::filesystem::exists(out2) | overw) && boost::filesystem::exists(out)&& boost::filesystem::exists(v)&& boost::filesystem::exists(NIRa) && boost::filesystem::exists(b) && boost::filesystem::exists(r)){
            //im 1 = masque solnul
            //im 2 = green
            //im 3 = nira
            //im 4 = red
            //im5 = bleu

            std::string exp("im1b1==1 and ((im2b1/(im3b1+im4b1+im2b1)) > 0.15 and im5b1/10000.0>0.04) ? 1 : 0");
            std::string aCommand(path_otb+"otbcli_BandMathX -il "+out+ " " + v + " " + NIRa + " " + r + " " +b+ " -out '"+ out2 + compr_otb+"' uint8 -exp '"+exp+"' -ram 5000 -progress 0");
            //std::cout << aCommand << std::endl;
            //system(aCommand.c_str());
            //otbcli_BinaryMorphologicalOperation -in qb_RoadExtract.tif -out opened.tif -channel 1 -xradius 5 -yradius 5 -filter erode
        }
    }
}

std::string tuileS2OneDateSco::getRasterCRnormName() const{
    return outputDirName + mAcqDate.substr(0,4)+mAcqDate.substr(5,2)+mAcqDate.substr(8,2)+ "_CRSWIRnorm"+mSuffix+".tif";
}

std::string tuileS2OneDateSco::getRasterCRName(){
    return outputDirName + mAcqDate.substr(0,4)+mAcqDate.substr(5,2)+mAcqDate.substr(8,2)+ "_CRSWIR"+mSuffix+".tif";
}

std::string tuileS2OneDateSco::getRasterEtatIName(){
    return outputDirName + mAcqDate.substr(0,4)+mAcqDate.substr(5,2)+mAcqDate.substr(8,2)+ "_EtatI"+mSuffix+".tif";
}

std::string tuileS2OneDateSco::getRasterEtatFName(){
    return outputDirName + mAcqDate.substr(0,4)+mAcqDate.substr(5,2)+mAcqDate.substr(8,2)+ "_EtatF"+mSuffix+".tif";
}

// applique le masque EP et le masque nuage et le masque edge (no data)
//==>new mask R1 & R2 with: 1=clear et dans masque EP input utilisateur, 2=not clear (clouds/shadows/etc.), 3=blackfill (nodata at all), 4, clear mais hors masque utilisateur (masque EP)
void tuileS2OneDateSco::masque(){
    if (mDebug){std::cout << "masque .." ;}
    for (int i(1) ; i<3 ; i++){
        //std::string out=interDirName+"mask_R"+std::to_string(i)+".tif";
        std::string out=getRasterMasqGenName(i);
        std::string clm(wd+"/raw/"+decompressDirName+"/MASKS/"+decompressDirName+"_CLM_R"+std::to_string(i)+".tif");
        std::string edg(wd+"/raw/"+decompressDirName+"/MASKS/"+decompressDirName+"_EDG_R"+std::to_string(i)+".tif");
        // check que le fichier out n'existe pas
        if (boost::filesystem::exists(clm) && boost::filesystem::exists(edg)) {
            if ((!boost::filesystem::exists(out) | overw )) {//| 1)) {
                //im 1 = masque EP
                //im 2 = masque edge
                //im 3 masque cloud

                //std::string exp("im1b1==1 and im2b1==0 and im3b1 ==0 ? 1 : im2b1 == 1 ? 3 : 2");
                // je change, je veux claculer le masque sur toutes la tuile pour pouvoir utiliser l'outil en dehors du masque essences en input
                std::string exp("im1b1==0 and im2b1==0 and im3b1 ==0 ? 4 : im1b1==1 and im2b1==0 and im3b1 ==0 ? 1 : im2b1 == 1 ? 3 : 2");
                // semble beaucoup plus lent avec les options de compression gdal
                std::string aCommand(path_otb+"otbcli_BandMathX -il "+getNameMasqueEP(i)+" "+edg+ " " + clm + " -out '"+ out + compr_otb+"' uint8 -exp '"+exp+"' -ram 4000 -progress 0");
                //std::string aCommand(path_otb+"otbcli_BandMathX -il "+EP_mask_path+"masque_EP_T31UFR_R"+std::to_string(i)+".tif "+edg+ " " + clm + " -out "+ out +" uint8 -exp '"+exp+"' -ram 4000 -progress 0");
                //std::cout << aCommand << std::endl;
                system(aCommand.c_str());
            }
        } else { std::cout << "fichiers masques introuvables " << edg << " , " << clm << std::endl;}
    }
}


void tuileS2OneDateSco::createRastEtat(){
     if (mDebug){std::cout << "crée les bandes raster pour stoquer les états .." ;}
     std::string out=getRasterEtatIName();
     std::string in=wd+"/raw/"+decompressDirName+"/MASKS/"+decompressDirName+"_CLM_R1.tif";
     if (boost::filesystem::exists(in)) {
         if ((!boost::filesystem::exists(out) | overw )) {
              std::string aCommand("gdal_create -if "+in+" "+ out);
             // std::cout << aCommand << std::endl;
              system(aCommand.c_str());
         }
         out=getRasterEtatFName();
         if ((!boost::filesystem::exists(out) | overw )) {
              std::string aCommand("gdal_create -if "+in+" "+ out);
             // std::cout << aCommand << std::endl;
              system(aCommand.c_str());
         }
     }
}
