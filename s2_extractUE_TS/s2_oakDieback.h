#ifndef S2_OAKDIEBACK_H
#define S2_OAKDIEBACK_H
#include <iostream>
#include "boost/program_options.hpp"
#include <cstring>
#include <algorithm>

#include "boost/filesystem.hpp"
#include "ogrsf_frmts.h"
#include "gdal_utils.h"
#include <filesystem>
#include "date.h"
#include "catalogue.h"
#include "tuiles2OneDate.h"

using namespace rapidxml;
using namespace libzippp;
using namespace rapidjson;
using namespace date;

namespace fs=boost::filesystem;
//using namespace std;
namespace po = boost::program_options;


void readXML(std::string aXMLfile);

void cropIm(std::string inputRaster, std::string aOut, OGREnvelope ext);

bool hasVal(std::string inputRaster, OGREnvelope ext, int aVal);

class catalogueExtract : public catalogue
{
public:
    catalogueExtract(){}

    void extractUE();

};

class ptGDL{
public:

    ptGDL(std::string aId,double aX, double aY):mId(aId),mX(aX),mY(aY){}
    double X(){return mX;}
    double Y(){return mY;}
    std::string getID(){return mId;}
    void setX(double x){ mX=x;}
    void setY(double y){ mY=y;}

private:
    double mX, mY;
    std::string mId;
};

std::vector<ptGDL> readPtGDLFile(std::string aFilePath, int cId,int cX,int cY);

#endif // S2_OAKDIEBACK_H
