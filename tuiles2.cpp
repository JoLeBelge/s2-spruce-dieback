#include "tuiles2.h"

std::string wdRacine("/home/lisein/Documents/Scolyte/S2/test/");
std::string wd;
std::string buildDir("/home/lisein/Documents/Scolyte/S2/build-s2_ts/");
std::string path_otb("/home/lisein/OTB/OTB-7.3.0-Linux64/bin/");
std::string EP_mask_path("/home/lisein/Documents/Scolyte/S2/input/");
//std::string iprfwFile("");
std::string compr_otb="?&gdal:co:INTERLEAVE=BAND&gdal:co:TILED=YES&gdal:co:BIGTIFF=YES&gdal:co:COMPRESS=DEFLATE&gdal:co:ZLEVEL=9";
int globSeuilCC(35);
bool overw(0);

int year_analyse(666);

double Xdebug(0);
double Ydebug(0);


std::string globTuile;

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

double a1(0.551512);
double b1(0.062849);
double b2 (-0.120683);
double b3 (0.005509);
double b4 (-0.044525);
double cst(2.0*M_PI/365.25);


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
    //std::cout << " read XML .." ;
    interDirName=wd +"intermediate/"+decompressDirName +"/";
    boost::filesystem::path dir(interDirName);
    boost::filesystem::create_directory(dir);
    outputDirName=wd +"output/";
    boost::filesystem::path dir2(outputDirName);
    boost::filesystem::create_directory(dir2);

    std::string xmlFileCopy(interDirName +"MTD.xml");
    std::string xmlFile(wd +"raw/"+decompressDirName +"/"+decompressDirName+"_MTD_ALL.xml");

    if ( boost::filesystem::exists(xmlFileCopy ) ){
        readXML(xmlFileCopy);
    } else {

        if ( boost::filesystem::exists(xmlFile ) ){
            readXML(xmlFile);
            // je copie  le xml dans intermediate de facon à pouvoir prendre les dossiers intermediate et output du cp traitements vers mon ordi et continuer les dvlpm sur ma machine
            boost::filesystem::copy(xmlFile,xmlFileCopy);
        }
    }
}

void tuileS2::readXML(std::string aXMLfile){

    // Read the xml file into a vector
    xml_document<> doc;
    xml_node<> * root_node;
    std::ifstream theFile (aXMLfile);
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
    cur_node = root_node->first_node("Dataset_Identification")->first_node("IDENTIFIER");
    mProd=cur_node->value();
    cur_node = root_node->first_node("Product_Characteristics")->first_node("ORBIT_NUMBER");
    mOrbitN=std::stoi(cur_node->value());
    cur_node = root_node->first_node("Product_Characteristics")->first_node("PRODUCTION_DATE");
    mProdDate=std::stoi(cur_node->value());
    cur_node = root_node->first_node("Geoposition_Informations")->first_node("Geopositioning")->first_node("Group_Geopositioning_List")->first_node("Group_Geopositioning")->first_node("NROWS");
    mXSize= std::stoi(cur_node->value());
    cur_node = root_node->first_node("Geoposition_Informations")->first_node("Geopositioning")->first_node("Group_Geopositioning_List")->first_node("Group_Geopositioning")->first_node("NCOLS");
    mYSize= std::stoi(cur_node->value());

    cur_node = root_node->first_node("Geoposition_Informations")->first_node("Coordinate_Reference_System")->first_node("Horizontal_Coordinate_System")->first_node("HORIZONTAL_CS_CODE");
    mEPSG=std::stoi(cur_node->value());
    cur_node = root_node->first_node("Product_Characteristics")->first_node("PLATFORM");;
    mSat=cur_node->value();
    cur_node = root_node->first_node("Product_Characteristics")->first_node("ACQUISITION_DATE");;
    mAcqDate=cur_node->value();
    mDate=ymdFromString(mAcqDate);

    cur_node = root_node->first_node("Geoposition_Informations")->first_node("Geopositioning")->first_node("Global_Geopositioning") ;

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

    //std::cout << " done " << std::endl;
}

