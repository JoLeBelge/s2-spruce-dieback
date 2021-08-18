#include <iostream>
#include "boost/program_options.hpp"
#include "cpostprocess.h"

using namespace std;
namespace po = boost::program_options;
/*
 * AOUT 2021 JO
 * OBJECTIF ; on a généré des cartes d'état sanitaire pour chaque année de 2016 à 2021 pour les pessières scolytés. Il y a 6 code d'état
 * L'analyse temporelle est strictement temporelle et non spatiale, car pas de comparaison d'une valeur de pixel avec ses voisins.
 * Il en résulte que certaines pessières sont classée en coupe alors que juste à coté on a de la coupe sanitaire. Certains filtres morpho permettraient modifier ça.
 * de plus, on aimerai aussi nuancer les classes scolytés (2 ; scolyté et 4 ; coupe sanitaire) en précisant si ce sont de nouveaux scolyte de l'année ou si c'était déjà présent l'année d'avant.
 *
 * Utilisation de la librairie elise de micmac pour les filtres morphos
 *
 * input ; la série tempo de carte d'état sanitaire.
 *
 * ./s2_postProcess --outils 2 --rasterIn ../../../ana2021/etatSan2/etatSanitaire_ANNEE.tif

 * */


int main(int argc, char *argv[])
{

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("outils", po::value<int>()->required(), "choix de l'outil à utiliser (1 : nettoyage des cartes, 2 : calcul des cartes d'évolutions du scolyte")
            ("rasterIn", po::value<std::string>()->required(), "raster unique d'état san ou pattern de nom de la série tempo de raster avec ANNEE à la place de l'année, ex: etatSanitaire_ANNEE.tif")
            ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    if (vm.count("outils")) {
        int mode(vm["outils"].as<int>());
        std::string pathIn;
        if (vm.count("rasterIn")) {
            pathIn=vm["rasterIn"].as<std::string>();
            // clé: année ou numéro, val ; nom du raster
            std::vector<std::pair<int,std::string>> vIn;
            std::map<int,std::string> mIn; // pour ordonner les raster par dates, juste de manière temporaire
            // détermine si il y a le mot de code ANNEE dans le nom et chercher les fichiers qui correspondent à la série temporelle
            if (pathIn.find("ANNEE")!=std::string::npos){
                fs::path pIn=pathIn;
                std::size_t found = pIn.stem().string().find("ANNEE");
                if (found!=std::string::npos){

                    //std::cout << "ANNEE : " << found << '\n';
                    std::string base=pIn.stem().string().substr(0,found);
                    std::cout <<" recherche de fichier tif avec nom " << base << " + année"<< std::endl;
                    int lengthName =pIn.stem().string().size()-1; // -1 car 4 chiffre pour année mais code ANNEE est de longeur 5

                    for (const std::filesystem::directory_entry & dirEntry : std::filesystem::recursive_directory_iterator(pIn.parent_path().string())){

                        found =dirEntry.path().stem().string().find(base);
                        // attention, j'ai aussi les fichiers résultats qui ont la même base de nom de fichier (ex _tmp.tif) qu'il ne faut pas prendre avec!!
                        if (found!=std::string::npos && dirEntry.path().extension()==".tif" && dirEntry.path().stem().string().size()==lengthName){
                            //std::cout << dirEntry << std::endl;
                            //std::cout << dirEntry.path().stem().string().substr(base.size(),base.size()+4) << std::endl;
                            int y=std::stoi(dirEntry.path().stem().string().substr(base.size(),base.size()+4) );
                            mIn.emplace(std::make_pair(y,dirEntry.path().string()));
                        }
                    }
                    // la map sert juste à ordonner les rater par date, on passe tout dans un vecteur
                    for (auto kv : mIn){
                        vIn.push_back(std::make_pair(kv.first,kv.second));
                        std::cout << kv.first << ", " << kv.second<< std::endl;
                    }

                }

            } else if (fs::exists(pathIn)){
                vIn.push_back(std::make_pair(1,pathIn));
            } else { std::cout << " le nom de raster en entrée semble erronné (fichier inexistant) " << std::endl;
                return 1;
            }

            switch (mode) {
            case 1:{
                std::cout << " je vais effectuer le post-traitement pour " << vIn.size() << " cartes " << std::endl;
                cPostProcess app(vIn,1);
                app.clean();
                break;
            }
            case 2:{
                std::cout << " je vais effectuer le post-traitement pour " << vIn.size() << " cartes " << std::endl;
                cPostProcess app(vIn,2);
                app.evol();
                break;
            }
            default:{
                std::cout << " mode outils incorrect " << std::endl;
            }
            }

        } else { std::cout << "Veillez préciser un raster en entrée" << std::endl;}

    } else {  std::cout << "Veillez préciser un outils en entrée" << std::endl;}
    return 1;

}
