#include "tuiles2onedatepheno.h"

extern std::string wd;
extern std::string path_otb;
extern std::string EP_mask_path;
extern std::string compr_otb;
extern int globSeuilCC;
extern bool overw;
extern bool mDebug;

extern std::vector<std::string> vBR2;
extern std::vector<std::string> vBR1;

void tuileS2OneDatePheno::readLines(int resol,int aRow) const{

    switch (resol){

    case 1:{
        if (vDS.find("masqR1")!=vDS.end()){
            GDALDataset * DSpt=vDS.at("masqR1");
            if(  DSpt != NULL &&  DSpt->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){
                DSpt->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, mXSize, 1, scanLineMasq, mXSize,1, GDT_Float32, 0, 0 );
            } else {std::cout << " euuhh non pas masqR1" << std::endl;}
        }

        if (vDS.find("2")!=vDS.end()){
            GDALDataset * DSpt=vDS.at("2");
            if(  DSpt != NULL &&  DSpt->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){
                DSpt->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, mXSize, 1, scanLine1, mXSize,1, GDT_Float32, 0, 0 );
            } else {std::cout << " euuhh non 2" << std::endl;}
        }
        if (vDS.find("3")!=vDS.end()){
            GDALDataset * DSpt=vDS.at("3");
            if(  DSpt != NULL &&  DSpt->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){
                DSpt->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, mXSize, 1, scanLine2, mXSize,1, GDT_Float32, 0, 0 );
            } else {std::cout << " euuhh non 3" << std::endl;}
        }
        if (vDS.find("4")!=vDS.end()){
            GDALDataset * DSpt=vDS.at("4");
            if(  DSpt != NULL &&  DSpt->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){
                DSpt->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, mXSize, 1, scanLine3, mXSize,1, GDT_Float32, 0, 0 );
            } else {std::cout << " euuhh non 4" << std::endl;}
        }
        if (vDS.find("8")!=vDS.end()){
            GDALDataset * DSpt=vDS.at("8");
            if(  DSpt != NULL &&  DSpt->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){
                DSpt->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, mXSize, 1, scanLine4, mXSize,1, GDT_Float32, 0, 0 );
            } else {std::cout << " euuhh non 5" << std::endl;}
        }
        break;
    }
    case 2:{
        int XR2=mXSize/2;
        if (vDS.find("masqR2")!=vDS.end()){
            GDALDataset * DSpt=vDS.at("masqR2");
            if(  DSpt != NULL &&  DSpt->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){
                DSpt->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, XR2, 1, scanLineMasq, XR2,1, GDT_Float32, 0, 0 );
            } else {std::cout << " euuhh non pas masqR2" << std::endl;}
        }  else {std::cout << "masqR2?" << std::endl;}
        if (vDS.find("8A")!=vDS.end()){
            GDALDataset * DSpt=vDS.at("8A");
            if(  DSpt != NULL &&  DSpt->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){
                DSpt->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, XR2, 1, scanLine1, XR2,1, GDT_Float32, 0, 0 );
            } else {std::cout << " euuhh non 8A" << std::endl;}
        } else {std::cout << "8A?" << std::endl;}
        if (vDS.find("11")!=vDS.end()){
            GDALDataset * DSpt=vDS.at("11");
            if(  DSpt != NULL &&  DSpt->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){
                DSpt->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, XR2, 1, scanLine2, XR2,1, GDT_Float32, 0, 0 );
            } else {std::cout << " euuhh non 11" << std::endl;}
        }
        if (vDS.find("12")!=vDS.end()){
            GDALDataset * DSpt=vDS.at("12");
            if(  DSpt != NULL &&  DSpt->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){
                DSpt->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, XR2, 1, scanLine3, XR2,1, GDT_Float32, 0, 0 );
            } else {std::cout << " euuhh non 12" << std::endl;}
        }
        if (vDS.find("5")!=vDS.end()){
            GDALDataset * DSpt=vDS.at("5");
            if(  DSpt != NULL &&  DSpt->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){
                DSpt->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, XR2, 1, scanLine4, XR2,1, GDT_Float32, 0, 0 );
            } else {std::cout << " euuhh non 5" << std::endl;}
        }
        if (vDS.find("6")!=vDS.end()){
            GDALDataset * DSpt=vDS.at("6");
            if(  DSpt != NULL &&  DSpt->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){
                DSpt->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, XR2, 1, scanLine5, XR2,1, GDT_Float32, 0, 0 );
            } else {std::cout << " euuhh non 6" << std::endl;}
        }
        if (vDS.find("7")!=vDS.end()){
            GDALDataset * DSpt=vDS.at("7");
            if(  DSpt != NULL &&  DSpt->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){
                DSpt->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, XR2, 1, scanLine6, XR2,1, GDT_Float32, 0, 0 );
            } else {std::cout << " euuhh non 7" << std::endl;}
        }
        break;
    }
    default:
        break;

    }
}