// applique le masque EP et le masque nuage et le masque edge (no data)
//==>new mask R1 & R2 with: 0=clear, 1=not clear (clouds/shadows/etc.), 2=blackfill (nodata at all)
void tuileS2::masque(){
    std::cout << "masque .." ;
    for (int i(1) ; i<3 ; i++){
        std::string out=interDirName+"mask_R"+std::to_string(i)+".tif";
        std::string clm(wd+"/raw/"+decompressDirName+"/MASKS/"+decompressDirName+"_CLM_R"+std::to_string(i)+".tif");
        std::string edg(wd+"/raw/"+decompressDirName+"/MASKS/"+decompressDirName+"_EDG_R"+std::to_string(i)+".tif");
        // check que le fichier out n'existe pas
        if (boost::filesystem::exists(clm) && boost::filesystem::exists(edg)) {
        if ((!boost::filesystem::exists(out) | overw)) {
            //im 1 = masque EP
            //im 2 = masque edge
            //im 3 masque cloud

            std::string exp("im1b1==1 and im2b1==0 and im3b1 ==0 ? 1 : im2b1 == 1 ? 3 : 2");
            // semble beaucoup plus lent avec les options de compression gdal
            std::string aCommand(path_otb+"otbcli_BandMathX -il "+getNameMasqueEP(i)+" "+edg+ " " + clm + " -out '"+ out + compr_otb+"' uint8 -exp '"+exp+"' -ram 4000 -progress 0");
            //std::string aCommand(path_otb+"otbcli_BandMathX -il "+EP_mask_path+"masque_EP_T31UFR_R"+std::to_string(i)+".tif "+edg+ " " + clm + " -out "+ out +" uint8 -exp '"+exp+"' -ram 4000 -progress 0");
            //std::cout << aCommand << std::endl;
            system(aCommand.c_str());
        }
        } else { std::cout << "fichiers masques introuvables " << edg << " , " << clm << std::endl;}
    }
}

// resample des bandes 8A, 11 et 12 après leur avoir appliqué le masque
void tuileS2::resample(){
    std::cout << "resample ..";
    for (std::string b : vBR2){
        std::string out=interDirName+"band_R2_B"+b+"_mask_20m.tif";
        std::string out10m=interDirName+"band_R2_B"+b+"_mask_10m.tif";
        std::string in=wd+"/raw/"+decompressDirName+"/"+decompressDirName+"_FRE_B"+b+".tif";
        std::string inMask=interDirName+"mask_R2.tif";
        // check que le fichier n'existe pas
        if (boost::filesystem::exists(in) && boost::filesystem::exists(inMask)){
        if ((!boost::filesystem::exists(out) | overw)){

            std::string exp("im2b1==1 ? im1b1 : 0");
            // il faut faire attention à l'ordre des raster input car les no data de im1 sont utilisés par défaut dans la couche résultat. donc masque en 2ieme position
            std::string aCommand(path_otb+"otbcli_BandMathX -il "+in+" "+inMask+ " -out '"+ out + compr_otb+"' int16 -exp '"+exp+"' -ram 4000 -progress 0");
            std::cout << aCommand << std::endl;
            system(aCommand.c_str());
            aCommand="gdalwarp -tr 10 10 -r bilinear -overwrite -srcnodata 0 -co 'COMPRESS=DEFLATE' "+ out+ " " + out10m;
            std::cout << aCommand << std::endl;
            system(aCommand.c_str());
        }

         } else { std::cout << "fichiers introuvables " << in << " , " << inMask  << std::endl;}
    }
}

void tuileS2::computeCR(){
    std::cout << "computeCR .." ;

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
        std::string exp("im2b1!=0 ? im2b1/(im1b1+(1610-865)* ((im3b1-im1b1)/(2190-865))) : 0");
        std::string aCommand(path_otb+"otbcli_BandMathX -il "+NIRa+" "+SWIR1+" "+SWIR2+" -out '"+ out + compr_otb+"' double -exp '"+exp+"' -ram 5000 -progress 0");
        //std::cout << aCommand << std::endl;
        system(aCommand.c_str());
    }
    } else { std::cout << "fichiers introuvables " << NIRa << " , " << SWIR1 << " , " << SWIR2 << std::endl;}

    r_crswir = std::make_unique<rasterFiles>(out);
}

