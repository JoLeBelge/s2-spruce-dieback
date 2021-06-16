#include <iostream>
#include "catalogue.h"
#include <execution>
#include <boost/filesystem.hpp>
using namespace std;

extern std::string wdRacine;// j'en fait un deuxieme car je vais changer wd dans la boucle sur la liste des tuiles
extern std::string wd;
extern std::string buildDir;
extern std::string path_otb;
extern std::string EP_mask_path;
extern std::string globTuile;
extern double seuilCR;

bool mergeEtatSan(0);
extern bool doAnaTS;

//extern std::string iprfwFile;
extern int year_analyse;

extern double Xdebug;
extern double Ydebug;
extern bool debugDetail;

extern bool overw;
extern int nbDaysStress;

int main(int argc, char *argv[])
{
    char userName[20];
    getlogin_r(userName,sizeof(userName));
    std::string s(userName);
    std::string pathTheiaD("/home/lisein/Documents/Scolyte/S2/s2_ts/theia_d/");
    if (s=="lisein"){

    } else {
        wdRacine="/media/gef/Data2/S2Scolyte/";
        buildDir="/home/gef/Documents/build-s2_ts/";
        path_otb="/home/gef/Documents/OTB-7.2.0-Linux64/bin/";
        EP_mask_path="/home/gef/Documents/input/";
        pathTheiaD="/home/gef/Documents/s2/theia_d/";

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
            ("srCR", po::value<double>(), "seuil ration CR à partir duquel on détecte un stress. Defaut 1.4")
            ("nbJourStress", po::value<int>(), "nombre du jours seuil à partir dusquel on n'envisage plus un retour à la normal pour un stress temporaire pronlongé. Default 90")
            ("mergeEtatSan", po::value<bool>(), "fusionne les cartes d'état sanitaire")
            ("anaTS", po::value<bool>(), "effectue l'analyse sur la série temporelle, defaut true mais si on veux faire un merge des cartes Etat san sans tout recalculer -->mettre à false")
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
        if (vm.count("srCR")) {seuilCR=vm["srCR"].as<double>();}
        if (vm.count("anaTS")) {doAnaTS=vm["anaTS"].as<bool>();}
        if (vm.count("mergeEtatSan")) {mergeEtatSan=vm["mergeEtatSan"].as<bool>();}

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
                    //if (aVTuiles.size()==1){

                        // lancer la requete theia avant de créer le catalogue
                        //std::string aCommand="python "+pathTheiaD+"/theia_download.py -t "+globTuile+" -c SENTINEL2 -a "+pathTheiaD+"config_theia.cfg -d 2016-01-01 -f 2020-06-01 -m 1 -n -w"+wd;
                        std::string aCommand="curl -k  -o "+wd+"search.json 'https://theia.cnes.fr/atdistrib/resto2/api/collections/SENTINEL2/search.json?completionDate=2020-06-01&startDate=2016-01-01&maxRecords=500&location="+globTuile+"&processingLevel=LEVEL2A'";
                        std::cout << aCommand << std::endl;
                        system(aCommand.c_str());
                        std::string inputJson=wd+"search.json";
                        catalogue cata(inputJson);
                    //} else {
                    //    std::cout << " vous avez renseigné une liste de tuile avec l'option création de catalogue depuis une recherche theai ; incompatible." << std::endl;
                    //}
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
            std::string aCommand("gdal_merge.py -n 0 -o "+dir.string()+"/etatSanitaire_"+std::to_string(kv.first)+".tif -of GTiff -v --optfile "+dir.string()+"/merge_"+std::to_string(kv.first)+".txt");
            std::cout << aCommand << std::endl;
            system(aCommand.c_str());
        }
        }

    }

    return 0;
}
