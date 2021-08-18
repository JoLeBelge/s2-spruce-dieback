#ifndef CPOSTPROCESS_H
#define CPOSTPROCESS_H

#include <cstring>
#include <algorithm>
#include "StdAfx.h"

#include "boost/filesystem.hpp"

#include "ogrsf_frmts.h"
#include "gdal_utils.h"
#include <filesystem>

namespace fs=boost::filesystem;


class esOney; // etat sanitaire à une année

class cPostProcess
{
public:
    cPostProcess(std::vector<std::pair<int, string> > aMRaster, int mode=1);

    // copie des fonctions que j'ai développée pour nettoyer la carte d'apport en eau pour FEELU en

    void cleanVoisinage(Im2D_U_INT1 * aIn,int Val2Clean, int ValConflict1, int seuilVois);
    void cleanIsolatedPix(Im2D_U_INT1 * aIn,int Val2Clean, int Val2Replace, int seuilVois);
    // fillHole
    void fillHole(Im2D_U_INT1 * aIn, int Val2Clean, int ValConflict1, int ValCopain,int seuilVois, int aSz1, int aSz2);
    void decompressRaster(std::string aIn, std::string aOut);
    // compresser les résultats
    void compress();
    void compressTif(std::string aIn);

    void clean();
    void evol();

private:
    std::vector<std::shared_ptr<esOney>> mVES;

};


class esOney{
public:
    esOney(std::string aRaster, int mode);
    ~esOney(){delete mIm;}

    void createTmp();
    void updateCoupeSan(Im2D_U_INT1 * imPrevYear);
    void updateCoupeSanRetro(Im2D_U_INT1 * imNextYear);
    void detectNewSco(Im2D_U_INT1 * imPrevYear);
    void clean();
    void copyTifMTD(std::string aRasterOut);
    void saveClean();
    void saveEvol();
    void loadClean();

    Im2D_U_INT1 * getImPtr(){return mIm;}

    std::string getNameTmp(){
       return mRasterName.substr(0,mRasterName.size()-4)+"_tmp.tif";
    }
    std::string getNameClean(){
       return mRasterName.substr(0,mRasterName.size()-4)+"_clean.tif";
    }
    std::string getNameEvol(){
       return mRasterName.substr(0,mRasterName.size()-4)+"_evol.tif";
    }
    std::string getNameRaster(){return mRasterName;}
private:
    std::string mRasterName;
    Im2D_U_INT1 * mIm;

};

#endif // CPOSTPROCESS_H
