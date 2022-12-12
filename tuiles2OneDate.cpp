#include "tuiles2OneDate.h"

std::string wdRacine("/home/lisein/Documents/Scolyte/S2/test/");
std::string wd("toto");
std::string path_otb("/home/lisein/OTB/OTB-7.3.0-Linux64/bin/");
std::string EP_mask_path("/home/lisein/Documents/Scolyte/S2/input/");
std::string compr_otb="?&gdal:co:INTERLEAVE=BAND&gdal:co:TILED=YES&gdal:co:BIGTIFF=YES&gdal:co:COMPRESS=DEFLATE&gdal:co:ZLEVEL=9";
int globSeuilCC(35);// 35 :je monte un peu ça car seulement 120 dates pour tuile UFR alors que 170 pour d'autres.
bool overw(0);

int year_analyse(666);

double Xdebug(0);
double Ydebug(0);
double seuilCR(1.7);
extern bool mDebug;

// scolytes; j'aimerai pouvoir avoir plusieurs masques , celui gha EP >50% et celui EP+DO > 50 % pour les besoins d'Adrien.
// donc j'identifie toutes mes couches intermédiaires et de résultats avec ce suffixe.
std::string globSuffix("");


bool doDelaisCoupe(0);
bool doFirstDateSco(0);


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

/*
 * valeurs de l'INRAE
double a1(0.551512);
double b1(0.062849);
double b2 (-0.120683);
double b3 (0.005509);
double b4 (-0.044525);
*/
// ajusté sur 300 pessières saines en Ardenne
double a1(0.58880089);
double b1(0.05537490);
double b2 (-0.10809739);
double b3 (-0.01737327);
double b4 (-0.02677137);

double cst(2.0*M_PI/365.25);


void tuileS2OneDate::download(){
    // création d'un token d'authentification dans le dossier ou on a lancer la commande
    std::string aCommand("curl -k -s -X POST  --data-urlencode 'ident=nicolas.latte@uliege.be' --data-urlencode 'pass=a1b2c3d4ULG_' https://theia.cnes.fr/atdistrib/services/authenticate/>token.json");
    //std::cout << aCommand << std::endl;

    system(aCommand.c_str());
    // lecture du token là ou on a lancé la commande
    std::ifstream in("token.json");
    std::ostringstream sstream;
    sstream << in.rdbuf();
    const std::string token(sstream.str());
    in.close();

    archiveName=wd+mProd+".zip";
    std::string get_product = "curl  -o "+archiveName+" -k -H 'Authorization: Bearer "+token+"' https://theia.cnes.fr/atdistrib/resto2/collections/SENTINEL2/"+mFeature_id+"/download/?issuerId=theia";
    if (mDebug){std::cout << get_product << std::endl;}
    system(get_product.c_str());
}