bool tuileS2OneDatePheno::openDS(){

    bool aRes(1);
    for (std::string b : vBR1){
        GDALDataset * DSpt= (GDALDataset *) GDALOpen( getRasterR1Name(b).c_str(), GA_ReadOnly );
        vDS.emplace(std::make_pair(b,DSpt));
    }
    for (std::string b : vBR2){
        GDALDataset * DSpt= (GDALDataset *) GDALOpen( getOriginalRasterR2Name(b).c_str(), GA_ReadOnly );
        vDS.emplace(std::make_pair(b,DSpt));
    }
    for (int r(1) ; r<3;r++){
        GDALDataset * DSpt= (GDALDataset *) GDALOpen( getRasterMasqGenName(r).c_str(), GA_ReadOnly );
        vDS.emplace(std::make_pair("masqR"+std::to_string(r),DSpt));
    }
    scanPix=(float *) CPLMalloc( sizeof( float ) * 1 );
    initLines(1);

    return aRes;
}
void tuileS2OneDatePheno::closeDS(){

    if (scanPix!=NULL){ CPLFree(scanPix);}
    if (scanLineMasq!=NULL){ CPLFree(scanLineMasq);}
    if (scanLine1!=NULL){ CPLFree(scanLine1);}
    if (scanLine2!=NULL){ CPLFree(scanLine2);}
    if (scanLine3!=NULL){ CPLFree(scanLine3);}
    if (scanLine4!=NULL){ CPLFree(scanLine4);}

    for (auto kv : vDS){
        GDALDataset * DSpt=kv.second;
        if (DSpt != NULL){GDALClose( DSpt );}
    }
}

void tuileS2OneDatePheno::initLines(int resol){
    //std::cout << "tuileS2OneDatePheno initLine " << resol << std::endl;
    if (scanLineMasq!=NULL){ CPLFree(scanLineMasq);}
    if (scanLine1!=NULL){ CPLFree(scanLine1);}
    if (scanLine2!=NULL){ CPLFree(scanLine2);}
    if (scanLine3!=NULL){ CPLFree(scanLine3);}
    if (scanLine4!=NULL){ CPLFree(scanLine4);}
    if (scanLine5!=NULL){ CPLFree(scanLine5);}
    if (scanLine6!=NULL){ CPLFree(scanLine6);}
    switch (resol) {
    case 1:{
        scanLineMasq=(float *) CPLMalloc( sizeof( float ) * mYSize );
        scanLine1=(float *) CPLMalloc( sizeof( float ) * mYSize );
        scanLine2=(float *) CPLMalloc( sizeof( float ) * mYSize );
        scanLine3=(float *) CPLMalloc( sizeof( float ) * mYSize );
        scanLine4=(float *) CPLMalloc( sizeof( float ) * mYSize );
        break;}
    case 2:{
        int YR2=mYSize/2;
        scanLineMasq=(float *) CPLMalloc( sizeof( float ) * YR2 );
        scanLine1=(float *) CPLMalloc( sizeof( float ) * YR2 );
        scanLine2=(float *) CPLMalloc( sizeof( float ) * YR2 );
        scanLine3=(float *) CPLMalloc( sizeof( float ) * YR2 );
        scanLine4=(float *) CPLMalloc( sizeof( float ) * YR2 );
        scanLine5=(float *) CPLMalloc( sizeof( float ) * YR2 );
        scanLine6=(float *) CPLMalloc( sizeof( float ) * YR2 );
        break;}
    default:
        break;
    }
    //std::cout << "done " << std::endl;
}

void tuileS2OneDatePheno::masque(){
    if (mDebug){std::cout << "masque .." << std::endl;}
    for (int i(1) ; i<3 ; i++){

        std::string out=getRasterMasqGenName(i);

        std::string clm(wd+"/raw/"+decompressDirName+"/MASKS/"+decompressDirName+"_CLM_R"+std::to_string(i)+".tif");
        std::string edg(wd+"/raw/"+decompressDirName+"/MASKS/"+decompressDirName+"_EDG_R"+std::to_string(i)+".tif");
        // check que le fichier out n'existe pas
        if (boost::filesystem::exists(clm) && boost::filesystem::exists(edg)) {
            if ((!boost::filesystem::exists(out) | overw)) { //| 1)) {
                //im 1 = masque ep our compo
                //im 2 = masque edge
                //im 3 masque cloud
                std::cout << "out="<< out << std::endl;
                //std::string exp("im1b1==1 and im2b1==0 and im3b1 ==0 ? 1 : im2b1 == 1 ? 3 : 2");
                // je change, je veux claculer le masque sur toutes la tuile pour pouvoir utiliser l'outil en dehors du masque essences en input
                std::string exp("im1b1==0 and im2b1==0 and im3b1 ==0 ? 4 : im1b1==1 and im2b1==0 and im3b1 ==0 ? 1 : im2b1 == 1 ? 3 : 2");
                // semble beaucoup plus lent avec les options de compression gdal
                std::string aCommand(path_otb+"otbcli_BandMathX -il "+getNameMasque(i)+" "+edg+ " " + clm + " -out '"+ out + compr_otb+"' uint8 -exp '"+exp+"' -ram 4000 -progress 0");
                //std::string aCommand(path_otb+"otbcli_BandMathX -il "+EP_mask_path+"masque_EP_T31UFR_R"+std::to_string(i)+".tif "+edg+ " " + clm + " -out "+ out +" uint8 -exp '"+exp+"' -ram 4000 -progress 0");
                //std::cout << aCommand << std::endl;
                system(aCommand.c_str());
            }
        } else { std::cout << "fichiers masques introuvables " << edg << " , " << clm << std::endl;}
    }
}
