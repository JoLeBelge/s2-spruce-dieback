#ifndef RASTERFILE_H
#define RASTERFILE_H
#include <boost/algorithm/string/replace.hpp>
#include "cpl_conv.h" // for CPLMalloc()
// pour les vecteurs
#include "ogrsf_frmts.h"
#include "gdal_utils.h"
#include <numeric>
#include "iostream"
#include <boost/filesystem.hpp>


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

    // retourne le chemin d'accès complêt
    std::string getPathTif() const{return mPathRaster;}

    bool rasterExist(){return exists(mPathRaster);}

    // méthode GDAL
    int getValue(double x, double y, bool quiet=1);
    double getValueDouble(double x, double y);

    pts getUV(double x, double y);

protected:
    std::string mPathRaster, mPathQml, mCode;
};


class pts{
public:
     pts(double aX, double aY):mX(aX),mY(aY),mId(-1){}
    pts(int aId,double aX, double aY):mId(aId),mX(aX),mY(aY){}
    double X(){return mX;}
    double Y(){return mY;}

    int getID(){return mId;}

    void setX(double x){ mX=x;}
    void setY(double y){ mY=y;}

private:
    double mX, mY;
    int mId;
};

#endif // RASTERFILE_H
