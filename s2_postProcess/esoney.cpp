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

    std::cout << nbnSco/100 << ";" << nboSco/100 << ";" <<nbnCS/100 << ";" <<nboCS/100 << std::endl;
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

void esOney::statWithMasque(Im2D_U_INT1 * imMasque, int aMasqVal){
    INT nb;
    ELISE_COPY(select(mIm->all_pts(),mIm->in()!=0 && mIm->in()!=255 && imMasque->in()==aMasqVal),1,(sigma(nb)<< 1));
    std::cout << nb/100  ;
    std::vector<int> v{1,2,3,4,5,6,21,22,41,42};

    for (int a : v){
        nb=0;
        ELISE_COPY(select(mIm->all_pts(),mIm->in()==a && imMasque->in()==aMasqVal),1,(sigma(nb)<< 1));
        std::cout <<";"<< nb/100 ;
    }
    std::cout << std::endl;
}



void esOney::project(){
    std::string aCommand= std::string("gdalwarp -t_srs EPSG:31370 -te 42250.0 21170.0 295170.0 167700.0 -ot Byte -overwrite -tr 10 10 -co 'COMPRESS=NONE' "+ mRasterName +" "+getNameProj()+" ");
    //std::cout << aCommand << "\n";
    system(aCommand.c_str());
}


void esOney::clean(){
    std::cout << " nettoye carte " << mRasterName << std::endl;
    // repérer les parties connexes de coupe normale et si elles ont beaucoup de voisins en coupe sanitaire, remplacer ces valeurs pas coupes sanitaire
    Neighbourhood V8=Neighbourhood::v8();
    // eviter effet de bord
    ELISE_COPY(
                mIm->border(2),
                0,
                mIm->out()
                );

    // pixels de la catégorie en question :toutes les coupes non sanitaire (image de masque pour accélérer process)
    Im2D_U_INT1 Im(mIm->sz().x,mIm->sz().y,0);
    //ELISE_COPY(select(aIn->all_pts(),aIn->in(4)==cn),1,Im.oclip());
    ELISE_COPY(mIm->all_pts(),mIm->in(),Im.oclip());

    // j'ai trois images pour finir. Input, Im = copie de Input pour neigh_test_and_set qui modifie l'image, Im2 = selection uniquement des coupes sanitaire pour dilate qui est programmé en tendu.

    // dilate en tendu ; on modifie l'input durant les traitements, je fait une couche tmp alors
    Im2D_U_INT1 Im2(mIm->sz().x,mIm->sz().y,0);
    ELISE_COPY(select(mIm->all_pts(),mIm->in(4)==cs),1,Im2.oclip());

    U_INT1 ** d = Im.data();
    for (INT x=0; x < mIm->sz().x; x++)
    {
        for (INT y=0; y < mIm->sz().y; y++)
        {
            if (d[y][x] == cn)
            {
                Liste_Pts_INT2 cc(2);//, cc2(2);
                // lecture de la composante connexe
                ELISE_COPY
                        (
                            conc
                            (Pt2di(x,y),
                             Im.neigh_test_and_set(V8,cn,cs,20)),
                            //sel_func(V8,aIn.in() == c)
                            0,
                            cc
                            );

                // lecture des voisins de cette composante conn
                INT nb_pts;
                // attention avec les dilates, programmation tendue.
                ELISE_COPY
                        (
                            dilate
                            (
                                cc.all_pts(),
                                sel_func(V8,Im2.in()==1)
                                ),
                            0,
                            Im2.out() | (sigma(nb_pts)<< 1)
                            );

                double r=((double) nb_pts)/((double) cc.card());
                //Pt2di cdg;
                //ELISE_COPY (cc.all_pts(),Virgule(FX,FY),cdg.sigma());
                if (r>0.4){
                    //std::cout << "composante connexe " << cdg.x  << " , " <<  cdg.y <<" va être remplacée par coupe sanitaire car r= " << r << " , nombre de pixel " << cc.card() << std::endl;
                    // on change de classe
                    ELISE_COPY (cc.all_pts(),cs,mIm->out());
                } else {
                    //std::cout << "composante connexe " << cdg.x  << " , " <<  cdg.y <<" ne va pas être remplacée par coupe sanitaire car r= " << r << " , nombre de pixel " << cc.card() << std::endl;
                }

                // fin if (d[y][x] == 1)
            }
        }
    }

    //Tiff_Im::CreateFromIm(aIn,getNameClean());
    //mIm=& aIn;
}


void esOney::copyTifMTD(std::string aRasterOut){
    // copy projection et src dans gdal
    GDALDataset *pIn, *pOut;
    GDALDriver *pDriver;
    const char *pszFormat = "GTiff";
    pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    pOut = (GDALDataset*) GDALOpen(aRasterOut.c_str(), GA_Update);
    pIn = (GDALDataset*) GDALOpen(mRasterName.c_str(), GA_ReadOnly);
    pOut->SetProjection( pIn->GetProjectionRef() );
    double tr[6];
    pIn->GetGeoTransform(tr);
    pOut->SetGeoTransform(tr);
    GDALClose(pIn);
    GDALClose(pOut);
}

