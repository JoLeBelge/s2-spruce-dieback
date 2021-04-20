#include "tuiles2.h"
int globSeuilCC(35);

std::string wd("/home/lisein/Documents/Scolyte/S2/test/");
std::string buildDir("/home/lisein/Documents/Scolyte/S2/build-s2_ts/");

std::vector<std::string> tokeep{"_FRE_",//Flat REflectance bands (not SRE)
                                "_MTD_ALL",//metedata xml
                                "_CLM_R1",//cloud and shodow mask 10m
                                "_MG2_R1",//geophysic mask with snow 10m
                                "_EDG_R1",//nodata mask 10m, edge mask
                                "_CLM_R2",//cloud and shodow mask 20m
                                "_MG2_R2",//geophysic mask with snow 20m
                                "_EDG_R2"//nodata mask 20m
                               };
catalogue::catalogue(std::string aJsonFile){
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
                tuileS2 t;
                t.mProd =f[i]["properties"]["productIdentifier"].GetString();
                t.mFeature_id = f[i]["id"].GetString();
                if (f[i]["properties"]["cloudCover"].IsInt()){t.mCloudCover = f[i]["properties"]["cloudCover"].GetInt();}
                t.mAcqDate =f[i]["properties"]["startDate"].GetString();
                t.mProdDate =f[i]["properties"]["productionDate"].GetString();
                t.mPubDate =f[i]["properties"]["published"].GetString();
                mVProduts.push_back(t);
            }
        }

        // récapitulatif;
        summary();

        std::for_each(
                    std::execution::par_unseq,
                    mVProduts.begin(),
                    mVProduts.end(),
                    [](tuileS2& t)
        {

            // check que cloudcover est en dessous de notre seuil
            if (t.mCloudCover<globSeuilCC){
                //check si le produit existe déjà décompressé avant de le télécharger
                if (!t.pretraitementDone()){
                    t.download();
                    t.nettoyeArchive();
                    t.decompresse();
                    t.removeArchive();
                } else {
                    std::cout << t.mProd << " a déjà été téléchargé précédemment" << std::endl;
                }
                t.readXML();
                t.catQual();
            }
        });

        //mVProduts.at(1).wrap();
        //mVProduts.at(1).masque();
        //mVProduts.at(1).resample();
    }
}

// comptage des produits avec cloudcover ok
int catalogue::countValid(){
    int aRes(0);
    for (tuileS2 t : mVProduts){
        if (t.mCloudCover<globSeuilCC){aRes++;}
    }
    return aRes;
}

void tuileS2::download(){
    // création d'un token d'authentification
    std::string aCommand("curl -k -s -X POST  --data-urlencode 'ident=nicolas.latte@uliege.be' --data-urlencode 'pass=a1b2c3d4ULG_' https://theia.cnes.fr/atdistrib/services/authenticate/>token.json");
    //std::cout << aCommand << std::endl;
    system(aCommand.c_str());
    std::ifstream in(buildDir+"/token.json");
    std::ostringstream sstream;
    sstream << in.rdbuf();
    const std::string token(sstream.str());
    in.close();

    archiveName=wd+mProd+".zip";
    std::string get_product = "curl  -o "+archiveName+" -k -H 'Authorization: Bearer "+token+"' https://theia.cnes.fr/atdistrib/resto2/collections/SENTINEL2/"+mFeature_id+"/download/?issuerId=theia";
    std::cout << get_product << std::endl;
    system(get_product.c_str());
}

void tuileS2::nettoyeArchive(){

    // nicolas semble dire qu'il y a 10 bandes+6 masques+1 MTD (tkpn) fichiers à garder, détection sur base de leurs noms

    if ( boost::filesystem::exists(archiveName ) ){
        ZipArchive zf(archiveName);
        zf.open(ZipArchive::WRITE);
        std::vector<ZipEntry> v=zf.getEntries();
        int c(0);
        for (ZipEntry zent: v){
            // std::cout << zent.getName() << std::endl;
            std::string Name=zent.getName();
            bool keep(0);
            for (std::string sub : tokeep ){
                std::size_t found = Name.find(sub);
                if (found!=std::string::npos){keep=1;}
            }
            if (!keep){
                c++;
                //std::cout << " je vais supprimer " << Name << std::endl;
                zf.deleteEntry(Name);
            }
        }
        //std::cout << " j'ai supprimé " << c << " éléments de l'archive " << std::endl;
        zf.close();
    }
}

void tuileS2::decompresse(){
    // il faut décompresser dans le directory
    std::string aCommand= std::string("unzip -a "+ archiveName + " -d "+wd+"raw/");
    //std::cout << aCommand << "\n";
    system(aCommand.c_str());
    // récupérer le nom du dossier qui n'as pas le mm nom que l'archive (ajout suffixe V1-1 ou V2-1)
    for(auto & p : boost::filesystem::recursive_directory_iterator(wd+"raw/")){
        std::string dir= p.path().parent_path().filename().string();
        std::size_t found = dir.find(mProd);
        if (found!=std::string::npos){
            //std::cout << "trouvé " <<  dir << std::endl;
            decompressDirName=dir;
            break;
        }
    }
    
}

