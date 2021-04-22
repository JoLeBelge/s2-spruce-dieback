#include <iostream>
#include "tuiles2.h"
using namespace std;

extern std::string wd;
extern std::string buildDir;
extern std::string path_otb;
extern std::string EP_mask_path;
int main()
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

    // création du wd si nécessaire
    boost::filesystem::path dir(wd);
    boost::filesystem::create_directory(dir);
    boost::filesystem::path dir2(wd+"raw/");
    boost::filesystem::create_directory(dir2);
    boost::filesystem::path dir3(wd+"intermediate/");
    boost::filesystem::create_directory(dir3);

    catalogue cata(inputJson);
    return 0;
}
