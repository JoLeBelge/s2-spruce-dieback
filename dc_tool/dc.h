#ifndef DC_H
#define DC_H
#include "param-hl.h"
#include <iostream>
#include "ogrsf_frmts.h"
#include "gdal_utils.h"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <map>

namespace fs = boost::filesystem;

class dc
{
public:
    int tileX, tileY;
    std::string dirName, dirHighLev, name;

    //int nx, ny;
    bool exist();
    void genClassRaster(GDALDataset *DShouppiers, GDALDataset *DSzone, std::string aOut);
    void modifyClassRaster(std::string aOut, std::string exp);
    // exporte les valeurs des indices pour chaque pixel en classe 1, 2 ou 3. Quel format? Une ligne par pixel x indice? -> ça c'est le bon plan oui

    // index = vocable force = une bande spectral ou un indice spectral
    std::map<std::tuple<int, int>, std::vector<double> > exportIndex2txt(int idx, bool inverseUV=false);

    int writeIndexInSits_cube(int idx, std::string outDir, int posPix0);

    void exportIndex2Sits_cube(std::string dirOut, int idx);

    void exportComputedVI2Sits_cube(std::string dirOut, int vi_num);
    void getTransform(double * tr);

    void l3tol2(std::string dirOut);
    int maskStat();

    std::string getName(){return name;}
    std::string getHL(){return dirHighLev;}

    dc(par_hl_t * param, int& x, int& y);
    par_hl_t *phl;
};

#endif // DC_H