/*void tuileS2::wrap(){
    // check EPSG
}*/

std::string tuileS2::getRasterCRName(){
    return outputDirName + mAcqDate.substr(0,4)+mAcqDate.substr(5,2)+mAcqDate.substr(8,2)+ "_CRSWIR.tif";
}

std::string tuileS2::getRasterCRnormName(){
    return outputDirName +mAcqDate.substr(0,4)+mAcqDate.substr(5,2)+mAcqDate.substr(8,2)+ "_CRSWIRnorm.tif";
}

std::string tuileS2::getDate(){
    return mAcqDate.substr(0,4)+mAcqDate.substr(5,2)+mAcqDate.substr(8,2);
}

std::string tuileS2::getRasterMasqSecName(){
    return interDirName + "mask_R1_solnu.tif";
}
std::string tuileS2::getRasterMasqGenName(int resol=1){
    return interDirName + "mask_R"+std::to_string(resol)+".tif";
}

//https://gis.stackexchange.com/questions/233874/what-is-the-range-of-values-of-sentinel-2-level-2a-images
// surface _reflectance = DN/ 10 000
// varie souvent entre 0 et 1 mais on peut avoir des valeurs supérieures à 1
void tuileS2::masqueSpecifique(){
    std::cout << "detection sol nu .." ;
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
    if ((!boost::filesystem::exists(out) | overw) ){
        //im 1 = masque général
        //im 2 = swir1
        //im 3 = r
        //im 4 = v

        std::string exp("im1b1!=1 ? 0 : im2b1/10000<0.125 and ((im3b1+im4b1)/10000)<0.08 ? 1 : im1b1==1 and im2b1/10000.0>0.125 ? 2 : im1b1==1 and ((im3b1+im4b1)/10000.0)>0.08 ? 3 : 0");

        // valeur 1 : ok - 2 : sol nu détection swir - 3 sol nul détection r+v
        std::string aCommand(path_otb+"otbcli_BandMathX -il "+m+ " " + SWIR1 + " " + v + " " + r + " -out '"+ out + compr_otb+"' uint8 -exp '"+exp+"' -ram 5000 -progress 0");
        //std::cout << aCommand << std::endl;
        system(aCommand.c_str());
    }
    r_solnu = std::make_unique<rasterFiles>(out);
     } else { std::cout << "fichiers introuvables " << m << " , " << SWIR1 << " , " << v << std::endl;}

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

double tuileS2::getCRSWIR(pts & pt){
    return r_crswir->getValueDouble(pt.X(),pt.Y());
}

double tuileS2::getCRSWIRNorm(pts & pt){
    return r_crswirNorm->getValueDouble(pt.X(),pt.Y())*1.0/127.0;
}

int tuileS2::getMaskSolNu(pts & pt){
    return r_solnu->getValue(pt.X(),pt.Y());
}

year_month_day ymdFromString(std::string date){
    /*
    int d=std::stoi(date.substr(0,2));
    int m=std::stoi(date.substr(3,4));
    int y=std::stoi(date.substr(6,9));*/
    int d=std::stoi(date.substr(8,2));
    int m=std::stoi(date.substr(5,2));
    int y=std::stoi(date.substr(0,4));

    //std::cout << date <<  " ; y " << y << " m " << m << " d " << d << std::endl;
    year_month_day ymd(year{y},month{m},day{d});
    return ymd;
}

std::vector<pts> readPtsFile(std::string aFilePath){
    std::cout << " readPtsFile" << std::endl;
    std::vector<pts> aRes;
    std::vector<std::vector<std::string>> aVV = parseCSV2V(aFilePath, ',');

    bool firstL(1);// dégager les headers
    for (std::vector<std::string> l : aVV){
        if (!firstL){
            //std::cout << " l at 2 = " << l.at(2) << " , l.at(3) " << l.at(3) << std::endl;
            double aX=std::stod(l.at(4));
            double aY=std::stod(l.at(5));
            aRes.push_back(pts(aX,aY));
        } else { firstL=0;}
    }
    return aRes;
}

