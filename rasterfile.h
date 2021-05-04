#ifndef RASTERFILE_H
#define RASTERFILE_H
#include <boost/algorithm/string/replace.hpp>
#include "cpl_conv.h" // for CPLMalloc()
// pour les vecteurs
#include "ogrsf_frmts.h"
#include "gdal_utils.h"
#include <numeric>
#include "iostream"


class rasterFiles;
class pts;

inline bool exists (const std::string& name){
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}


class rasterFiles{
public:
    rasterFiles(std::string aPathTif);
    rasterFiles();
    //~rasterFiles();

    // retourne le chemin d'accès complêt
    std::string getPathTif() const{return mPathRaster;}

    bool rasterExist(){return exists(mPathRaster);}

    // méthode GDAL
    int getValue(double x, double y);
    double getValueDouble(double x, double y);


protected:
    std::string mPathRaster, mPathQml, mCode;
};


class pts{
public:    pts(double aX, double aY):mX(aX),mY(aY){}

    double X(){return mX;}
    double Y(){return mY;}

    // clé; date, val, valeur du masque
    std::map<std::string,int> mVMasqVals;
    // clé; date, val, valeur du cr
    std::map<std::string,double> mCRVals;

    std::string catHeader();
    std::string catVal();

private:
    double mX, mY;
};

#endif // RASTERFILE_H
