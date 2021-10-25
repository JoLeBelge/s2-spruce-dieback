#include <iostream>
#include "cataloguesco.h"
#include <execution>
#include <boost/filesystem.hpp>
#include "date.h"
using namespace std;
using namespace std::chrono;

extern std::string wdRacine;// j'en fait un deuxieme car je vais changer wd dans la boucle sur la liste des tuiles
extern std::string wd;
extern std::string buildDir;
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

std::string d1("2016-01-01"),d2("2021-07-11");

int main(int argc, char *argv[])
{

    year_month_day today = year_month_day{floor<days>(system_clock::now())};
    d2 = format("%F",today);
    //std::cout << "d2 " << d2 << std::endl;
    char userName[20];
    getlogin_r(userName,sizeof(userName));
    std::string s(userName);
    //std::string pathTheiaD("/home/lisein/Documents/Scolyte/S2/s2_ts/theia_d/");
    if (s=="lisein"){

    } else {
        wdRacine="/media/gef/Data2/S2Scolyte/";
        buildDir="/home/gef/Documents/build-s2_ts/";
        path_otb="/home/gef/Documents/OTB-7.2.0-Linux64/bin/";
        EP_mask_path="/home/gef/Documents/input/";
        //pathTheiaD="/home/gef/Documents/s2/theia_d/";

    }

    GDALAllRegister();
    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "affiche l'aide ci-dessous")
            ("catalogue", po::value<int>(), "création d'un catalogue d'image Sentinel 2 sur base de ; option (1) fichier json résultant d'une requête theia. option (2) dossiers déjà présent dans dossier intermediate/")
            ("tuile",  po::value<std::vector<std::string> >()->multitoken(), "nom de la tuile ou liste de tuile. sert pour prendre le masque input, nommer le dossier de travail (wd) et les output finaux (carte etatSanitaire).")
            ("dates",  po::value<std::vector<std::string> >()->multitoken(), "date 1 et date 2 pour le téléchargement de la série temporelle. Default; début 2016-01-01 et fin =date d'aujourd'hui" )
            ("srCR", po::value<double>(), "seuil ratio CRswir à partir duquel on détecte un stress. Defaut 1.4")
            ("nbJourStress", po::value<int>(), "nombre du jours seuil à partir dusquel on n'envisage plus un retour à la normal pour un stress temporaire pronlongé. Default 90")
            ("XYtest", po::value<std::vector<double> >()->multitoken(), "coordonnée d'un point pour lequel on va faire tourner l'analyse temporelle avec de nombreuses information écrite dans la console qui serviront à améliorer les filtres sur les valeurs d'état sanitaire de la TS. Attention, EPSG est 32631 (UTM 31N)")
            ("XYtestIn", po::value< std::string>(), "Fichier texte séparateur virgule avec col 1 = id col 2=X et col3=3, epsg 32631 (UTM 31N), on effectue l'analyse en mode Test sur tout ces points là")
            ("XYtestOut", po::value< std::string>(), "fichier texte ou sauver les résultats de XYTest sur un point.")
            ("Overwrite", po::value<bool>(), "Overwrite tout les résultats (prétraitement compris), défaut =0")
            ("testDetail", po::value<bool>(), "pour le test sur une position, affichage ou non des valeurs de toutes les bandes ou juste les valeurs d'état, def true")
             ("testClean", po::value<bool>(), "pour le test sur une position, nettoyage ou pas, def true")
            ("mergeES", po::value<bool>(), "fusionne les cartes d'état sanitaire des différentes tuiles")
            ("anaTS", po::value<bool>(), "effectue l'analyse sur la série temporelle, defaut true mais si on veux faire un merge des cartes Etat san sans tout recalculer -->mettre à false")
            ("debug", po::value<bool>(), "si true, le logiciel est plus bavard, ça aide pour débugger")

               ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    std::vector<std::string> aVTuiles;
    if (!vm["tuile"].empty()){

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

        if (vm.count("nbJourStress")) {nbDaysStress=vm["nbJourStress"].as<int>();}
        if (vm.count("annee")) {year_analyse=vm["annee"].as<int>();}
        if (vm.count("Overwrite")) {overw=vm["Overwrite"].as<bool>();}
        if (vm.count("testDetail")) {debugDetail=vm["testDetail"].as<bool>();}
        if (vm.count("testClean")) {docleanTS1pos=vm["testClean"].as<bool>();}
        if (vm.count("srCR")) {seuilCR=vm["srCR"].as<double>();}
        if (vm.count("anaTS")) {doAnaTS=vm["anaTS"].as<bool>();}
        if (vm.count("mergeES")) {mergeEtatSan=vm["mergeES"].as<bool>();}
        if (vm.count("debug")) {mDebug=vm["debug"].as<bool>();}
        if (vm.count("XYtestOut")) {globResXYTest=vm["XYtestOut"].as<std::string>();}
        if (vm.count("XYtestIn")) {XYtestFile=vm["XYtestIn"].as<std::string>();}

        std::vector<double> opts;
        if (!vm["XYtest"].empty() && (opts = vm["XYtest"].as<vector<double> >()).size() == 2) {
            std::cout << "Test pour une position donnee "<<std::endl;
            Xdebug=opts.at(0);
            Ydebug=opts.at(1);
        }

        std::vector<std::string> dates;
        if (!vm["dates"].empty() && (dates = vm["dates"].as<vector<std::string> >()).size() == 2) {
            std::cout << "dates de début et de fin entrées par l'utilisateur "<<std::endl;
            d1=dates.at(0);
            d2=dates.at(1);
        }


        for (std::string t : aVTuiles){

            if (t=="T32ULU"){wdRacine="/home/gef/Documents/";}

            wd=wdRacine+ t +"/";
            globTuile=t;
            // création du wd si nécessaire
            boost::filesystem::path dir(wd);
            boost::filesystem::create_directory(dir);
            boost::filesystem::path dir2(wd+"raw/");
            boost::filesystem::create_directory(dir2);
            boost::filesystem::path dir3(wd+"intermediate/");
            boost::filesystem::create_directory(dir3);
            boost::filesystem::path dir4(wd+"output/");
            boost::filesystem::create_directory(dir4);
            boost::filesystem::path dir5(wd+"input/");
            boost::filesystem::create_directory(dir5);

            if (vm.count("catalogue")) {

                int mode(vm["catalogue"].as<int>());
                switch (mode) {
                case 1:{
                        std::cout << "\n\n Création du catalogue pour tuile " << t << "\n\n" <<std::endl;
                        // lancer la requete theia avant de créer le catalogue
                        std::string aCommand="curl -k  -o "+wd+"search.json 'https://theia.cnes.fr/atdistrib/resto2/api/collections/SENTINEL2/search.json?completionDate="+d2+"&startDate="+d1+"&maxRecords=500&location="+globTuile+"&processingLevel=LEVEL2A'";
                        //std::cout << aCommand << std::endl;
                        system(aCommand.c_str());
                        std::string inputJson=wd+"search.json";
                        catalogueSco cata(inputJson);
                        cata.traitement();
                        std::cout << "Tuile " << t << " faite \n\n" <<std::endl;

                    break;
                }
                case 2:{
                    // ne pas mettre de parenthèse !.
                    std::cout <<"\n\n Création du catalogue pour tuile " << t << "\n\n" <<std::endl;
                    catalogueSco cata;
                    cata.traitement();
                    std::cout << "Tuile " << t << " faite \n\n" <<std::endl;

                    break;
                }
                }
            }

        }
        // fin du traitement de chacune des tuiles. maintenant on peux fusionner les tuiles si on le souhaite
        if (mergeEtatSan){
            std::map<int, std::vector<std::string>> aMES;
            for (std::string t : aVTuiles){
                std::string tDir=wdRacine+ t +"/";
                boost::filesystem::directory_iterator end_itr;

                  // cycle through the directory
                  for (boost::filesystem::directory_iterator itr(tDir); itr != end_itr; ++itr)
                  {
                      if (itr->path().filename().string().size()>25 && itr->path().filename().extension().string()==".tif"){
                        if (boost::filesystem::is_regular_file(itr->path()) && itr->path().filename().string().substr(0,14)=="etatSanitaire_") {
                          std::string  etatSan= itr->path().string();
                          //std::cout << etatSan << std::endl;

                          int  year=stoi(itr->path().filename().string().substr(21,4));
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
        // effectue le merge par année
            boost::filesystem::path dir(wdRacine+"merge/");
            boost::filesystem::create_directory(dir);

        for (auto & kv : aMES){
            std::cout << " fusion des cartes d'état sanitaire pour l'année " << kv.first << std::endl;
            // crée un fichier txt qui liste les inputs - pour gdal merge
            std::ofstream out;
            out.open(dir.string()+"merge_"+std::to_string(kv.first)+".txt");
            for (std::string f : kv.second){
                out << f;
              out << "\n";
            }
            out.close();
            std::string aCommand("gdal_merge.py -n 0 -o "+dir.string()+"/etatSanitaire_"+std::to_string(kv.first)+".tif -of GTiff -co 'COMPRESS=DEFLATE' -v --optfile "+dir.string()+"/merge_"+std::to_string(kv.first)+".txt");
            std::cout << aCommand << std::endl;
            system(aCommand.c_str());
        }
        }

    }

    return 0;
}
