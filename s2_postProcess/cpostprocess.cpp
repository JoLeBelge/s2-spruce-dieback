#include "cpostprocess.h"

int cn(3),cs(4),sco(2), newSco(22),oldSco(21),newCS(42), oldCS(41);
extern std::string pathProbPres;

std::string path_otb("/home/lisein/OTB/OTB-7.3.0-Linux64/bin/");
std::string compr_otb="?&gdal:co:INTERLEAVE=BAND&gdal:co:TILED=YES&gdal:co:BIGTIFF=YES&gdal:co:COMPRESS=DEFLATE&gdal:co:ZLEVEL=9";

cPostProcess::cPostProcess(std::vector<std::pair<int, string>> aMRaster, int mode)
{
    GDALAllRegister();
    for (auto kv : aMRaster){
        std::string pathIn=kv.second;
        std::shared_ptr<esOney> es=std::make_shared<esOney>(pathIn,kv.first, mode);
        // gdal a virtual char **GetMetadata(const char *pszDomain = "") pour voir si le raster est compressé ou pas mais pour le moment je ne m'ennuie pas, je le décompresse à chaque fois pour être sur
        mVES.push_back(es);
        //std::cout << "done" << std::endl;
    }
}

void cPostProcess::compress(){
    for (std::shared_ptr<esOney> & es : mVES){
        compressTif(es->getNameClean());
        compressTif(es->getNameEvol());
    }
}

void cPostProcess::statistique(){
    std::cout << "y;tot;1;2;3;4;5;6;21;22;41;42"<< std::endl;
    for (std::shared_ptr<esOney> & es : mVES){
        std::cout << es->getY();
        es->stat();
    }
}

void cPostProcess::compressTif(std::string aIn){
    std::string aCommand= std::string("gdalwarp -co 'COMPRESS=DEFLATE' -overwrite "+ aIn +" "+aIn.substr(0,aIn.size()-4)+"_co.tif ");
    //std::cout << aCommand << "\n";
    system(aCommand.c_str());
}

// masque les cartes états sanitaires avec la probabilité de présence de l'EP
void cPostProcess::masque(int seuilPP){
    std::cout << "charge l'image de probabilité de présence " << std::endl;
    Im2D_U_INT1 PP=Im2D_U_INT1::FromFileStd(pathProbPres);
    for (std::shared_ptr<esOney> & es : mVES){
        // gdal et otb fonctionne, mais c'est pas franchement rapide. le faire avec Elise permettrait d'accélérer la chose, déjà vu que je vais ouvrir qu'une seule fois le raster de prob de présence.
        //std::string aCommand= std::string("gdal_calc.py -A "+  +" -B "+pathProbPres +" --calc='(B>="+std::to_string(seuilPP)+")*A' --type=Byte --outfile="+ es->getNameRaster().substr(0,es->getNameRaster().size()-4)+"_masque.tif ");
        //std::string aCommand(path_otb+"otbcli_BandMathX -il "+es->getNameRaster()+" "+pathProbPres+ " -out '"+ es->getNameRaster().substr(0,es->getNameRaster().size()-4)+"_masque.tif" + compr_otb+"' uint8 -exp ' im2b1 >="+ std::to_string(seuilPP)+" ? im1b1 : 0' -ram 4000 -progress 0");
        //std::cout << aCommand << "\n";
        //system(aCommand.c_str());

        std::cout << " masque Etat San " << std::endl;
        Im2D_U_INT1 * im =es->getImPtr();
        ELISE_COPY(select(im->all_pts(),PP.in()<seuilPP),0,im->oclip());
        es->saveMasq();

    }
}

void cPostProcess::clean(){
    int c(0);
    for (std::shared_ptr<esOney> & es : mVES){
        es->clean();
        // répercuter les modifications de cette année sur les années d'après
        for (int i(c+1);i<mVES.size();i++){
            mVES.at(i)->updateCoupeSan(es->getImPtr());
        }
        //es->saveClean();
        c++;
    }

    // maitenant que j'ai fait la boucle dans un sens, je la refait dans l'autre car certaines coupes sanitaires sont ajoutées en 2021 alors qu'elles étaitent déjà présentent en 2018
    mVES.at(mVES.size()-1)->saveClean();

    for (int c(mVES.size()-2);c>=0;c--){
        std::shared_ptr<esOney> es=mVES.at(c);
        es->updateCoupeSanRetro(mVES.at(c+1)->getImPtr());
        es->saveClean();
    }
}

