#include "catalogue.h"

extern std::string wd;
extern std::string buildDir;
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
}

catalogue::catalogue():mDSmaskR1(NULL),mDSmaskR2(NULL),scanLineR1(NULL),scanLineR2(NULL),scanPix(NULL){
    // listing des dossiers dans intermediate puis lecture des métadonnées XML
    std::cout << "création de la collection depuis les dossiers présent dans le répertoire " << wd << "intermediate/ " << std::endl;
    for(auto & p : boost::filesystem::directory_iterator(wd+"intermediate/")){
        std::string aDecompressDirName= p.path().filename().string();
        // création d'une tuile
        tuileS2OneDate * t=new tuileS2OneDate();
        t->decompressDirName =aDecompressDirName;
        t->readXML();

        mVProduts.push_back(t);
    }
    std::cout << " done .." << std::endl;
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