std::vector<std::vector<std::string>> parseCSV2V(std::string aFileIn, char aDelim){
    qi::rule<std::string::const_iterator, std::string()> quoted_string = '"' >> *(qi::char_ - '"') >> '"';
    qi::rule<std::string::const_iterator, std::string()> valid_characters = qi::char_ - '"' - aDelim;
    qi::rule<std::string::const_iterator, std::string()> item = *(quoted_string | valid_characters );
    qi::rule<std::string::const_iterator, std::vector<std::string>()> csv_parser = item % aDelim;

    std::vector<std::vector<std::string>> aOut;
    std::ifstream aFichier(aFileIn.c_str());
    if(aFichier)
    {
        std::string aLine;
        while(!aFichier.eof())
        {
            getline(aFichier,aLine,'\n');
            if(aLine.size() != 0)
            {
                std::string::const_iterator s_begin = aLine.begin();
                std::string::const_iterator s_end = aLine.end();
                std::vector<std::string> result;
                bool r = boost::spirit::qi::parse(s_begin, s_end, csv_parser, result);
                assert(r == true);
                assert(s_begin == s_end);
                // ajout d'un element au vecteur de vecteur
                aOut.push_back(result);
            }
        }
        aFichier.close();
    } else {
        std::cout << "file " << aFileIn << " not found " <<std::endl;
    }
    return aOut;
}

//crée une couche qui normalise le CR par le CR sensé être ok pour cette date ; sera plus facile à manipuler
void tuileS2::normaliseCR(){
    std::cout << "normalize CR " ;

    std::string out=getRasterCRnormName();
    std::string in=getRasterCRName();
    // check que le fichier n'existe pas
    if (boost::filesystem::exists(in)){
    if (!boost::filesystem::exists(out) | overw){

        // CRSWIRnorm = CRSWIR/theoreticalCR(t)


        double cr = getCRth();

        // gain de 1/127, comme cela je stoque des valeurs de 0 à 2 sur du 8 bits
        std::string exp("im1b1!=0 ? 127*im1b1/"+std::to_string(cr)+" : 0");
        std::string aCommand(path_otb+"otbcli_BandMathX -il "+in+" -out '"+ out + compr_otb+"' uint8 -exp '"+exp+"' -ram 5000 -progress 0");
        //std::cout << aCommand << std::endl;
        system(aCommand.c_str());
    }
    } else { std::cout << "fichier introuvable " << in << std::endl;}
    r_crswirNorm = std::make_unique<rasterFiles>(out);

}

bool tuileS2::openDS(){
    //std::cout << "ouverture dataset pour " << mProd << std::endl;
    //std::cout << "ouverture dataset pour " << getRasterCRnormName() << std::endl;
    bool aRes(0);
    if (exists(getRasterCRnormName()) && exists(getRasterMasqSecName())){
        //std::cout << "open " << getRasterCRnormName() << " \n" << " et " << getRasterMasqSecName() << std::endl;
        mDScrnom= (GDALDataset *) GDALOpen( getRasterCRnormName().c_str(), GA_ReadOnly );
        mDSsolnu= (GDALDataset *) GDALOpen( getRasterMasqSecName().c_str(), GA_ReadOnly );
        /* fonctionne très bien mais année par année, si plus de donnée, sature la mémoire vive donc pas si pratique...
        const char *pszFormat = "MEM";
        GDALDriver *pDriver= GetGDALDriverManager()->GetDriverByName(pszFormat);
    GDALDataset * cr=(GDALDataset *) GDALOpen( getRasterCRnormName().c_str(), GA_ReadOnly );
    GDALDataset * sol=(GDALDataset *) GDALOpen( getRasterCRnormName().c_str(), GA_ReadOnly );
    const char * ch="immem";
    mDSsolnu= pDriver->CreateCopy( ch,sol,FALSE, NULL,NULL, NULL );
    mDScrnom= pDriver->CreateCopy( ch,cr,FALSE, NULL,NULL, NULL );
    GDALClose( cr );
    GDALClose( sol );
    */

        if( mDSsolnu == NULL |  mDScrnom == NULL)
        {
            std::cout << "dataset pas ouvert correctement pour tuile " << mProd << std::endl;
            closeDS();
        } else {
            aRes=1;
            scanPix=(float *) CPLMalloc( sizeof( float ) * 1 );
            scanLineSolNu=(float *) CPLMalloc( sizeof( float ) * mYSize );
            scanLineCR=(float *) CPLMalloc( sizeof( float ) * mYSize );
        }
    } else {
        std::cout << "ne trouve pas " << getRasterCRnormName() << " \n" << " ou bien " << getRasterMasqSecName() << std::endl;
    }
    return aRes;
}
void tuileS2::closeDS(){
    if (mDScrnom != NULL){GDALClose( mDScrnom );}
    if (mDSsolnu != NULL){GDALClose( mDSsolnu );}

    if (scanPix!=NULL){ CPLFree(scanPix);}
    if (scanLineSolNu!=NULL){ CPLFree(scanLineSolNu);}
    if (scanLineCR!=NULL){ CPLFree(scanLineCR);}
}


