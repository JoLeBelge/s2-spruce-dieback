#ifndef CARTEESS_H
#define CARTEESS_H

#include <iostream>
#include "boost/program_options.hpp"
#include <cstring>
#include <algorithm>

#include "boost/filesystem.hpp"
#include "ogrsf_frmts.h"
#include "gdal_utils.h"
#include <filesystem>
/* si je tente de compiler rapidjson et micmac , j'ai des erreurs car certaines macro micmac ont les mm nom que certaines méthode de rapidjson
# celui-la donne une solution;
 https://titanwolf.org/Network/Articles/Article?AID=efb01467-4032-467f-b6cb-61a20274788e
en fait ça fonctionne très bien si j'effectue l'incude de rapidjson AVANT celui de MICMAC, Je suppose que micmac est plus propre et qu'il effectue un undef des noms de macro conflictuel, pas comme rapidjson
*/

#include "../catalogueperiodpheno.h"
#include "StdAfx.h"

using namespace rapidxml;
using namespace libzippp;
using namespace rapidjson;
using namespace date;

//using namespace std;
namespace po = boost::program_options;

void echantillonPts();
std::vector<int> subsample(int nbSub,int nbTot);

std::string roundDouble(double d, int precisionVal=3);

void readXML(std::string aXMLfile);

// mes points
class mpt : public   OGRPoint
{
public:
   mpt(double x, double y, int code):OGRPoint(x,y),mCode(code){}
   std::string cat(){ return std::to_string(mCode)+";"+std::to_string(getX())+";"+std::to_string(getY());}
   int Code(){return mCode;}
private:
     int mCode;
};

#endif // CARTEESS_H
