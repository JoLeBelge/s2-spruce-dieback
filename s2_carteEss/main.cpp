#include "carteEss.h"

using namespace std::chrono;

std::string pathCompo("/home/lisein/Documents/carteApt/GIS/COMPO/202109/compo_all_sp10m.tif");
//std::string pathES("/home/lisein/Documents/carteApt/GIS/ES_EP/etatSanitaire_2021_masq_evol_BL72_tmp.tif");
std::string pathES("/media/gef/Data2/S2Scolyte/merge/etatSanitaire_2020_masq_evol_BL72.tif");
std::string pathRF;
extern bool mDebug;
extern std::string wdRacine;
extern std::string EP_mask_path;
extern std::string wd;
extern std::string globTuile;

extern std::string path_otb;

extern bool doAnaTS;
extern bool docleanTS1pos;

extern std::string globSuffix;

std::string d1("2016-01-01"),d2("2021-07-11");

bool compression(std::string aRasterName);
void decompress(std::string aRasterName);
std::string getNameTmp(std::string aIn);
int yMax(2018);

int main(int argc, char *argv[])
{
    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message - Attention, l'appli va lire le fichier XML s2_carteEss.xml pour définir les options de l'outils (tuiles et masque utilisé)")
            ("outils", po::value<int>()->required(), "choix de l'outil à utiliser (1: prepare point d'entrainement sur base carte compo et état sanitaire EP 2021, 2 ; calcule bande spectrale par trimestre, 3 ; applique une forêt aléatoire")
            ("xmlIn", po::value< std::string>()->required(), "Fichier xml contenant les paramètres pour l'application s2 carte Ess")
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

    int mode(vm["outils"].as<int>());
    // lecture des paramètres pour cet utilitaire
    readXML(vm["xmlIn"].as<std::string>());
    GDALAllRegister();
    wd=wdRacine+ globTuile +"/";
    switch (mode) {
    case 1:{
        std::cout << " echantillonnage stratifié pt d'entrainement " << std::endl;
        echantillonPts();
        break;
    }
    case 2:{
        std::cout << " calcul des valeurs spectrales trimestrielles moyennes (2016-2017) " << std::endl;
        cataloguePeriodPheno cata;
        cata.traitement();

        break;
    }
    case 3:{
        std::cout << " applique une RF " << std::endl;
        cataloguePeriodPheno cata;

        cata.applyRF(pathRF);

        break;
    }
    default:{
        std::cout << " mode outils incorrect " << std::endl;
    }
    }

    return 1;

}

