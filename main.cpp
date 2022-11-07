#include <iostream>
#include "cataloguesco.h"
#include "reportings2.h"
#include <execution>
#include <boost/filesystem.hpp>
#include "date.h"
using namespace std;
using namespace std::chrono;

extern std::string wdRacine;// j'en fait un deuxieme car je vais changer wd dans la boucle sur la liste des tuiles
extern std::string wd;
extern std::string path_otb;
extern std::string EP_mask_path;
extern std::string globTuile;
extern std::string globResXYTest;
extern double seuilCR;
extern bool docleanTS1pos;

bool mergeEtatSan(0);
extern bool doAnaTS;

extern int year_analyse;

extern double Xdebug;
extern double Ydebug;
extern bool debugDetail;

extern bool overw;
extern int nbDaysStress;
extern bool mDebug;// plus bavard avec cette option
extern std::string XYtestFile;

// key ; nom de tuile. Val ; working directory
std::map<std::string,std::string> mapTuiles;
std::map<std::string,bool> mapDoTuiles;
extern std::string globSuffix;
int catalogueMode(1);

extern bool doDelaisCoupe;
extern bool doFirstDateSco;

std::string d1("2016-01-01"),d2("2021-07-11");

int mergeEPSG(0);

void readXML(std::string aXMLfile);

std::string getBaseName(int mode, string aTuile);

