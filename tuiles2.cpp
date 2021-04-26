#include "tuiles2.h"
int globSeuilCC(35);

std::string wd("/home/lisein/Documents/Scolyte/S2/test/");
std::string buildDir("/home/lisein/Documents/Scolyte/S2/build-s2_ts/");
std::string path_otb("/home/lisein/OTB/OTB-7.2.0-Linux64/bin/");
std::string EP_mask_path("/home/lisein/Documents/Scolyte/S2/input/");
std::string compr_otb="?&gdal:co:INTERLEAVE=BAND&gdal:co:TILED=YES&gdal:co:BIGTIFF=YES&gdal:co:COMPRESS=DEFLATE&gdal:co:ZLEVEL=9";
//std::string compr_otb="?&gdal:co:COMPRESS=DEFLATE";
//std::string compr_otb="";
bool overw(0);
std::vector<std::string> vBR2{"8A", "11", "12"};

std::vector<std::string> tokeep{"_FRE_",//Flat REflectance bands (not SRE)
                                "_MTD_ALL",//metedata xml
                                "_CLM_R1",//cloud and shodow mask 10m
                                "_MG2_R1",//geophysic mask with snow 10m
                                "_EDG_R1",//nodata mask 10m, edge mask
                                "_CLM_R2",//cloud and shodow mask 20m
                                "_MG2_R2",//geophysic mask with snow 20m
                                "_EDG_R2"//nodata mask 20m
                               };
// les longeurs d'onde, utilisé pour calcul des droites dans CRSWIR
double lNIRa(865);
double lSWIR1(1610);
double lSWIR2(2190);

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
                t.mDate=ymdFromString(t.mAcqDate);
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
                //t.catQual();

            }
        });
        // boucle sans multithread pour pour les traitements d'image
        std::for_each(
                    std::execution::seq,
                    mVProduts.begin(),
                    mVProduts.end(),
                    [](tuileS2& t)
        {
            // check que cloudcover est en dessous de notre seuil
            if (t.mCloudCover<globSeuilCC){
            t.masque();
            t.resample();
            t.computeCR();
            }
        });


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
        std::size_t found = dir.find(mProd.substr(0,mProd.size()-1));
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
        std::size_t found = dir.find(mProd.substr(0,mProd.size()-1));
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
    std::string xmlFile(wd +"raw/"+decompressDirName +"/"+decompressDirName+"_MTD_ALL.xml");
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
        cur_node = root_node->first_node("Geoposition_Informations")->first_node("Geopositioning")->first_node("Global_Geopositioning") ;
        //mULX=std::stoi(cur_node->value());
        //cur_node = root_node->first_node("Geoposition_Informations")->first_node("Geopositioning")->first_node("Group_Geopositioning_List")->first_node("Group_Geopositioning")->first_node("ULY")  ;
        //mULY=std::stoi(cur_node->value());
        for (xml_node<> * node = cur_node->first_node("Point"); node; node = node->next_sibling())
        {
            std::string n(node->first_attribute("name")->value());
            //std::cout << "n = " << n << std::endl;
            if (n=="upperLeft"){
                xml_node<> * nodeP=node->first_node("X");
                mXmin=std::stod(nodeP->value()) ;
                nodeP=node->first_node("Y");
                 mYmax=std::stod(nodeP->value()) ;
            } else if (n=="lowerRight"){
                xml_node<> * nodeP=node->first_node("X");
                mXmax=std::stod(nodeP->value()) ;
                nodeP=node->first_node("Y");
                mYmin=std::stod(nodeP->value()) ;
            }
        }

    } else {
        std::cout << " pas trouvé fichier " << xmlFile << std::endl;
    }
    interDirName=wd +"intermediate/"+decompressDirName +"/";
    boost::filesystem::path dir(interDirName);
    boost::filesystem::create_directory(dir);
    outputDirName=wd +"output/";
    boost::filesystem::path dir2(outputDirName);
    boost::filesystem::create_directory(dir2);
    std::cout << " done " << std::endl;
}

