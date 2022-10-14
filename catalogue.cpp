#include "catalogue.h"

extern std::string wd;
extern std::string path_otb;
//extern std::string iprfwFile;
extern int globSeuilCC;
extern int year_analyse;
extern double Xdebug;
extern double Ydebug;
extern bool overw;
extern bool debugDetail;

extern std::string globTuile;
extern std::string globResXYTest;
extern std::string globSuffix;

bool mDebug(0);

catalogue::catalogue(std::string aJsonFile):mDSmaskR1(NULL),mDSmaskR2(NULL),scanLineR1(NULL),scanLineR2(NULL),scanPix(NULL){
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
                tuileS2OneDate * t=new tuileS2OneDate();
                t->mProd =f[i]["properties"]["productIdentifier"].GetString();
                t->mFeature_id = f[i]["id"].GetString();
                if (f[i]["properties"]["cloudCover"].IsInt()){t->mCloudCover = f[i]["properties"]["cloudCover"].GetInt();}
                t->mAcqDate =f[i]["properties"]["startDate"].GetString();
                t->mDate=ymdFromString(t->mAcqDate);
                t->mPtrDate=& t->mDate;
                t->mProdDate =f[i]["properties"]["productionDate"].GetString();
                t->mSuffix = globSuffix;
                //t->mPubDate =f[i]["properties"]["published"].GetString();
                mVProduts.push_back(t);
            }
        }

        std::for_each(
                    std::execution::par_unseq,
                    mVProduts.begin(),
                    mVProduts.end(),
                    [](tuileS2OneDate * t)
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

    }

    // en fait j'ai un bug qui apparait quand je traite les tuiles dans la boucle en parrallel, des fois les métadonnées lue dans le XML ne sont pas prise en compte, des fois oui.
    // pourtant j'ai besoin de cette boucle pour le téléchargement en parrallel
    for(tuileS2OneDate * t : mVProduts)
    {
        delete t;
    }
    mVProduts.clear();

    init();

}


void catalogue::init(){
    // listing des dossiers dans intermediate puis lecture des métadonnées XML
    std::cout << "création de la collection depuis les dossiers présent dans le répertoire " << wd << "intermediate/ " << std::endl;

    std::cout << "d1 " << d1 << " , d2 " << d2 << std::endl;
     std::istringstream in1{d1},in2{d2};
     //date::sys_days sd1,sd2;
     //date::parse(in1, "%F", sd1);
     //date::parse(in2, "%F", sd2);
     date::year_month_day dfirst;
     date::year_month_day dlast;

     in1 >> date::parse("%F", dfirst);
     in2 >> date::parse("%F", dlast);


    for(auto & p : boost::filesystem::directory_iterator(wd+"intermediate/")){
        std::string aDecompressDirName= p.path().filename().string();
        // création d'une tuile
        tuileS2OneDate * t=new tuileS2OneDate();
        t->decompressDirName =aDecompressDirName;
        t->readXML();
        t->mSuffix = globSuffix;

        if (t->getymd() > dfirst && t->getymd() < dlast){
        mVProduts.push_back(t);
        } else if (mDebug){ std::cout << "prise de vue " << t->getDate() << " pas prise en compte , date1  " << dfirst <<" , date 2 " << dlast<< std::endl;}
    }
    // change le nombre de fichiers que l'appli peut ouvrir simultanément
    // fonctionne pas , je dois lancer manuellement la commande ulimit -n 65536 avant d'utiliser l'appli

    struct rlimit lim = {65536, 65536};
    if (setrlimit(RLIMIT_STACK, &lim) == -1) {
                           printf("rlimit failed\n");
                   }
    std::cout << " done .." << std::endl;
}

catalogue::catalogue():mDSmaskR1(NULL),mDSmaskR2(NULL),scanLineR1(NULL),scanLineR2(NULL),scanPix(NULL){
    init();
}

// comptage des produits avec cloudcover ok
int catalogue::countValid(){
    int aRes(0);
    for (const tuileS2OneDate * t : mVProduts){
        if (t->mCloudCover<globSeuilCC){aRes++;}
    }
    return aRes;
}


void catalogue::summary(){
        if (mDebug){for (tuileS2OneDate * t : mVProduts){t->cat();}}
        std::cout << " Nombre de produits ok ; " << countValid() << std::endl;
}

void catalogue::readMasqLine(int aRow, int aRes){
    switch (aRes) {
    case 1:{
        if( mDSmaskR1 != NULL && mDSmaskR1->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){
            mDSmaskR1->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, mX, 1, scanLineR1, mX,1, GDT_Float32, 0, 0 );
        }else {
            std::cout << "readMasqLine R1 ; failed " << std::endl;
        }
        break;}
    case 2:{
        if( mDSmaskR2 != NULL && mDSmaskR2->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){
            mDSmaskR2->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, mX/2, 1, scanLineR2, mX/2,1, GDT_Float32, 0, 0 );
        }else {
            std::cout << "readMasqLine R2 ; failed " << std::endl;
        }
        break;}
    default:
        break;
    }
}

int catalogue::getMasqVal(int aCol, int aRow){
    //std::cout << "getMasqEpVal " << std::endl;
    int aRes=0;

    if( mDSmaskR1 != NULL && mDSmaskR1->GetRasterBand(1)->GetXSize() > aCol && mDSmaskR1->GetRasterBand(1)->GetYSize() > aRow && aRow >=0 && aCol >=0){
        mDSmaskR1->GetRasterBand(1)->RasterIO( GF_Read, aCol, aRow, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
        aRes=scanPix[0];
    }

    return aRes;
}

int catalogue::getEPSG(){
    int aRes(0);
    if (mVProduts.size()>0){aRes=mVProduts.at(0)->getEPSG();}
    return aRes;
}