int main(int argc, char *argv[])
{

    year_month_day today = year_month_day{floor<days>(system_clock::now())};
    d2 = format("%F",today);
    //std::cout << "d2 " << d2 << std::endl;
    char userName[20];
    getlogin_r(userName,sizeof(userName));
    std::string s(userName);
    if (s=="lisein"){

    } else {
        wdRacine="/media/gef/Data2/S2Scolyte/";
        path_otb="/home/gef/Documents/OTB-7.2.0-Linux64/bin/";
        EP_mask_path="/home/gef/Documents/input/";
    }

    GDALAllRegister();

    system("ulimit -n 65536");
    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "affiche l'aide ci-dessous")
            ("xmlIn", po::value< std::string>()->required(), "Fichier xml contenant les paramètres pour l'application s2 time serie")
            ("dates",  po::value<std::vector<std::string> >()->multitoken(), "date 1 et date 2 pour le téléchargement de la série temporelle. Default; début 2016-01-01 et fin =date d'aujourd'hui" )
            ("srCR", po::value<double>(), "seuil ratio CRswir à partir duquel on détecte un stress. Defaut 1.4")
            ("nbJourStress", po::value<int>(), "nombre du jours seuil à partir dusquel on n'envisage plus un retour à la normal pour un stress temporaire pronlongé. Default 90")
            ("XYtest", po::value<std::vector<double> >()->multitoken(), "coordonnée d'un point pour lequel on va faire tourner l'analyse temporelle avec de nombreuses information écrite dans la console qui serviront à améliorer les filtres sur les valeurs d'état sanitaire de la TS. Attention, EPSG est 32631 (UTM 31N)")
            ("XYtestIn", po::value< std::string>(), "Fichier texte séparateur virgule avec col 1 = id col 2=X et col3=3, epsg 32631 (UTM 31N), on effectue l'analyse en mode Test sur tout ces points là")
            ("XYtestOut", po::value< std::string>(), "fichier texte ou sauver les résultats de XYTest sur un point.")
            ("testDetail", po::value<bool>(), "pour le test sur une position, affichage ou non des valeurs de toutes les bandes ou juste les valeurs d'état, def true")
            ("testClean", po::value<bool>(), "pour le test sur une position, nettoyage ou pas, def true")
            ("mergeES", po::value<bool>(), "fusionne les cartes d'état sanitaire des différentes tuiles")
            ("anaTS", po::value<bool>(), "effectue l'analyse sur la série temporelle, defaut true mais si on veux faire un merge des cartes Etat san sans tout recalculer -->mettre à false")
            ("mode", po::value<int>(), "mode 1= pas de téléchargement de nouvelle tuile. Mode 2 = téléchargement")
            ("debug", po::value<bool>(), "si true, le logiciel est plus bavard, ça aide pour débugger")
            ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    // lis les options des paramètres qui sont dans un fichier xml
    // attention, il faut gerer le fait qu'on peut rentrer des options à la fois via xml, à la fois via ligne de commande-> eviter les conflicts.
    //il faut également pouvoir changer le chemin d'accès pour chaque tuiles, vu que je ne peux plus les laisser sur un seul et mm disque dur car cela dépassera les 2TB

    readXML(vm["xmlIn"].as<std::string>());
    // overwrite de ces deux arguments
    if (vm.count("anaTS")) {doAnaTS=vm["anaTS"].as<bool>();}
    if (vm.count("mergeES")) {mergeEtatSan=vm["mergeES"].as<bool>();}
    if (vm.count("mode")) {catalogueMode=vm["mode"].as<int>();}


    std::vector<std::string> aVTuiles;
    if (!vm["tuile"].empty() | mapTuiles.size()>0){

        if (vm.count("nbJourStress")) {nbDaysStress=vm["nbJourStress"].as<int>();}

        if (vm.count("Overwrite")) {overw=vm["Overwrite"].as<bool>();}
        if (vm.count("testDetail")) {debugDetail=vm["testDetail"].as<bool>();}
        if (vm.count("testClean")) {docleanTS1pos=vm["testClean"].as<bool>();}
        if (vm.count("srCR")) {seuilCR=vm["srCR"].as<double>();}
        if (vm.count("XYtestOut")) {globResXYTest=vm["XYtestOut"].as<std::string>();}
        if (vm.count("XYtestIn")) {XYtestFile=vm["XYtestIn"].as<std::string>();}
        std::vector<double> opts;
        if (!vm["XYtest"].empty() && (opts = vm["XYtest"].as<vector<double> >()).size() == 2) {
            std::cout << "Test pour une position donnee "<<std::endl;
            Xdebug=opts.at(0);
            Ydebug=opts.at(1);
        }

        // paramètres issu du xml
        if(mapTuiles.size()==0){
            aVTuiles = vm["tuile"].as<vector<std::string> >();

            // toutes les tuiles qui sont déja téléchargées;
            if (aVTuiles.at(0)=="all"){
                aVTuiles.clear();
                for(auto & p : boost::filesystem::directory_iterator(wdRacine)){
                    std::string aTuile=p.path().filename().string();

                    if (aTuile.substr(0,3)=="T31"){
                        aVTuiles.push_back(aTuile);
                        std::cout << " tuile " << aTuile << " ajoutée." << std::endl;
                    }
                }
            }


            if (vm.count("catalogue")) {catalogueMode=vm["catalogue"].as<int>();}
            if (vm.count("annee")) {year_analyse=vm["annee"].as<int>();}
            if (vm.count("debug")) {mDebug=vm["debug"].as<bool>();}




            std::vector<std::string> dates;
            if (!vm["dates"].empty() && (dates = vm["dates"].as<vector<std::string> >()).size() == 2) {
                std::cout << "dates de début et de fin entrées par l'utilisateur "<<std::endl;
                d1=dates.at(0);
                d2=dates.at(1);
            }

            //
            for (std::string t : aVTuiles){
                if (t=="T32ULU"){
                    mapTuiles.emplace(std::make_pair(t,"/home/gef/Documents/"));
                }
                else {  mapTuiles.emplace(std::make_pair(t,wdRacine));}

                mapDoTuiles.emplace(std::make_pair(t,1));
            }


        }

        if (!mergeEtatSan){

            reportingS2 r;
        for (auto kv : mapTuiles){

            if (mapDoTuiles.find(kv.first)!=mapDoTuiles.end() && mapDoTuiles.at(kv.first)){
                std::string t=kv.first;
                wd=kv.second+ t +"/";
                globTuile=t;
                std::cout << " tuile " <<   globTuile << ", wd " << wd << std::endl;

                // création du wd si nécessaire
                boost::filesystem::path dir(wd);
                if(!boost::filesystem::exists(dir)){boost::filesystem::create_directory(dir);}
                boost::filesystem::path dir2(wd+"raw/");
                boost::filesystem::create_directory(dir2);
                boost::filesystem::path dir3(wd+"intermediate/");
                boost::filesystem::create_directory(dir3);
                boost::filesystem::path dir4(wd+"output/");
                boost::filesystem::create_directory(dir4);
                boost::filesystem::path dir5(wd+"input/");
                boost::filesystem::create_directory(dir5);


                switch (catalogueMode) {
                case 1:{
                    std::cout << "----------- Création du catalogue pour tuile " << t  <<std::endl;
                    // lancer la requete theia avant de créer le catalogue
                    std::string aCommand="curl -k  -o "+wd+"search.json 'https://theia.cnes.fr/atdistrib/resto2/api/collections/SENTINEL2/search.json?completionDate="+d2+"&startDate="+d1+"&maxRecords=500&location="+globTuile+"&processingLevel=LEVEL2A'";
                    if (mDebug){std::cout << aCommand << "\n" << std::endl;}
                    system(aCommand.c_str());
                    std::string inputJson=wd+"search.json";
                    catalogueSco cata(inputJson);
                    cata.traitement();
                    std::cout << "Tuile " << t << " faite" <<std::endl;
                    r.add1Tuile(&cata);
                    break;
                }
                case 2:{
                    // ne pas mettre de parenthèse !.
                    std::cout <<"----------- Création du catalogue pour tuile " << t <<std::endl;
                    catalogueSco cata;
                    cata.traitement();
                    std::cout << "Tuile " << t << " faite" <<std::endl;
                    r.add1Tuile(&cata);
                    break;
                }
                }
            }
        }
         r.genReport("toto.txt");
        }


        // fin du traitement de chacune des tuiles. maintenant on peux fusionner les tuiles si on le souhaite
        if (mergeEtatSan){

            std::cout << " merge Etat sanitaire (et carte firstDate / delaisCoupe si il y en a)" << std::endl;

            for (int x(1);x<4;x++){

                std::cout << " Mode " << x << std::endl;

                // un peu moche mais bon
                int add(0);
                if(x>1){add=1;}
                std::map<int, std::vector<std::string>> aMES;

                for (auto kv : mapTuiles){

                    std::string t=kv.first;
                    std::string tDir=kv.second+ t +"/";
                    std::cout << " tuile " << tDir << std::endl;

                    std::string aBaseName=getBaseName(x,t);
                    if (mDebug){std::cout << "baseName " << aBaseName << std::endl;}
                    boost::filesystem::directory_iterator end_itr;

                    // cycle through the directory
                    for (boost::filesystem::directory_iterator itr(tDir); itr != end_itr; ++itr)
                    {
                        if (itr->path().filename().extension().string()==".tif"){
                            if (boost::filesystem::is_regular_file(itr->path()) && itr->path().filename().string().substr(0,aBaseName.size())==aBaseName && itr->path().filename().string().size()==aBaseName.size()+add+4+4) {
                                std::string  etatSan= itr->path().string();
                                if (mDebug){std::cout << etatSan << std::endl;}


                                int  year=stoi(itr->path().filename().string().substr(aBaseName.size()+add,4));
                                //if (mDebug){std::cout << "year " << year << std::endl;}
                                if (aMES.find(year)!=aMES.end()){
                                    aMES.at(year).push_back(etatSan);
                                } else {
                                    std::vector<std::string> aV;
                                    aV.push_back(etatSan);
                                    aMES.emplace(std::make_pair(year,aV));
                                }
                            }
                        }
                    }
                }

                // il faut également faire un check des src qui ne sont pas les même pour toutes les tuiles depuis que je travaille sur le Grand-Est.
                // effectue le merge par année
                boost::filesystem::path dir(wdRacine+"merge/");
                boost::filesystem::path dir2(wdRacine+"merge/tmp/");
                boost::filesystem::create_directory(dir);
                boost::filesystem::create_directory(dir2);

                // vérifie le src et si pas celui qu'on souhaite, on reprojette avant merge
                std::cout << " Vérifie la cohérence des SRC" << std::endl;
                for (auto & kv : aMES){
                    int k(0);
                    for (std::string f : kv.second){
                        GDALDataset * DS= (GDALDataset *) GDALOpen(f.c_str(), GA_ReadOnly );
                        OGRSpatialReference osr;
                        osr.importFromWkt(DS->GetProjectionRef());
                        if (OGRERR_NONE!=osr.AutoIdentifyEPSG()){std::cout << " auto i epsg failed" << std::endl;}
                        int EPSG=std::stoi(osr.GetAuthorityCode("PROJCS"));
                        GDALClose(DS);
                        if (EPSG!=mergeEPSG){
                            boost::filesystem::path p(f);
                            std::string out= wdRacine+"merge/tmp/"+p.filename().string();

                            std::string aCommand="gdalwarp -t_srs EPSG:"+std::to_string(mergeEPSG)+" -ot Byte -overwrite -dstnodata 255 -tr 10 10 "+ f+ " "+ out;
                            std::cout << aCommand << std::endl;
                            if (!boost::filesystem::exists(out) | overw){
                            system(aCommand.c_str());
                            }
                            aMES.at(kv.first).at(k)=out;
                        }
                        k++;
                    }
                }


                for (auto & kv : aMES){
                    std::cout << " fusion des cartes d'état sanitaire pour l'année " << kv.first << std::endl;
                    std::string aBaseResult("etatSanitaire");
                    switch (x) {
                    case 2:
                        aBaseResult="FirstDateSco";
                        break;
                    case 3:
                        aBaseResult="delaisCoupe";
                    default:
                        break;
                    }
                    // crée un fichier txt qui liste les inputs - pour gdal merge
                    std::ofstream out;
                    std::string aMergeFile=dir.string()+"merge_"+aBaseResult+"_"+std::to_string(kv.first)+".txt";
                    out.open(aMergeFile);
                    for (std::string f : kv.second){
                        out << f;
                        out << "\n";
                    }
                    out.close();
                    // attention, le merge ne fonction QUE si les nd sont définie dans les raster d'entrée. l'argument -n 255 ne fonctionne pas si pas déjà écrit dans les MTD des raster. voir exemple GE compo qui fusionne très bien.
                    // donc fonctionne que si toutes les carte ES ont été reprojetée car durant l'appel à gdal je spécifie -dstnodata 255
                    std::string aCommand("gdal_merge.py -n 0 -n 255 -o "+dir.string()+aBaseResult+"_"+std::to_string(kv.first)+globSuffix+".tif -of GTiff -co 'COMPRESS=DEFLATE' -v --optfile "+aMergeFile);
                    std::cout << aCommand << std::endl;
                    system(aCommand.c_str());
                }
            }

        }


    }

    return 0;
}