int tuileS2::getMasqVal(int aCol,int aRow){
    int aRes=0;
    if( mDSsolnu != NULL && mDSsolnu->GetRasterBand(1)->GetXSize() > aCol && mDSsolnu->GetRasterBand(1)->GetYSize() > aRow && aRow >=0 && aCol >=0){
        mDSsolnu->GetRasterBand(1)->RasterIO( GF_Read, aCol, aRow, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
        aRes=scanPix[0];
    }
    return aRes;
}
double tuileS2::getCRnormVal(int aCol, int aRow){
    double aRes=0.0;
    if( mDScrnom != NULL && mDScrnom->GetRasterBand(1)->GetXSize() > aCol && mDScrnom->GetRasterBand(1)->GetYSize() > aRow && aRow >=0 && aCol >=0){
        mDScrnom->GetRasterBand(1)->RasterIO( GF_Read, aCol, aRow, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
        // applique le gain
        aRes=scanPix[0]*1.0/127.0;
    }
    return aRes;
}


void tuileS2::readCRnormLine(int aRow){

    if( mDScrnom != NULL && mDScrnom->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){
        mDScrnom->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, mXSize, 1, scanLineCR, mXSize,1, GDT_Float32, 0, 0 );
    }
}
void tuileS2::readMasqLine(int aRow){
    if( mDSsolnu != NULL && mDSsolnu->GetRasterBand(1)->GetYSize() > aRow && aRow >=0){
        mDSsolnu->GetRasterBand(1)->RasterIO( GF_Read, 0, aRow, mXSize, 1, scanLineSolNu, mXSize,1, GDT_Float32, 0, 0 );
    }
}
double tuileS2::getCRnormVal(int aCol){
    // applique le gain
    return scanLineCR[aCol]*1.0/127.0;
}
int tuileS2::getMasqVal(int aCol){
    return scanLineSolNu[aCol];
}

std::string tuileS2::getRasterR1Name(std::string numBand){
    return wd+"/raw/"+decompressDirName+"/"+decompressDirName+"_FRE_B"+numBand+".tif";
}

std::string tuileS2::getRasterR2Name(std::string numBand){
    return interDirName+"band_R2_B"+numBand+"_mask_10m.tif";
}




double getCRtheorique(year_month_day ymd){
    year_month_day startOfYear(year{ymd.year()},month{1},day{1});
    //daysSinceStartOfYear
    days daysSinceStartOfYear = date::sys_days(ymd) - date::sys_days(startOfYear);
    int x=daysSinceStartOfYear.count()+1;
    return a1 + b1*sin(cst*x)+ b2*cos(cst*x)+ b3*sin(cst*2*x)+ b4*cos(cst*2*x);
}

inline bool operator< (const tuileS2 & t1, const tuileS2 & t2)
{

    days diff=date::sys_days(t1.getymd())-date::sys_days(t2.getymd());
    if (diff.count()>0){
        return 1;
    } else {
        return 0;
    }
    //return  s1.getId() < s2.getId();
}

std::string getNameMasqueEP(int i){
    return wd+"input/masque_EP_"+globTuile+"_R"+std::to_string(i)+".tif";
    //return EP_mask_path+"masque_EP_T31UFR_R"+std::to_string(i)+"_80pct.tif";
}
