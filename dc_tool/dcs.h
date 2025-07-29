#ifndef DCS_H
#define DCS_H
#include "dc.h"
#include "param-hl.h"
#include <iostream>
#include <fstream>
#include "vector"
#include "../../force/src/modules/cross-level/tile-cl.h"
#include "cmath"

class dcs
{
public:
    dcs(par_hl_t * param);
    par_hl_t * phl;

    void genClassRaster(std::string inputShpHouppier, std::string inputShpZone);
    void exportTS2txt(std::string dirOut);
    void exportSample2txt(std::string dirOut);

    void maskStat();
    void l3tol2(std::string dirOut);

    // test si je sais changer le format de la TSI de FORCE pour avoir un format lisible par SITS
    // 1 raster individuel par sensorxbandxdate
    void exportDC2Sits_local(std::string dirOut);

    // ça ca crée un datacube au format sits mais qui est la compilation de toute mes tuiles forces, donc le géoref est bidon ainsi que la position respective des pixels.
    // C'est uniquement pour pouvoir avoir la classe sits tibble dans R qui ne peux pas être créé à partir de csv
    void exportallDC2OneSits_local(std::string dirOut);
    std::vector<dc> alldc;
};

#endif // DCS_H
