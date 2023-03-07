#ifndef CPOSTPROCESS_H
#define CPOSTPROCESS_H

#include <cstring>
#include <algorithm>
#include "StdAfx.h"

#include "boost/filesystem.hpp"
#include "ogrsf_frmts.h"
#include "gdal_utils.h"
#include <filesystem>
#include "esoney.h"



namespace fs=boost::filesystem;


class esOney; // etat sanitaire à une année

class cPostProcess
{
public:
    cPostProcess(std::vector<std::pair<int, string> > aMRaster, int mode=1);

    void decompressRaster(std::string aIn, std::string aOut);
    // compresser les résultats
    void compress();
    void compressTif(std::string aIn);

    void project();
    void checkCompression(std::string * aRaster);

    void masque(int seuilPP);
    void statistique();
     // évolution de la crise ventilée par zone bioclimatique
      void statWithStation();

    void surveillance();

    void  extractValToPt(std::string aShpIn);

    void clean();
    void evol();

private:
    std::vector<std::shared_ptr<esOney>> mVES;

};



#endif // CPOSTPROCESS_H
