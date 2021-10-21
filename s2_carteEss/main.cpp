#include "carteEss.h"

std::string pathCompo("/home/lisein/Documents/carteApt/GIS/COMPO/202109/compo_all_sp10m.tif");
std::string pathES("/home/lisein/Documents/carteApt/GIS/ES_EP/etatSanitaire_2021_masq_evol_BL72_tmp.tif");

extern std::string wdRacine;
extern std::string buildDir;
extern std::string EP_mask_path;
extern std::string wd;
extern std::string globTuile;

extern std::string path_otb;

extern bool doAnaTS;
extern bool docleanTS1pos;

int main(int argc, char *argv[])
{
    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("outils", po::value<int>()->required(), "choix de l'outil à utiliser (1: prepare point d'entrainement sur base carte compo et état sanitaire EP 2021, 2")
            ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    int mode(vm["outils"].as<int>());
    // lecture des paramètres pour cet utilitaire
    readXML("s2_carteEss.xml");
    switch (mode) {
    case 1:{
        std::cout << " echantillonnage stratifié pt d'entrainement " << std::endl;
        echantillonPts();
        break;
    }
    case 2:{
        std::cout << " calcul des valeurs spectrales trimestrielles moyennes (2016 - 2020) " << std::endl;

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
    std::cout << "charge image " << pathCompo << std::endl;
    Im2D_U_INT1 imCompo=Im2D_U_INT1::FromFileStd(pathCompo);
    std::cout << "charge image " << pathES << std::endl;
    Im2D_U_INT1 imES=Im2D_U_INT1::FromFileStd(pathES);
    double xO(42250.0),yO(167700.0);
    int x0=((double) abs(xO-153038.0)/10.0);
    int y0=((double) abs(yO-136888.0)/10.0);

    Pt2di pt1(x0,y0);
    Pt2di pt2(x0+10000,y0+10000); // grosso modo, nombre de pixel dans une tuile
    int nbSub(100);
    std::vector<mpt*> aRes;
    // selection de n pt pour chaque classe de la carte compo, sauf pour la classe épicéa pour laquelle on sera plus restrictif.
    std::vector<int> vCode{2,3,4,5,6,7,8,9};
    for (int es : vCode){
         Liste_Pts_INT2 pts(2);
        if (es!=5){
        //ce serai efficace si je faisait directement une sélection du rectangle correspondant  grosso modo à la tuile centrale de RW
        ELISE_COPY(select(rectangle(pt1,pt2),imCompo.in()==es),0,pts);
        // maintenant je tire au sort n pt et je récupère leur coordonnées.
        // attention par ailleurs, je ne veux que des points sur la tuile sentinel 2 la plus présente en Belgique! enfin j'imagine que c'est plus simple.
        } else {
            // épicéa sain uniquement
             ELISE_COPY(select(rectangle(pt1,pt2),imCompo.in()==es && imES.in()==1),0,pts);

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
             //for (INT k=0 ; k<nb ; k++){
             for (int k : s){
                 //std::cout << "pt id " << k << " , " << tx[k] << "," << ty[k] << ";"<< std::endl;
                 // création d'un points ogr
                 //pt p(tx[k],ty[k]);
                 aRes.push_back(new mpt(xO+10*tx[k]+5.0,yO-10*ty[k]-5.0,es)); // +5 et -5 pour être au centre du pixel
                 //std::cout << " point " << k << " , " << std::to_string(pt.getX()) << " , " << std::to_string(pt.getY()) << std::endl;
             }
    }

    GDALAllRegister();
    // création du catalogue, sans traitement, sans debug, puis on utilise une fonction membre spécialisée qui retourne le résumé trimestriel pour chaque pt.
    doAnaTS=0;
    docleanTS1pos=0; // pour point en dehors du masque
    globTuile="T31UFR";
    wd=wdRacine+ globTuile +"/";

    catalogue cata;
    cata.openDS();
    // crée un fichier txt pour l'export des résultats
    std::ofstream out;
    out.open("obs-compoS2-A.txt");
    std::vector<std::string> vB{"b2","b3","b4","b8","b11","b12"};
    out << "compo;X;Y";
    for (int tri(1);tri<5;tri++){
        for (std::string b : vB){
        out << ";" << b<< "_" <<std::to_string(tri) ;
        }
    }
    out <<"\n" ;
    int c(0);
    OGRSpatialReference oSourceSRS, oTargetSRS;
    oSourceSRS.importFromEPSG(31370);
    oTargetSRS.importFromEPSG(32631);
    OGRCoordinateTransformation * poCT = OGRCreateCoordinateTransformation( &oSourceSRS, &oTargetSRS );
//#pragma omp parallel num_threads(12) shared(cata,poCT)
            //{
//#pragma omp for
    for (mpt * p : aRes){
        //if (c==10){break;}
        p->transform(poCT);
        std::map<int,vector<double>> * r=cata.getMeanRadByTri1Pt(p->getX(),p->getY());
//#pragma omp critical
                               //{
             out << p->Code() << ";"  << roundDouble(p->getX(),0) << ";" << roundDouble(p->getY(),0) ;
        for (auto kv : *r){
            for (double v : kv.second){
            out << ";"  << roundDouble(v) ;
            }
        }
        out <<"\n" ;
        }
        c++;
   // }
   // }
    out.close();
    cata.closeDS();

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
    pathCompo=cur_node->value();
    cur_node = root_node->first_node("pathES");
    pathES=cur_node->value();
    cur_node = root_node->first_node("wdRacine");
    wdRacine=cur_node->value();
    cur_node = root_node->first_node("buildDir");
    buildDir=cur_node->value();
    cur_node = root_node->first_node("path_otb");
    path_otb=cur_node->value();
    cur_node = root_node->first_node("EP_mask_path");
    EP_mask_path=cur_node->value();

}

std::string roundDouble(double d, int precisionVal){
    std::string aRes("");
    if (precisionVal>0){aRes=std::to_string(d).substr(0, std::to_string(d).find(".") + precisionVal + 1);}
    else  {
        aRes=std::to_string(d+0.5).substr(0, std::to_string(d+0.5).find("."));}
    return aRes;
}