void echantillonPts(){
    // lecture carte compo et ES2021
    //std::cout << "charge image " << pathCompo << std::endl;
    //Im2D_U_INT1 imCompo=Im2D_U_INT1::FromFileStd(pathCompo);
    //je pourrais utiliser directement les cartes de pourcentage de gha des essences pour avoir des points d'entrainements pour lesquels j'aurai une plus grande confiance
    // car le modèle de octobre 2021 montre les limites de la carte de composition, elles les amplifient même.
    std::vector<Im2D_U_INT1*> vImCompo;
    std::vector<std::string> vPaths{"AU", "BO","CH","DO", "EP", "HE", "MZ", "PE", "PI"};
    for (int i(1);i<10;i++){
        std::string aRName("/home/gef/Documents/input/compo/compo_"+std::to_string(i)+"_"+vPaths.at(i-1)+"10m.tif");
        if (compression(aRName)){decompress(aRName); aRName=getNameTmp(aRName);}
        // attention, il on fait les pourcentage de gha en résolution 2m!!! fait chier, faut repasser en 10 mètres sinon ç'est incompatible avec carte état sanitaire
        // décompressé ça fait 6gb par image, pas possible de tout charger en mémoire vive.
        Im2D_U_INT1 * im= new Im2D_U_INT1(Im2D_U_INT1::FromFileStd(aRName));
        vImCompo.push_back(im);
    }

    std::cout << "charge image " << pathES << std::endl;
    Im2D_U_INT1 imES=Im2D_U_INT1::FromFileStd(pathES);
    double xO(42250.0),yO(167700.0);
    int x0=((double) abs(xO-153038.0)/10.0);
    int y0=((double) abs(yO-136700.0)/10.0);

    Pt2di pt1(x0,y0);
    Pt2di pt2(x0+10000,y0+10000); // grosso modo, nombre de pixel dans une tuile
    int nbSub(1000);// 100 c'est pas du tout assez, je passe à 1000 par classe
    std::vector<mpt*> aRes;
    // selection de n pt pour chaque classe de la carte compo, sauf pour la classe épicéa pour laquelle on sera plus restrictif.
    std::vector<int> vCode{1,2,3,4,5,6,7,8,9};
    for (int es : vCode){
        Liste_Pts_INT2 pts(2);
        if (es!=5){
            //ce serai efficace si je faisait directement une sélection du rectangle correspondant  grosso modo à la tuile centrale de RW
            //ELISE_COPY(select(rectangle(pt1,pt2),imCompo.in()==es),0,pts);
            // attention car no data sont à 255 donc si je précise pas , il va les sélectionner le con
            ELISE_COPY(select(rectangle(pt1,pt2),vImCompo.at(es-1)->in()>80 && vImCompo.at(es-1)->in()<101),0,pts);
            // maintenant je tire au sort n pt et je récupère leur coordonnées.
            // attention par ailleurs, je ne veux que des points sur la tuile sentinel 2 la plus présente en Belgique! enfin j'imagine que c'est plus simple.
        } else {
            // épicéa sain uniquement. Mais du coup il n'y a pas les épicéa en bordure et autre.
            //ELISE_COPY(select(rectangle(pt1,pt2),imCompo.in()==es && imES.in()==1),0,pts);
            ELISE_COPY(select(rectangle(pt1,pt2),imES.in()==1),0,pts);

        }
        Im2D_INT2 Il3 =pts.image();
        INT2 ** d = Il3.data();
        INT nb= Il3.tx();
        std::cout << "Code " << es <<" , nombre de points " << nb << std::endl;

        // échantillonnage
        std::vector<int> s=subsample(nbSub,nb);
        //std::cout << "echantillonnage ; nombre de points " << s.size() << std::endl;
        INT2 * tx =d[0];
        INT2 * ty =d[1];
        for (int k : s){
            //std::cout << "pt id " << k << " , " << tx[k] << "," << ty[k] << ";"<< std::endl;
            // création d'un points ogr
            //pt p(tx[k],ty[k]);
            // attention, est-ce que les positions données par tx et ty commencent à 0 ou à 1. en plus, ça n'est peut-être pas la bonne lecture

            //aRes.push_back(new mpt(xO+10*tx[k]+5.0,yO-10*ty[k]-5.0,es)); // +5 et -5 pour être au centre du pixel
            aRes.push_back(new mpt(xO+10*(tx[k])+5,yO-10*(ty[k])-5,es)); // +5 et -5 pour être au centre du pixel
            //std::cout << " point " << k << " , " << std::to_string(pt.getX()) << " , " << std::to_string(pt.getY()) << std::endl;
        }
    }

    GDALAllRegister();
    // création du catalogue, sans traitement, sans debug, puis on utilise une fonction membre spécialisée qui retourne le résumé trimestriel pour chaque pt.
    doAnaTS=0;
    docleanTS1pos=0; // pour point en dehors du masque
    globTuile="T31UFR";
    wd=wdRacine+ globTuile +"/";

    /* utilisé dans la premiere étape de prototypage
    cataloguePeriodPheno cata;
    cata.openDS();
    for (mpt * p : aRes){
        //if (c==10){break;}
        p->transform(poCT);
        std::map<int,vector<double>> * r=cata.getMeanRadByTri1Pt(p->getX(),p->getY());

             out << p->Code() << ";"  << roundDouble(p->getX(),0) << ";" << roundDouble(p->getY(),0) ;
        for (auto kv : *r){
            for (double v : kv.second){
            out << ";"  << roundDouble(v) ;
            }
        }
        out <<"\n" ;
        }
        c++;
    out.close();
    cata.closeDS();*/

    cataloguePeriodPheno cata;

    cata.getMeanRadByTriMultiPt(aRes,"obs-compoS2-C.txt");


    /*if(cata.openDS4RF()){ // vérifie que les bandes rayonnement moyen par trimestre sont calculées. attention, elles doivent l'être pour toute la forêt wallone sur cette tuile du coup..

    // extraction pour chaque point de la valeur
    for (mpt * p : aRes){
        //if (c==10){break;}
        p->transform(poCT);
        //std::map<int,vector<double>> * r=cata.getMeanRadByTri1Pt(p->getX(),p->getY());
        std::map<int,vector<int>> * r=cata.getMeanRadByTri1Pt(p->getX(),p->getY());

            // out << p->Code() << ";"  << roundDouble(p->getX(),0) << ";" << roundDouble(p->getY(),0) ;
        out << p->Code() << ";"  << roundDouble(p->getX(),0) << ";" << roundDouble(p->getY(),0) ;
        for (auto kv : *r){
            for ( v : kv.second){
            out << ";"  << roundDouble(v) ;
            }
        }
        out <<"\n" ;
        }
        c++;
        */

}