void readXML(std::string aXMLfile){
    // Read the xml
    if ( !boost::filesystem::exists( aXMLfile ) ){ std::cout << " fichier " << aXMLfile << " n'existe pas " << std::endl;} else {
        std::cout << " read params " << std::endl;
        xml_document<> doc;
        xml_node<> * root_node;
        xml_node<> * cur_node;
        std::ifstream theFile (aXMLfile);
        std::vector<char> buffer((std::istreambuf_iterator<char>(theFile)), std::istreambuf_iterator<char>());
        buffer.push_back('\0');
        // Parse the buffer using the xml file parsing library into doc
        doc.parse<0>(&buffer[0]);
        // Find our root node
        root_node = doc.first_node("params");

        cur_node = root_node->first_node("path_otb");
        if (cur_node){
            path_otb=cur_node->value();} else {std::cout << " pas path_otb dans fichier xml" << std::endl;}
        cur_node = root_node->first_node("EP_mask_path");
        if (cur_node){EP_mask_path=cur_node->value();} else {std::cout << " pas EP_mask_path dans fichier xml" << std::endl;}
        cur_node = root_node->first_node("suffix");
        if (cur_node){globSuffix=cur_node->value();} else {std::cout << " pas suffix dans fichier xml" << std::endl;}
        cur_node = root_node->first_node("catalogueMode");
        if (cur_node){catalogueMode=std::stoi(cur_node->value());} else {std::cout << " pas catalogueMode dans fichier xml" << std::endl;}
        cur_node = root_node->first_node("debugDetail");
        if (cur_node){debugDetail=std::stoi(cur_node->value());} else {std::cout << " pas debugDetail dans fichier xml" << std::endl;}
        mDebug=debugDetail;

        cur_node = root_node->first_node("nbDaysStress");
        if (cur_node){nbDaysStress=std::stoi(cur_node->value());
        std::cout << "nombre de jour de stress premis : " << nbDaysStress << std::endl;} else {std::cout << " pas nbDaysStress dans fichier xml" << std::endl;}
        cur_node = root_node->first_node("doDelaisCoupe");
        if (cur_node){doDelaisCoupe=std::stoi(cur_node->value());} else {std::cout << " pas doDelaisCoupe dans fichier xml" << std::endl;}
        cur_node = root_node->first_node("doFirstDateSco");
        if (cur_node){doFirstDateSco=std::stoi(cur_node->value());} else {std::cout << " pas doFirstDateSco dans fichier xml" << std::endl;}

        cur_node = root_node->first_node("date1");
        if (cur_node){d1=cur_node->value();} else {std::cout << " pas date1 dans fichier xml" << std::endl;}
        cur_node = root_node->first_node("date2");
        if (cur_node){d2=cur_node->value();} else {std::cout << " pas date2 dans fichier xml" << std::endl;}

        cur_node = root_node->first_node("mergeEPSG");
        if (cur_node){mergeEPSG=std::stoi(cur_node->value());} else {std::cout << " pas mergeEPSG dans fichier xml" << std::endl;}
        cur_node = root_node->first_node("mergeEtatSan");
        if (cur_node){mergeEtatSan=std::stoi(cur_node->value());} else {std::cout << " pas mergeEtatSan dans fichier xml" << std::endl;}

        cur_node = root_node->first_node("doAnaTS");
        if (cur_node){doAnaTS=std::stoi(cur_node->value());} else {std::cout << " pas doAnaTS dans fichier xml" << std::endl;}
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

        std::cout << " done" << std::endl;
    }
}

std::string getBaseName(int mode, std::string aTuile){
    std::string aRes("");

    switch (mode) {
    case 1:{
        aRes="etatSanitaire_"+aTuile+"_"+globSuffix;
        break;}
    case 2:{
        aRes="FirstDateSco_"+aTuile+"_"+globSuffix;
        break;}
    case 3:{
        aRes="delaisCoupe_"+aTuile+"_"+globSuffix;
        break;}
    default:
        break;
    }
    return aRes;

}