void cPostProcess::evol(){
    // première année ; que des nouveaux scolytes. on donne une image pour l'année précédente qui est vide
    std::shared_ptr<esOney> es0=mVES.at(0);
    Im2D_U_INT1 Im(es0->getImPtr()->sz().x,es0->getImPtr()->sz().y,0);
     std::cout << "y;newSco;oldSco;newCS;oldCS \n" << std::endl;
    es0->detectNewSco(&Im);
    es0->saveEvol();
    es0->loadClean();// j'ai modifié l'image de cette état san , je dois la recharger car j'en ai besoin pour la boucle ci-dessous

    for (int c(1);c<mVES.size();c++){
        std::shared_ptr<esOney> es=mVES.at(c);
        std::cout << es->getY() << ";";
        es->detectNewSco(mVES.at(c-1)->getImPtr());
        es->saveEvol();
        es->loadClean();
    }
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

void cPostProcess::cleanVoisinage(Im2D_U_INT1 * aIn,int Val2Clean, int ValConflict1, int seuilVois){

    std::cout << "Nettoyage carte AE valeur " << Val2Clean << ", seuil nombre de voisin " << seuilVois << " Apport d'eau à ne pas modifier : " << ValConflict1 << std::endl;

    Im2D_U_INT1 Im(aIn->sz().x,aIn->sz().y,0);
    // pixels qu'on va peut-être changer de catégorie
    ELISE_COPY(select(aIn->all_pts(),aIn->in(0)!=Val2Clean && aIn->in(0)!=ValConflict1),1,Im.oclip());
    //statistique sur le nombre de pixel voisin qui sont de la classe d'AE à nettoyer
    Im2D_U_INT1 ImNbVois(aIn->sz().x,aIn->sz().y,0);
    Im2D_BIN * IbinTmp=new Im2D_BIN(aIn->sz().x,aIn->sz().y,0);
    ELISE_COPY(select(aIn->all_pts(),aIn->in(4)==Val2Clean),1,IbinTmp->oclip());
    ELISE_COPY(aIn->all_pts(),rect_som(IbinTmp->in(0),1),ImNbVois.oclip());
    delete IbinTmp;
    ELISE_COPY(select(aIn->all_pts(),ImNbVois.in()>=seuilVois && Im.in()==1),Val2Clean,aIn->oclip());
}

void cPostProcess::cleanIsolatedPix(Im2D_U_INT1 * aIn,int Val2Clean, int Val2Replace, int seuilVois){

    std::cout << "Nettoyage carte AE valeur " << Val2Clean << " pour pixels isolé, seuil nombre de voisin " << seuilVois << ", remplace par : " << Val2Replace << std::endl;

    Im2D_U_INT1 Im(aIn->sz().x,aIn->sz().y,0);
    // pixels qu'on va peut-être changer de catégorie
    ELISE_COPY(select(aIn->all_pts(),aIn->in(0)==Val2Clean),1,Im.oclip());
    //statistique sur le nombre de pixel voisin qui sont de la classe d'AE à nettoyer
    Im2D_U_INT1 ImNbVois(aIn->sz().x,aIn->sz().y,0);
    Im2D_BIN * IbinTmp=new Im2D_BIN(aIn->sz().x,aIn->sz().y,0);
    ELISE_COPY(select(aIn->all_pts(),aIn->in(4)==Val2Clean),1,IbinTmp->oclip());
    ELISE_COPY(aIn->all_pts(),rect_som(IbinTmp->in(0),1),ImNbVois.oclip());
    delete IbinTmp;
    // seuil voisin +1 car le pixels en question est compté dedans car de la mm catégorie mais n'est pas voisin de lui-même
    ELISE_COPY(select(aIn->all_pts(),ImNbVois.in()<=seuilVois+1 && Im.in()==1),Val2Replace,aIn->oclip());
}

void cPostProcess::fillHole(Im2D_U_INT1 * aIn,int Val2Clean, int ValCopain,int ValConflict1, int seuilVois,int aSz1,int aSz2){

    std::cout << "Nettoyage carte AE valeur " << Val2Clean << ", soutenu par valeur " << ValCopain <<", seuil nombre de voisin " << seuilVois << " Apport d'eau à ne pas modifier : " << ValConflict1 << std::endl;

    Im2D_U_INT1 Im(aIn->sz().x,aIn->sz().y,0);
    // pixels qu'on va peut-être changer de catégorie
    ELISE_COPY(select(aIn->all_pts(),aIn->in(0)!=Val2Clean && aIn->in(0)!=ValConflict1),1,Im.oclip());
    //statistique sur le nombre de pixel voisin qui sont de la classe d'AE à nettoyer
    Im2D_U_INT1 ImNbVois(aIn->sz().x,aIn->sz().y,0);
    Im2D_BIN * IbinTmp=new Im2D_BIN(aIn->sz().x,aIn->sz().y,0);
    ELISE_COPY(select(aIn->all_pts(),(aIn->in(4)==Val2Clean)| (aIn->in(4)==ValCopain)),1,IbinTmp->oclip());

    // 49 pixels de voisinage (3+1+3) pow 2
    ELISE_COPY(aIn->all_pts(),rect_som(IbinTmp->in(0),aSz1)-rect_som(IbinTmp->in(0),aSz2),ImNbVois.oclip());
    delete IbinTmp;
    ELISE_COPY(select(aIn->all_pts(),ImNbVois.in()>=seuilVois && Im.in()==1),Val2Clean,aIn->oclip());
}
