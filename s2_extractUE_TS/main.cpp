
#include "s2_oakDieback.h"

using namespace std::chrono;


std::string pathES("/media/gef/Data2/S2Scolyte/merge/etatSanitaire_2020_masq_evol_BL72.tif");

extern bool mDebug;
extern std::string wdRacine;
extern std::string wd;
extern std::string globTuile;

extern std::string path_otb;

extern bool doAnaTS;
extern bool docleanTS1pos;

extern std::string globSuffix;

std::string d1("2016-01-01"),d2("2024-02-29");

// key ; nom de tuile. Val ; working directory
std::map<std::string,std::string> mapTuiles;
std::map<std::string,bool> mapDoTuiles;

int main(int argc, char *argv[])
{
    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message - Attention, l'appli va lire le fichier XML s2_carteEss.xml pour définir les options de l'outils (tuiles et fichier de points UE")
            //("outils", po::value<int>()->required(), "choix de l'outil à utiliser (1: prepare point d'entrainement sur base carte compo et état sanitaire EP 2021, 2 ; calcule bande spectrale par trimestre, 3 ; applique une forêt aléatoire, 4 merge results")
            ("xmlIn", po::value< std::string>()->required(), "Fichier xml contenant les paramètres pour l'application s2 oakDieback")
            ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    year_month_day today = year_month_day{floor<days>(system_clock::now())};
    d2 = format("%F",today);

    // lecture des paramètres pour cet utilitaire
    readXML(vm["xmlIn"].as<std::string>());
    GDALAllRegister();

    for (auto kv : mapTuiles){

        if (mapDoTuiles.find(kv.first)!=mapDoTuiles.end() && mapDoTuiles.at(kv.first)){
            std::string t=kv.first;
            wd=kv.second+ t +"/";
            globTuile=t;

            std::cout << "creation catalogue " << t << std::endl;
            catalogueExtract cata;
            cata.extractUE();

        }
    }

    return 1;

}

void readXML(std::string aXMLfile){
    // Read the xml file into a vector
    if ( !boost::filesystem::exists( aXMLfile ) ){ std::cout << " fichier " << aXMLfile << " n'existe pas " << std::endl;} else {
        std::cout << " read params " << std::endl;
        xml_document<> doc;
        xml_node<> * root_node;
        std::ifstream theFile (aXMLfile);
        std::vector<char> buffer((std::istreambuf_iterator<char>(theFile)), std::istreambuf_iterator<char>());
        buffer.push_back('\0');
        // Parse the buffer using the xml file parsing library into doc
        doc.parse<0>(&buffer[0]);
        // Find our root node
        root_node = doc.first_node("params");
        xml_node<>* cur_node = root_node->first_node("debug");
        if (cur_node){mDebug=std::stoi(cur_node->value());} else {std::cout << " pas debug dans fichier xml" << std::endl;}
        cur_node = root_node->first_node("suffix");
        if (cur_node){globSuffix=cur_node->value();
            globSuffix.erase(std::remove(globSuffix.begin(), globSuffix.end(), ' '), globSuffix.end());
            cur_node = root_node->first_node("Tuiles");
            // tuile et chemin d'accès; je dois mettre ça dans deux vecteur, je suppose. Ou un vecteur de tuple pour avoir les deux directement.
            for (xml_node<> * node = cur_node->first_node("Tuile"); node; node = node->next_sibling())
            {
                std::string n(node->first_attribute("name")->value());
                //std::cout << " tuile " << n <<  std::endl;
                xml_node<> * nodeP=node->first_node("wd");
                if (nodeP){mapTuiles.emplace(std::make_pair(n,nodeP->value()));} else {std::cout << " pas wd pour tuile " << n << " dans fichier xml" << std::endl;}
                nodeP=node->first_node("doTuile");
                if (nodeP){
                    mapDoTuiles.emplace(std::make_pair(n,std::stoi(nodeP->value())));
                } else {
                    mapDoTuiles.emplace(std::make_pair(n,true));
                }
            }
        } else {std::cout << " pas suffix dans fichier xml" << std::endl;}
    }
    std::cout << " done" << std::endl;
}

std::string roundDouble(double d, int precisionVal){
    std::string aRes("");
    if (precisionVal>0){aRes=std::to_string(d).substr(0, std::to_string(d).find(".") + precisionVal + 1);}
    else  {
        aRes=std::to_string(d+0.5).substr(0, std::to_string(d+0.5).find("."));}
    return aRes;
}

void catalogueExtract::extractUE(){
    std::cout << "extract UE "  << std::endl;

    std::vector<std::string> vBR2={"5","6","7","8A","11","12"};
    std::vector<std::string> vBR1{"2","3","4","8"};

    //std::vector<ptGDL> pts=readPtGDLFile("/home/gef/Documents/input/pts_GDLoakDieback_31UTMN.csv");
    std::vector<ptGDL> pts=readPtGDLFile("/home/gef/Documents/suiviACR/pts_acr2_31UTM.csv",0,1,2);

    for (ptGDL pt : pts){

        std::cout << "pt "<< pt.getID()  << std::endl;

        OGREnvelope env;
        env.MinX=pt.X()-100;
        env.MaxX=pt.X()+100;
        env.MinY=pt.Y()-100;
        env.MaxY=pt.Y()+100;

        //boost::filesystem::path dir2("/media/gef/Datas/oakDieback/");

        boost::filesystem::path dir2("/home/gef/Documents/suiviACR/s2/");
        boost::filesystem::create_directory(dir2);
        boost::filesystem::path dir3("//home/gef/Documents/suiviACR/s2/acr_"+pt.getID()+"/");
        boost::filesystem::create_directory(dir3);

        for (tuileS2OneDate * t : mVProduts){

            // test qu'on n'est pas en dehors du footprint de la pdv
            std::string edg1(wd+"/raw/"+t->decompressDirName+"/MASKS/"+t->decompressDirName+"_EDG_R2.tif");

            if (hasVal(edg1,env,0)){

                boost::filesystem::path dir4(dir3.generic_string() +"/"+t->getDate());
                boost::filesystem::create_directory(dir4);

                std::string xmlFile(wd +"raw/"+t->decompressDirName +"/"+t->decompressDirName+"_MTD_ALL.xml");
                std::string xmlFileCopy(dir4.generic_string() +"/MTD.xml");

                std::string aCommand="cp "+xmlFile+ " "+ xmlFileCopy;
                system(aCommand.c_str());
                //boost::filesystem::copy(xmlFile,xmlFileCopy);

                for (std::string b : vBR1){
                    // effectue le crop
                    std::string aout=dir4.generic_string() +"/FRE_B"+b+".tif";
                    cropIm(t->getRasterR1Name(b),aout,env);
                }
                for (std::string b : vBR2){
                    std::string aout=dir4.generic_string() +"/FRE_B"+b+".tif";
                    cropIm(t->getOriginalRasterR2Name(b),aout,env);
                }

                for (int i(1);i<3;i++){
                    std::string clm(wd+"/raw/"+t->decompressDirName+"/MASKS/"+t->decompressDirName+"_CLM_R"+std::to_string(i)+".tif");
                    std::string edg(wd+"/raw/"+t->decompressDirName+"/MASKS/"+t->decompressDirName+"_EDG_R"+std::to_string(i)+".tif");

                    std::string aout=dir4.generic_string() +"/CLM_R"+std::to_string(i)+".tif";
                    cropIm(clm,aout,env);
                    aout=dir4.generic_string() +"/EDG_R"+std::to_string(i)+".tif";
                    cropIm(edg,aout,env);
                }
            }

        }
    }


}

void cropIm(std::string inputRaster, std::string aOut, OGREnvelope ext){

    GDALAllRegister();
    if (exists(inputRaster)){

        const char *inputPath=inputRaster.c_str();
        const char *cropPath=aOut.c_str();
        GDALDataset *pInputRaster, *pCroppedRaster;
        GDALDriver *pDriver;
        const char *pszFormat = "GTiff";
        pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        pInputRaster = (GDALDataset*) GDALOpen(inputPath, GA_ReadOnly);

        double transform[6], tr1[6];
        pInputRaster->GetGeoTransform(transform);
        pInputRaster->GetGeoTransform(tr1);

        OGREnvelope extGlob=OGREnvelope();
        extGlob.MaxY=transform[3];
        extGlob.MinX=transform[0];
        extGlob.MinY=transform[3]+transform[5]*pInputRaster->GetRasterBand(1)->GetYSize();
        extGlob.MaxX=transform[0]+transform[1]*pInputRaster->GetRasterBand(1)->GetXSize();
        // garder l'intersect des 2 extend
        ext.Intersect(extGlob);
        //std::cout << ext.MinX << " , " << ext.MaxX << " , " << ext.MinY << " , " << ext.MaxY << " après intersect " << std::endl;

        double width((ext.MaxX-ext.MinX)), height((ext.MaxY-ext.MinY));

        //adjust top left coordinates
        transform[0] = ext.MinX;
        transform[3] = ext.MaxY;
        //determine dimensions of the new (cropped) raster in cells
        int xSize = round(width/transform[1]);
        int ySize = round(height/transform[1]);
        //std::cout << "xSize " << xSize << ", ySize " << ySize << std::endl;

        //create the new (cropped) dataset
        pCroppedRaster = pDriver->Create(cropPath, xSize, ySize, 1, pInputRaster->GetRasterBand(1)->GetRasterDataType(), NULL); //or something similar
        pCroppedRaster->SetProjection( pInputRaster->GetProjectionRef() );
        //pCroppedRaster->SetSpatialRef(pInputRaster->GetSpatialRef());
        pCroppedRaster->SetGeoTransform( transform );

        int xOffset=round((transform[0]-tr1[0])/tr1[1]);
        int yOffset=round((transform[3]-tr1[3])/tr1[5]);
        float *scanline;
        scanline = (float *) CPLMalloc( sizeof( float ) * xSize );
        // boucle sur chaque ligne
        for ( int row = 0; row < ySize; row++ )
        {
            // lecture
            pInputRaster->GetRasterBand(1)->RasterIO( GF_Read, xOffset, row+yOffset, xSize, 1, scanline, xSize,1, GDT_Float32, 0, 0 );
            // écriture
            pCroppedRaster->GetRasterBand(1)->RasterIO( GF_Write, 0, row, xSize,1, scanline, xSize, 1,GDT_Float32, 0, 0 );
        }
        CPLFree(scanline);
        if( pCroppedRaster != NULL ){GDALClose( (GDALDatasetH) pCroppedRaster );}

        GDALClose(pInputRaster);
    } else {
        std::cout << " attention, un des fichiers input n'existe pas : " << inputRaster << std::endl;
    }
}

bool hasVal(std::string inputRaster, OGREnvelope ext, int aVal){
    bool aRes(0);
    GDALAllRegister();
    if (exists(inputRaster)){
        const char *inputPath=inputRaster.c_str();
        GDALDataset *pInputRaster;
        GDALDriver *pDriver;
        const char *pszFormat = "GTiff";
        pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        pInputRaster = (GDALDataset*) GDALOpen(inputPath, GA_ReadOnly);

        double transform[6], tr1[6];
        pInputRaster->GetGeoTransform(transform);
        pInputRaster->GetGeoTransform(tr1);

        OGREnvelope extGlob=OGREnvelope();
        extGlob.MaxY=transform[3];
        extGlob.MinX=transform[0];
        extGlob.MinY=transform[3]+transform[5]*pInputRaster->GetRasterBand(1)->GetYSize();
        extGlob.MaxX=transform[0]+transform[1]*pInputRaster->GetRasterBand(1)->GetXSize();
        // garder l'intersect des 2 extend
        ext.Intersect(extGlob);
        //std::cout << ext.MinX << " , " << ext.MaxX << " , " << ext.MinY << " , " << ext.MaxY << " après intersect " << std::endl;

        double width((ext.MaxX-ext.MinX)), height((ext.MaxY-ext.MinY));

        //adjust top left coordinates
        transform[0] = ext.MinX;
        transform[3] = ext.MaxY;
        //determine dimensions of the new (cropped) raster in cells
        int xSize = round(width/transform[1]);
        int ySize = round(height/transform[1]);
        int xOffset=round((transform[0]-tr1[0])/tr1[1]);
        int yOffset=round((transform[3]-tr1[3])/tr1[5]);
        float *scanline;
        scanline = (float *) CPLMalloc( sizeof( float ) * xSize );
        // boucle sur chaque ligne
        for ( int row = 0; row < ySize; row++ )
        {
            // lecture
            pInputRaster->GetRasterBand(1)->RasterIO( GF_Read, xOffset, row+yOffset, xSize, 1, scanline, xSize,1, GDT_Float32, 0, 0 );
            for ( int col = 0; col < xSize; col++ ){
                if (scanline[col]==aVal){aRes=1; break;}
            }
        }
        CPLFree(scanline);

        GDALClose(pInputRaster);
    } else {
        std::cout << " attention, un des fichiers input n'existe pas : " << inputRaster << std::endl;
    }
    return aRes;
}

std::vector<ptGDL> readPtGDLFile(std::string aFilePath, int cId,int cX,int cY){
    std::cout << " readPtsFile" << std::endl;
    std::vector<ptGDL> aRes;
    std::vector<std::vector<std::string>> aVV = parseCSV2V(aFilePath, ',');

    bool firstL(1);// dégager les headers
    for (std::vector<std::string> l : aVV){
        if (!firstL){
            //if (l.size()!=3){std::cout << "Fichier avec position à tester ne semble pas avoir les 3 colonnes nécessaires ... " << std::endl;}
            //std::cout << " l at 2 = " << l.at(2) << " , l.at(3) " << l.at(3) << std::endl;
            std::string aId=l.at(cId);
            double aX=std::stod(l.at(cX));
            double aY=std::stod(l.at(cY));
            aRes.push_back(ptGDL(aId,aX,aY));
        } else { firstL=0;}
    }
    std::cout << " done " << std::endl;
    return aRes;
}
