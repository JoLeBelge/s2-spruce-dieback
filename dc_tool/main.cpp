#include <iostream>
#include "dcs.h"
#include "param-hl.h"
#include "boost/program_options.hpp"
#include "../../force/src/modules/cross-level/string-cl.h"

namespace po = boost::program_options;
using namespace std;

//./dc_tool --param_hl /home/jo/Documents/S2/param/dc_tool_param.prm --dirOut /home/jo/Documents/S2/dc_tool_out/ --input_gpkg /home/jo/Documents/OGF/deperissementCH/houppier/dead_trees_from_ortho_v2.gpkg --input_zone_gpkg /home/jo/Documents/OGF/deperissementCH/houppier/grille_1ha_chenaie_selected.gpkg

// ./dc_tool --param_hl /home/jo/Documents/S2/param/dc_tool_param.prm --dirOut /home/jo/Documents/S2/dc_tool_out/ --input_gpkg /home/jo/Documents/OGF/deperissementCH/houppier/dead_trees_from_ortho_v2.gpkg --input_zone_gpkg /home/jo/Documents/OGF/deperissementCH/houppier/grille_1ha_chenaie_selected.gpkg




//./dc_tool --param_hl /home/jo/Documents/S2/param/dc_tool_param.prm --dirOut /home/jo/Documents/S2/dc_tool_out/sits/ --mode 2

int main(int argc, char *argv[])
{
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "aide")
            ("param_hl", po::value< std::string>()->required(), "fichier de paramètre force hl")
            ("dirOut", po::value< std::string>()->required(), "dossier pour écrire les résultats")
            ("input_gpkg", po::value< std::string>(), "gpkg avec houppiers dépérissant")
            ("input_zone_gpkg", po::value< std::string>(), "gpkg avec les zones photointerprétées. les parties sans houppiers dépérissants correspondent au arbres sains")
            ("mode", po::value<int>(), "switch between different tools. 1: genClassRaster (label). 2:")
            ("outName", po::value< std::string>(), "name for class raster mask")
            ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }
    GDALAllRegister();

    par_hl_t    *phl= allocate_param_higher();
    copy_string(phl->f_par, NPOW_10, vm["param_hl"].as<std::string>().c_str());
    if (parse_param_higher(phl) == FAILURE){
        printf("Reading parameter file failed!\n"); return FAILURE;}

    dcs allcube(phl);

    int mode(1);
    if (vm.count("mode")) {mode=vm["mode"].as<int>();}

    switch (mode) {

    case 1:{
        if (vm.count("input_gpkg") && vm.count("input_zone_gpkg") ) {
            if(vm.count("outName")){
            allcube.genClassRaster(vm["input_gpkg"].as<std::string>(),vm["input_zone_gpkg"].as<std::string>(), vm["outName"].as<std::string>());
            } else {
                  allcube.genClassRaster(vm["input_gpkg"].as<std::string>(),vm["input_zone_gpkg"].as<std::string>());
            }
        } else {
            std::cout << "require input_gpkg and input_zone_gpkg" << std::endl;
        }
        break;
    }
    case 2:{
         if(vm.count("outName")){
        allcube.exportallDC2OneSits_local(vm["dirOut"].as<std::string>(),vm["outName"].as<std::string>() );
         } else { allcube.exportallDC2OneSits_local(vm["dirOut"].as<std::string>());}
        break;
    }
    case 3:{
        // finalement je peux beaucoup plus simplement utiliser
        // TSI avec exploded=T
        // puis
        // mm3d MyRename "[0-9]*-[0-9]*_001-365_HL_TSA_SEN2L_(.*)_TSI_([0-9][0-9][0-9][0-9])([0-9][0-9])([0-9][0-9]).tif" "SEN2L_FORCETSI_T1_\$1_\$2-\$3-\$4.tif" Exe=0
        // pour avoir un format sits ready to process!

        allcube.exportDC2Sits_local(vm["dirOut"].as<std::string>());
        break;
    }

    case 4:{
        // generate tile allowed list based on composition map
        std::cout << " generate tile allowed list " << std::endl;
        allcube.maskStat();
        break;
    }

    case 5:{
        // micmic level2-force from level3-TSI for using udf entry point in force but with tsa output. See discussion on github forum
        std::cout << " micmic level2-force from level3-TSI " << std::endl;
        //allcube.l3tol2(vm["dirOut"].as<std::string>());
        allcube.l3tol2(phl->d_higher);
        break;
    }

        //allcube.exportTS2txt(vm["dirOut"].as<std::string>());
        //allcube.exportDC2Sits_local(vm["dirOut"].as<std::string>());
        //allcube.exportSample2txt(vm["dirOut"].as<std::string>());
    }
    return 0;
}