// tirage aléatoire d'index
std::vector<int> subsample(int nbSub,int nbTot){
    std::vector<int> aRes;
    if (nbSub<nbTot){
        for (int i(0);i<nbSub;i++){
            int pos=(rand() % nbTot);
            aRes.push_back(pos);
            nbTot--;
        }
    }else { std::cout << "poblème dans subsample" << std::endl;}
    return aRes;
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
    xml_node<>* cur_node = root_node->first_node("pathCompo");
    if (cur_node){pathCompo=cur_node->value();} else {std::cout << " pas pathCompo dans fichier xml" << std::endl;}
    cur_node = root_node->first_node("pathES");
    if (cur_node){pathES=cur_node->value();} else {std::cout << " pas pathES dans fichier xml" << std::endl;}
    cur_node = root_node->first_node("wdRacine");
    if (cur_node){wdRacine=cur_node->value();} else {std::cout << " pas wdRacine dans fichier xml" << std::endl;}
    cur_node = root_node->first_node("path_otb");
    if (cur_node){path_otb=cur_node->value();} else {std::cout << " pas path_otb dans fichier xml" << std::endl;}
    cur_node = root_node->first_node("EP_mask_path");
    if (cur_node){EP_mask_path=cur_node->value();} else {std::cout << " pas EP_mask_path dans fichier xml" << std::endl;}
    cur_node = root_node->first_node("globTuile");
    if (cur_node){globTuile=cur_node->value();} else {std::cout << " pas globTuile dans fichier xml" << std::endl;}
    cur_node = root_node->first_node("yMax");
    if (cur_node){yMax=std::stoi(cur_node->value());} else {std::cout << " pas ymax dans fichier xml" << std::endl;}
    cur_node = root_node->first_node("debug");
    if (cur_node){mDebug=std::stoi(cur_node->value());} else {std::cout << " pas debug dans fichier xml" << std::endl;}
    cur_node = root_node->first_node("pathRF");
    if (cur_node){pathRF=cur_node->value();} else {std::cout << " pas pathRF dans fichier xml" << std::endl;}
    cur_node = root_node->first_node("suffix");
    if (cur_node){globSuffix=cur_node->value();
    globSuffix.erase(std::remove(globSuffix.begin(), globSuffix.end(), ' '), globSuffix.end());
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


bool compression(std::string aRasterName){
    GDALDataset *pIn= (GDALDataset*) GDALOpen(aRasterName.c_str(), GA_ReadOnly);
    bool aRes(0);
    const char *comp = "COMPRESSION=DEFLATE";
    if (strcmp(*pIn->GetMetadata("IMAGE_STRUCTURE"),comp)== 0){aRes=1;}
    GDALClose(pIn);
    return aRes;
}

void decompress(std::string aRasterName){
    if (!fs::exists(getNameTmp(aRasterName))){
        // on décompresse tout ça
        std::string aCommand= std::string("gdal_translate -co 'COMPRESS=NONE' "+ aRasterName +" "+getNameTmp(aRasterName)+" ");
        std::cout << aCommand << "\n";
        system(aCommand.c_str());
    }
}

std::string getNameTmp(std::string aIn){
    return aIn.substr(0,aIn.size()-4)+"_tmp.tif";
}
