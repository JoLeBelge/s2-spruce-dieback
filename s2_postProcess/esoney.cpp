#include "esoney.h"
extern int cn,cs,sco, newSco,oldSco,newCS, oldCS;
void esOney::createTmp(){

    // en fait mes inputs sont déjà décompressé et lourd donc je ne crée pas de raster temporaire.
    //std::string aCommand= std::string("gdal_translate -co 'COMPRESS=NONE' "+ mRasterName +" "+getNameTmp()+" ");
    //std::cout << aCommand << "\n";
    //system(aCommand.c_str());
    GDALDataset *pIn= (GDALDataset*) GDALOpen(mRasterName.c_str(), GA_ReadOnly);
    bool test(0);
    //const char *comp = "DEFLATE";
    // comparaison de deux char * : pas d'overload pour ==, attention
    //if (strcmp(pIn->GetMetadataItem("COMPRESSION", "IMAGE_STRUCTURE"),comp)== 0){test=1;}
    const char *comp = "COMPRESSION=DEFLATE";
    if (strcmp(*pIn->GetMetadata("IMAGE_STRUCTURE"),comp)== 0){test=1;}


    //std::cout << "compression " << *pIn->GetMetadata("IMAGE_STRUCTURE")<< std::endl;

    //std::cout << "compression " << pIn->GetMetadataItem("COMPRESSION", "IMAGE_STRUCTURE")<< std::endl;

    GDALClose(pIn);

    if (test){
        std::cout << "compression détectée" << std::endl;
        if (!fs::exists(getNameTmp())){
            // on décompresse tout ça
            std::string aCommand= std::string("gdal_translate -co 'COMPRESS=NONE' "+ mRasterName +" "+getNameTmp()+" ");
            std::cout << aCommand << "\n";
            system(aCommand.c_str());

        }
         mRasterName=getNameTmp();
    }

    //lecture du raster
    std::cout << "charge image " << mRasterName << std::endl;
    mIm=new Im2D_U_INT1(Im2D_U_INT1::FromFileStd(mRasterName));
    // std::cout << "image chargée" << std::endl;
}


void esOney::updateCoupeSan(Im2D_U_INT1 * imPrevYear){
    ELISE_COPY(select(imPrevYear->all_pts(),imPrevYear->in()==cs),cs,mIm->oclip());
    // j'en profite pour bloquer les pixels scolytés que j'avais pas bien bloqué dans s2_timeS aout 2021
    ELISE_COPY(select(imPrevYear->all_pts(),imPrevYear->in()==sco && mIm->in()!=cs),sco,mIm->oclip());
}

void esOney::updateCoupeSanRetro(Im2D_U_INT1 * imNextYear){
    ELISE_COPY(select(imNextYear->all_pts(),imNextYear->in()==cs && mIm->in()==cn),cs,mIm->oclip());
}

void esOney::detectNewSco(Im2D_U_INT1 * imPrevYear){
    // attention, déjà il faut être sur que les Im2D_U_INT1 sont bien les cleans et non pas les images originale.
    INT nbnSco, nboSco,nbnCS,nboCS;

    // masque ND
    Im2D_U_INT1 nd(mIm->sz().x,mIm->sz().y,0);
    // en fait c'est 255 les no data..
    ELISE_COPY(select(mIm->all_pts(),mIm->in()!=0 && mIm->in()!=255),1,nd.out());

    // ancien scolyte
    ELISE_COPY(select(mIm->all_pts(),nd.in()==1 && imPrevYear->in()==sco && mIm->in()!=cs && mIm->in()!=cn)
               ,oldSco,mIm->oclip() | (sigma(nboSco)<< 1));
    //nouveau scolyte
    ELISE_COPY(select(mIm->all_pts(),nd.in()==1 && imPrevYear->in()!=sco && mIm->in()==sco)
               ,newSco,mIm->oclip()| (sigma(nbnSco)<< 1));
    // ancienne coupe sanitaire
    ELISE_COPY(select(mIm->all_pts(),nd.in()==1 && imPrevYear->in()==cs)
               ,oldCS,mIm->oclip()| (sigma(nboCS)<< 1));
    // nouvelles coupe sanitaire
    ELISE_COPY(select(mIm->all_pts(),nd.in()==1 && mIm->in()==cs && imPrevYear->in()!=cs)
               ,newCS,mIm->oclip()| (sigma(nbnCS)<< 1));

    std::cout << nbnSco << ";" << nboSco << ";" <<nbnCS << ";" <<nboCS << std::endl;
}


void esOney::saveClean(){
    Tiff_Im::CreateFromIm(*mIm,getNameClean());
    copyTifMTD(getNameClean());
}
void esOney::saveMasq(){
    Tiff_Im::CreateFromIm(*mIm,getNameMasq());
    copyTifMTD(getNameMasq());
}

void esOney::saveEvol(){
    Tiff_Im::CreateFromIm(*mIm,getNameEvol());
    copyTifMTD(getNameEvol());
}

esOney::esOney(std::string aRaster, int y, int mode):mRasterName(aRaster),mIm(NULL),mAn(y){

    switch (mode){
    case 1:{
        createTmp();
        break;
    }
    case 2:{
        loadClean();
        break;
    }
    }
}

void esOney::loadClean(){
    if (mIm!=NULL){delete mIm;}
    if (fs::exists(getNameClean())){
        std::cout << "charge image " << getNameClean() << std::endl;
        mIm=new Im2D_U_INT1(Im2D_U_INT1::FromFileStd(getNameClean()));
    } else { std::cout << getNameClean() << " n'existe pas" << std::endl;}
}

void esOney::stat(){
    // statistique sur la carte input , qui peut aussi bien être une carte d'évolution qu'une carte es simple ou masquée.
    INT nb;
    ELISE_COPY(select(mIm->all_pts(),mIm->in()!=0 && mIm->in()!=255),1,(sigma(nb)<< 1));
    std::cout <<";"<< nb/100 ;
    std::vector<int> v{1,2,3,4,5,6,21,22,41,42};

    for (int a : v){
        nb=0;
        ELISE_COPY(select(mIm->all_pts(),mIm->in()==a),1,(sigma(nb)<< 1));
        std::cout <<";"<< nb/100 ;
    }
    std::cout << std::endl;
}
