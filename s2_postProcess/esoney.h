#ifndef ESONEY_H
#define ESONEY_H
#include <cstring>
#include <algorithm>
#include "StdAfx.h"

#include "boost/filesystem.hpp"
#include "ogrsf_frmts.h"
#include "gdal_utils.h"
#include <filesystem>
#include "../rasterfile.h"

namespace fs=boost::filesystem;

void ChamferNoBorder(Im2D<U_INT1,INT> * i2d);

class esOney{
public:
    esOney(std::string aRaster, int y,int mode);
    ~esOney(){delete mIm;}

    void createTmp();
    void updateCoupeSan(Im2D_U_INT1 * imPrevYear);
    void updateCoupeSanRetro(Im2D_U_INT1 * imNextYear);
    void detectNewSco(Im2D_U_INT1 * imPrevYear);
    void clean();
    void copyTifMTD(std::string aRasterOut);
    void saveClean();
    void saveEvol();
     void saveMasq();
    void loadClean();
    void project();

    void extractValToPt(std::string aShpIn);

    void stat();
    void statWithMasque(Im2D_U_INT1 * imMasque, int aMasqVal);

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
    std::string getNameMasq(){
       return mRasterName.substr(0,mRasterName.size()-4)+"_masq.tif";
    }
    std::string getNameProj(){
       return mRasterName.substr(0,mRasterName.size()-4)+"_BL72.tif";
    }
    std::string getNameRaster(){return mRasterName;}
    std::string getY(){return std::to_string(mAn);}
private:
    std::string mRasterName;
    int mAn;
    Im2D_U_INT1 * mIm;

};

#endif // ESONEY_H