void tuileS2OneDate::nettoyeArchive(){

    // nicolas semble dire qu'il y a 10 bandes+6 masques+1 MTD (tkpn) fichiers à garder, détection sur base de leurs noms

    if ( boost::filesystem::exists(archiveName ) ){
        libzippp::ZipArchive zf(archiveName);
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

void tuileS2OneDate::decompresse(){
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

bool tuileS2OneDate::pretraitementDone(){
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

void tuileS2OneDate::removeArchive(){
    boost::filesystem::remove(archiveName);
}


void tuileS2OneDate::readXML(){
    if (mDebug){std::cout << " read XML .." ;}
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

void tuileS2OneDate::readXML(std::string aXMLfile){

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
    if (mDebug) { std::cout << " epsg " << mEPSG << std::endl;}

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

    mPtrDate=& mDate;

    //std::cout << " done " << std::endl;
}

// resample des bandes 8A, 11 et 12 après leur avoir appliqué le masque
void tuileS2OneDate::resample(){
    if (mDebug){std::cout << "resample ..";}
    for (std::string b : vBR2){
        std::string out=getRasterR2Name(b,2);;
        std::string out10m=getRasterR2Name(b);
        std::string in=getOriginalRasterR2Name(b);
        std::string inMask=getRasterMasqGenName(2);
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


/*void tuileS2::wrap(){
    // check EPSG
}*/

std::string tuileS2OneDate::getDate(){
    return mAcqDate.substr(0,4)+mAcqDate.substr(5,2)+mAcqDate.substr(8,2);
}

std::string tuileS2OneDate::getRasterMasqSecName() const{
    return interDirName + "mask"+mSuffix+"_R1_solnu.tif";
}
//suffix= type d'application, Sco, compo , EP, DO+EP
std::string tuileS2OneDate::getRasterMasqGenName(int resol){
    return interDirName + "mask_R"+std::to_string(resol)+mSuffix+".tif";
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
            if (l.size()!=3){std::cout << "Fichier avec position à tester ne semble pas avoir les 3 colonnes nécessaires ... " << std::endl;}
            //std::cout << " l at 2 = " << l.at(2) << " , l.at(3) " << l.at(3) << std::endl;
            int aId=std::stoi(l.at(0));
            double aX=std::stod(l.at(1));
            double aY=std::stod(l.at(2));
            aRes.push_back(pts(aId,aX,aY));
        } else { firstL=0;}
    }
    std::cout << " done " << std::endl;
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



double tuileS2OneDate::getDSVal(std::string bande,int aCol, int aRow){
    double aRes=0;
    if (vDS.find(bande)!=vDS.end()){
        GDALDataset * DSpt=vDS.at(bande);
        if( DSpt!= NULL && DSpt->GetRasterBand(1)->GetXSize() > aCol && DSpt->GetRasterBand(1)->GetYSize() > aRow && aRow >=0 && aCol >=0){
            // redéfini un scanpix pour pouvoir utiliser la fonction membre en parrallel sans conflict
            float * sp=(float *) CPLMalloc( sizeof( float ) * 1 );

            DSpt->GetRasterBand(1)->RasterIO( GF_Read, aCol, aRow, 1, 1, sp, 1,1, GDT_Float32, 0, 0 );
            aRes=sp[0];
        }
    }
    return aRes;
}

pts tuileS2OneDate::getUV(double x, double y){

    pts aRes(0,0);
        GDALDataset * DSpt=vDS.at("masqR1");

        if( DSpt != NULL){
            GDALRasterBand * mBand = DSpt->GetRasterBand( 1 );
            double transform[6];
            DSpt->GetGeoTransform(transform);
            double xOrigin = transform[0];
            double yOrigin = transform[3];
            double pixelWidth = transform[1];
            double pixelHeight = -transform[5];
            int col = int((x - xOrigin) / pixelWidth);
            int row = int((yOrigin - y ) / pixelHeight);
            aRes.setX(col);
            aRes.setY(row);
        }
    return aRes;
}

std::string tuileS2OneDate::getRasterR1Name(std::string numBand){
    return wd+"/raw/"+decompressDirName+"/"+decompressDirName+"_FRE_B"+numBand+".tif";
}

std::string tuileS2OneDate::getRasterR2Name(std::string numBand,int aR){
    std::string aRes=interDirName+"band_R2_B"+numBand+"_mask"+mSuffix+"_10m.tif";
    if (aR==2){aRes=interDirName+"band_R2_B"+numBand+"_mask"+mSuffix+"_20m.tif";}
    return aRes;
}

std::string tuileS2OneDate::getOriginalRasterR2Name(std::string numBand){
    return wd+"/raw/"+decompressDirName+"/"+decompressDirName+"_FRE_B"+numBand+".tif";
}


inline bool operator< (const tuileS2OneDate & t1, const tuileS2OneDate & t2)
{

    days diff=date::sys_days(t1.getymd())-date::sys_days(t2.getymd());
    if (diff.count()>0){
        return 1;
    } else {
        return 0;
    }
    //return  s1.getId() < s2.getId();
}

double getCRtheorique(year_month_day ymd){
    year_month_day startOfYear(year{ymd.year()},month{1},day{1});
    //daysSinceStartOfYear
    days daysSinceStartOfYear = date::sys_days(ymd) - date::sys_days(startOfYear);
    int x=daysSinceStartOfYear.count()+1;
    return a1 + b1*sin(cst*x)+ b2*cos(cst*x)+ b3*sin(cst*2*x)+ b4*cos(cst*2*x);
}
