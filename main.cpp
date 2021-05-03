#include <iostream>
#include "tuiles2.h"
using namespace std;

extern std::string wd;
extern std::string buildDir;
extern std::string path_otb;
extern std::string EP_mask_path;
extern std::string iprfwFile;
int main(int argc, char *argv[])
{
    char userName[20];
    getlogin_r(userName,sizeof(userName));
    std::string s(userName);

    std::string inputJson("/home/lisein/Documents/Scolyte/S2/s2_ts/theia_d/search.json");
    if (s=="lisein"){

    } else {
        wd="/home/gef/Documents/test/";
        buildDir="/home/gef/Documents/build-s2_ts/";
        inputJson="/home/gef/Documents/s2/theia_d/search.json";
        path_otb="/home/gef/Documents/OTB-7.2.0-Linux64/bin/";
        EP_mask_path="/home/gef/Documents/input/";
    }
    iprfwFile=EP_mask_path+"ptsIPRFW.csv";

    // création du wd si nécessaire
    boost::filesystem::path dir(wd);
    boost::filesystem::create_directory(dir);
    boost::filesystem::path dir2(wd+"raw/");
    boost::filesystem::create_directory(dir2);
    boost::filesystem::path dir3(wd+"intermediate/");
    boost::filesystem::create_directory(dir3);

    GDALAllRegister();
    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("catalogue", po::value<int>(), "création d'un catalogue d'image Sentinel 2 sur base de ; option (1) fichier json résultant d'une requête theia. option (2) dossiers déjà présent dans dossier intermediate/")
            ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    if (vm.count("catalogue")) {

        int mode(vm["catalogue"].as<int>());
        switch (mode) {
        case 1:{
            catalogue cata(inputJson);
            break;
        }
        case 2:{

            // ne pas mettre de parenthèse !.
            catalogue cata;
            break;
        }
        }
    }

    return 0;
}