bool tuileS2::pretraitementDone(){
    bool aRes(0);
    // récupérer le nom du dossier qui n'as pas le mm nom que l'archive (ajout suffixe V1-1 ou V2-1)
    for(auto & p : boost::filesystem::recursive_directory_iterator(wd+"raw/")){
        std::string dir= p.path().parent_path().filename().string();
        std::size_t found = dir.find(mProd);
        if (found!=std::string::npos){
            //std::cout << "trouvé " <<  dir << std::endl;
            decompressDirName=dir;
            aRes=1;
            break;
        }
    }
    return aRes;
}

void tuileS2::removeArchive(){
    boost::filesystem::remove(archiveName);
}


void tuileS2::readXML(){
    std::cout << " read XML " << std::endl;
    // récupérer le nom du dossier qui n'as pas le mm nom que l'archive (ajout suffixe V1-1 ou V2-1)
    // déjà fait précédemment
    /*for(auto & p : boost::filesystem::recursive_directory_iterator(wd)){
        std::string dir= p.path().parent_path().filename().string();
        std::size_t found = dir.find(mProd);
        if (found!=std::string::npos){
        //std::cout << "trouvé " <<  dir << std::endl;
        decompressDirName=dir;
        break;
        }
    }*/

    xml_document<> doc;
    xml_node<> * root_node;
    // Read the xml file into a vector
    std::string xmlFile(wd +decompressDirName +"/"+decompressDirName+"_MTD_ALL.xml");
    if ( boost::filesystem::exists(xmlFile ) ){
        std::ifstream theFile (xmlFile);
        std::vector<char> buffer((std::istreambuf_iterator<char>(theFile)), std::istreambuf_iterator<char>());
        buffer.push_back('\0');
        // Parse the buffer using the xml file parsing library into doc
        doc.parse<0>(&buffer[0]);
        // Find our root node
        root_node = doc.first_node("Muscate_Metadata_Document");
        xml_node<>* cur_node = root_node->first_node("Quality_Informations")->first_node("Current_Product")->first_node("Product_Quality_List")->first_node("Product_Quality")->first_node("Global_Index_List");
        for (xml_node<> * node = cur_node->first_node("QUALITY_INDEX"); node; node = node->next_sibling())
        {
            std::string n(node->first_attribute("name")->value());
            //std::cout << "n = " << n << std::endl;
            bool cast(1);
            std::string val(node->value());
            if (val=="false"){cast=0;}
            if (n=="CloudPercent"){
                mCloudCover=std::stoi(node->value()) ;
            } else if (n=="HotSpotDetected"){
                HotSpotDetected= cast ;
            } else if (n=="RainDetected"){
                RainDetected= cast ;
            } else if (n=="SnowPercent"){
                SnowPercent=std::stoi(node->value()) ;
            } else if (n=="SunGlintDetected"){
                SunGlintDetected= cast ;
            }
        }
        cur_node = root_node->first_node("Dataset_Identification")->first_node("GEOGRAPHICAL_ZONE");
        mTile=cur_node->value();
        cur_node = root_node->first_node("Product_Characteristics")->first_node("ORBIT_NUMBER");
        mOrbitN=std::stoi(cur_node->value());
        cur_node = root_node->first_node("Geoposition_Informations")->first_node("Coordinate_Reference_System")->first_node("Horizontal_Coordinate_System")->first_node("HORIZONTAL_CS_CODE");
        mEPSG=std::stoi(cur_node->value());
        cur_node = root_node->first_node("Product_Characteristics")->first_node("PLATFORM");;
        mSat=cur_node->value();
        cur_node = root_node->first_node("Product_Characteristics")->first_node("ACQUISITION_DATE");;
        mAcqDate=cur_node->value();
        cur_node = root_node->first_node("Geoposition_Informations")->first_node("Geopositioning")->first_node("Group_Geopositioning_List")->first_node("Group_Geopositioning")->first_node("ULX")  ;
        mULX=std::stoi(cur_node->value());
        cur_node = root_node->first_node("Geoposition_Informations")->first_node("Geopositioning")->first_node("Group_Geopositioning_List")->first_node("Group_Geopositioning")->first_node("ULY")  ;
        mULY=std::stoi(cur_node->value());

    } else {
        std::cout << " pas trouvé fichier " << xmlFile << std::endl;
    }
    std::cout << " done " << std::endl;
}

void tuileS2::wrap(){
    
    // check EPSG
}

year_month_day ymdFromString(std::string date){
    int d=std::stoi(date.substr(0,2));
    int m=std::stoi(date.substr(3,4));
    int y=std::stoi(date.substr(6,9));
    //std::cout << "y " << y << " m " << m << " d " << d << std::endl;
    year_month_day ymd(year{y},month{m},day{d});
    return ymd;
}

// Un masque de no_data, à résolution 10m (EDG_R1.tif) et 20m (EDG_R2.tif)
// un masque cloud cover mais beaucoup plus nuancé :