// applique le masque EP et le masque nuage et le masque edge (no data)
//==>new mask R1 & R2 with: 0=clear, 1=not clear (clouds/shadows/etc.), 2=blackfill (nodata at all)
void tuileS2::masque(){
     std::cout << "masque \n" << std::endl;
    for (int i(1) ; i<3 ; i++){
    std::string out=interDirName+"mask_R"+std::to_string(i)+".tif";
    // check que le fichier n'existe pas
    if (!boost::filesystem::exists(out) | overw){
    //im 1 = masque EP
    //im 2 = masque edge
    //im 3 masque cloud
    std::string clm(wd+"/raw/"+decompressDirName+"/MASKS/"+decompressDirName+"_CLM_R"+std::to_string(i)+".tif");
    std::string edg(wd+"/raw/"+decompressDirName+"/MASKS/"+decompressDirName+"_EDG_R"+std::to_string(i)+".tif");
    std::string exp("im1b1==1 and im2b1==0 and im3b1 ==0 ? 1 : im2b1 == 1 ? 3 : 2");
    // semble beaucoup plus lent avec les options de compression gdal
    std::string aCommand(path_otb+"otbcli_BandMathX -il "+EP_mask_path+"masque_EP_T31UFR_R"+std::to_string(i)+".tif "+edg+ " " + clm + " -out '"+ out + compr_otb+"' uint8 -exp '"+exp+"' -ram 4000 -progress 0");
    //std::string aCommand(path_otb+"otbcli_BandMathX -il "+EP_mask_path+"masque_EP_T31UFR_R"+std::to_string(i)+".tif "+edg+ " " + clm + " -out "+ out +" uint8 -exp '"+exp+"' -ram 4000 -progress 0");
    //std::cout << aCommand << std::endl;
    system(aCommand.c_str());
    }
    }
}

// resample des bandes 8A, 11 et 12 après leur avoir appliqué le masque
void tuileS2::resample(){
    std::cout << "resample \n" << std::endl;
  for (std::string b : vBR2){
    std::string out=interDirName+"band_R2_B"+b+"_mask_20m.tif";
    std::string out10m=interDirName+"band_R2_B"+b+"_mask_10m.tif";
    // check que le fichier n'existe pas
    if (!boost::filesystem::exists(out) | overw){
    std::string in=wd+"/raw/"+decompressDirName+"/"+decompressDirName+"_FRE_B"+b+".tif";
    std::string inMask=interDirName+"mask_R2.tif";
    std::string exp("im2b1==1 ? im1b1 : 0");
    // il faut faire attention à l'ordre des raster input car les no data de im1 sont utilisés par défaut dans la couche résultat. donc masque en 2ieme position
    std::string aCommand(path_otb+"otbcli_BandMathX -il "+in+" "+inMask+ " -out '"+ out + compr_otb+"' int16 -exp '"+exp+"' -ram 4000 -progress 0");
    std::cout << aCommand << std::endl;
    system(aCommand.c_str());
    aCommand="gdalwarp -tr 10 10 -r bilinear -overwrite -srcnodata 0 -co 'COMPRESS=DEFLATE' "+ out+ " " + out10m;
    std::cout << aCommand << std::endl;
    system(aCommand.c_str());
    }
    }
}

void tuileS2::computeCR(){
  std::cout << "computeCR \n" << std::endl;

    std::string out=outputDirName+getRasterCRName();
    // check que le fichier n'existe pas
    if (!boost::filesystem::exists(out) | overw){
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

    std::string NIRa=interDirName+"band_R2_B8A_mask_10m.tif";
    std::string SWIR1=interDirName+"band_R2_B11_mask_10m.tif";
    std::string SWIR2=interDirName+"band_R2_B12_mask_10m.tif";
    // si je tente de mettre sur 8 bit ; il me dis "error complex number". mais en double ça passe
    // j'ai des overflow mais pas beauoup. Le range de valeur attendu, c'est entre 0 et 2 (voir graph de Raphael) mais j'ai des valeurs qui dépassent 2.
    std::string exp("im2b1!=0 ? im2b1/(im1b1+(1610-865)* ((im3b1-im1b1)/(2190-865))) : 0");
    std::string aCommand(path_otb+"otbcli_BandMathX -il "+NIRa+" "+SWIR1+" "+SWIR2+" -out '"+ out + compr_otb+"' double -exp '"+exp+"' -ram 5000 -progress 0");
    //std::cout << aCommand << std::endl;
    system(aCommand.c_str());
    }
}

void tuileS2::wrap(){
    // check EPSG
}

std::string tuileS2::getRasterCRName(){
    return mAcqDate.substr(0,4)+mAcqDate.substr(5,2)+mAcqDate.substr(8,2)+ "_CRSWIR.tif";
}


year_month_day ymdFromString(std::string date){
    int d=std::stoi(date.substr(0,2));
    int m=std::stoi(date.substr(3,4));
    int y=std::stoi(date.substr(6,9));
    //std::cout << "y " << y << " m " << m << " d " << d << std::endl;
    year_month_day ymd(year{y},month{m},day{d});
    return ymd;
}
