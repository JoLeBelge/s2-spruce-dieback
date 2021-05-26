#include <iostream>
#include "catalogue.h"
using namespace std;

extern std::string wdRacine;// j'en fait un deuxieme car je vais changer wd dans la boucle sur la liste des tuiles
extern std::string wd;
extern std::string buildDir;
extern std::string path_otb;
extern std::string EP_mask_path;
extern std::string globTuile;
//extern std::string iprfwFile;
extern int year_analyse;

extern double Xdebug;
extern double Ydebug;
extern bool debugDetail;

extern bool overw;

int main(int argc, char *argv[])
{
    char userName[20];
    getlogin_r(userName,sizeof(userName));
    std::string s(userName);

    std::string inputJson("/home/lisein/Documents/Scolyte/S2/s2_ts/theia_d/search.json");
    if (s=="lisein"){

    } else {
        wdRacine="/media/gef/Data2/S2Scolyte/";
        buildDir="/home/gef/Documents/build-s2_ts/";
        inputJson="/home/gef/Documents/s2/theia_d/search.json";
        path_otb="/home/gef/Documents/OTB-7.2.0-Linux64/bin/";
        EP_mask_path="/home/gef/Documents/input/";
    }
    //iprfwFile=EP_mask_path+"ptsIPRFW.csv";

    GDALAllRegister();
    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("catalogue", po::value<int>(), "création d'un catalogue d'image Sentinel 2 sur base de ; option (1) fichier json résultant d'une requête theia. option (2) dossiers déjà présent dans dossier intermediate/")
            //("tuile", po::value< std::string>(), "nom de la tuile. sert pour prendre le masque input, nommer le dossier de travail (wd) et les output finaux (carte etatSanitaire).")
            ("tuile",  po::value<std::vector<std::string> >()->multitoken(), "nom de la tuile ou liste de tuile. sert pour prendre le masque input, nommer le dossier de travail (wd) et les output finaux (carte etatSanitaire).")
            ("XYtest", po::value<std::vector<double> >()->multitoken(), "coordonnée d'un point pour lequel on va faire tourner l'analyse temporelle avec de nombreuses information écrite dans la console qui serviront à améliorer les filtres sur les valeurs d'état sanitaire de la TS. Attention, EPSG est 32631 (UTM 31N)")
            ("annee", po::value<int>(), "annee d'analyse - utilisé avant car faire toute les années d'un coup c'était trop long - maintenant c'est reglé")
            ("Overwrite", po::value<bool>(), "Overwrite tout les résultats (prétraitement compris), défaut =0")
            ("testDetail", po::value<bool>(), "pour le test sur une position, affichage ou non des valeurs de toutes les bandes ou juste les valeurs d'état")
            ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    //if (vm.count("tuile")) {

    std::vector<std::string> aVTuiles;
    if (!vm["tuile"].empty()){

        aVTuiles = vm["tuile"].as<vector<std::string> >();
        if (vm.count("annee")) {year_analyse=vm["annee"].as<int>();}
        if (vm.count("Overwrite")) {overw=vm["Overwrite"].as<bool>();}
        if (vm.count("testDetail")) {debugDetail=vm["testDetail"].as<bool>();}

        std::vector<double> opts;
        if (!vm["XYtest"].empty() && (opts = vm["XYtest"].as<vector<double> >()).size() == 2) {
            std::cout << "Test pour une position donnee "<<std::endl;
            Xdebug=opts.at(0);
            Ydebug=opts.at(1);
        }

        for (std::string t : aVTuiles){

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
                    if (aVTuiles.size()==1){
                    catalogue cata(inputJson);
                    } else {
                        std::cout << " vous avez renseigné une liste de tuile avec l'option création de catalogue depuis une recherche theai ; incompatible." << std::endl;
                    }
                    break;
                }
                case 2:{

                    // ne pas mettre de parenthèse !.
                    catalogue cata;
                    break;
                }
                }
            }
        }
    }

    return 0;
}
