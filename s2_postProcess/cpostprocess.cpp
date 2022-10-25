#include "cpostprocess.h"

int cn(3),cs(4),sco(2), newSco(22),oldSco(21),newCS(42), oldCS(41),newCSoldSco(43);
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
        //compressTif(es->getNameClean());
        //compressTif(es->getNameEvol());
        compressTif(es->getNameRaster());
    }
}

void cPostProcess::statistique(){
    std::cout << "y;tot;1;2;3;4;5;6;21;22;41;42"<< std::endl;
    for (std::shared_ptr<esOney> & es : mVES){
        std::cout << es->getY();
        es->stat();
    }
}
void cPostProcess::statWithStation(){

    std::cout << "statWithStation " << std::endl;

    checkCompression(&pathProbPres);

    Im2D_U_INT1 Im=Im2D_U_INT1::FromFileStd(pathProbPres);
    // determiner les valeurs différentes.
    std::vector<int> vVal;
    U_INT1 ** d = Im.data();
    for (INT x=0; x < Im.sz().x; x++)
    {
        for (INT y=0; y < Im.sz().y; y++)
        {
            if (d[y][x] != 0 &&  d[y][x] != 255)
            {
                int val=d[y][x];
                vVal.push_back(val);
                // remplace par 0 dans l'image pour aller plus vite
                ELISE_COPY(select(Im.all_pts(),Im.in()==val),0,Im.out());
            }
        }
    }
    std::sort(vVal.begin(),vVal.end());
    // recharge l'image
    Im=Im2D_U_INT1::FromFileStd(pathProbPres);
    std::cout << "station;y;tot;1;2;3;4;5;6;21;22;41;42"<< std::endl;
    for (int v : vVal){
        for (std::shared_ptr<esOney> & es : mVES){
            std::cout << v <<";"<< es->getY()<< ";";
            es->statWithMasque(&Im,v);
        }
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

    checkCompression(&pathProbPres);

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

// alignement entre couche de masque et couche input (qui vient du merge de toutes les tuiles)
/*
void cPostProcess::checkAlign(std::string * aRaster, std::string * aRasterMasq){
    GDALDataset *pIn= (GDALDataset*) GDALOpen(aRaster->c_str(), GA_ReadOnly);
    bool test(0);
    const char *comp = "COMPRESSION=DEFLATE";
    if (strcmp(*pIn->GetMetadata("IMAGE_STRUCTURE"),comp)== 0){test=1;}
    GDALClose(pIn);
    if (test){
        std::cout << "compression détectée" << std::endl;
        std::string nameTmp=aRaster->substr(0,aRaster->size()-4)+"_tmp.tif";
        if (!fs::exists(nameTmp)){
            // on décompresse tout ça
            std::string aCommand= std::string("gdal_translate -co 'COMPRESS=NONE' "+ *aRaster +" "+nameTmp+" ");
            std::cout << aCommand << "\n";
            system(aCommand.c_str());

        }
         *aRaster=nameTmp;
    }
}*/

void cPostProcess::checkCompression(std::string * aRaster){
    GDALDataset *pIn= (GDALDataset*) GDALOpen(aRaster->c_str(), GA_ReadOnly);
    bool test(0);
    const char *comp = "COMPRESSION=DEFLATE";
    if (strcmp(*pIn->GetMetadata("IMAGE_STRUCTURE"),comp)== 0){test=1;}
    GDALClose(pIn);
    if (test){
        std::cout << "compression détectée" << std::endl;
        std::string nameTmp=aRaster->substr(0,aRaster->size()-4)+"_tmp.tif";
        if (!fs::exists(nameTmp)){
            // on décompresse tout ça
            std::string aCommand= std::string("gdal_translate -co 'COMPRESS=NONE' "+ *aRaster +" "+nameTmp+" ");
            std::cout << aCommand << "\n";
            system(aCommand.c_str());

        }
         *aRaster=nameTmp;
    }
}

void cPostProcess::project(){
    for (std::shared_ptr<esOney> & es : mVES){
        es->project();
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
     std::cout << "y;oldSco;newSco;oldCS;newCSnewSco;newCSoldSco\n" << std::endl;
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

void  cPostProcess::extractValToPt(std::string aShpIn){


    for (int c(0);c<mVES.size();c++){
        std::shared_ptr<esOney> es=mVES.at(c);
        std::cout << es->getNameRaster() << std::endl;
        // création d'un rasterFile pour extract value
        es->extractValToPt(aShpIn);

    }

}


void cPostProcess::surveillance(){
    // pour la dernière année, je calcul un pourcentage de nouveau scolyte pour une résolution de 5km
    std::shared_ptr<esOney> es = mVES.back();
    int dezoom=500;

    Pt2di aSzR = round_up(Pt2dr(es->getImPtr()->sz())/dezoom);
    Im2D_U_INT1 Im(aSzR.x,aSzR.y,0);


    for (int anY=0 ; anY<aSzR.y ; anY++)
    {
       for (int anX=0 ; anX<aSzR.x ; anX++)
       {
           int nbPix(0);
            Pt2di pos(anX*dezoom,anY*dezoom);
           ELISE_COPY(
                       select(rectangle(pos,pos+dezoom),es->getImPtr()->in()==newSco),
                       1,
                       sigma(nbPix)
                       );

           Im.SetI(Pt2di(anX,anY),nbPix);
       }
    }
       Tiff_Im::CreateFromIm(Im,es->getNameSurveillance());
       es->copyTifMTD(es->getNameSurveillance());
}

/*
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
}*/
