#include <iostream>
#include "tuiles2.h"
using namespace std;

extern std::string wd;
extern std::string buildDir;
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
    }

    // création du wd si nécessaire
    boost::filesystem::path dir(wd);
    boost::filesystem::create_directory(dir);

    catalogue cata(inputJson);
    return 0;
}
