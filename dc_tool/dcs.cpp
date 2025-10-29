#include "dcs.h"
extern std::map<std::string, std::string> mIndexName2Band;

dcs::dcs(par_hl_t *param):phl(param)
{
    int *allow_x = NULL, *allow_y = NULL, allow_k;

    // read tile allow-list
    if (tile_readlist(phl->f_tile, &allow_x, &allow_y, &allow_k) != SUCCESS){
        printf("Reading tile file failed!");

        //force-cube.sh donne un exemple de création de la liste des tuiles sur base de la définition du dc
        // mais je n'arrive pas à comprendre vite du coup je vais lire le shp grid pour aller plus vite

        std::string grid =std::string(phl->d_lower)+"/grid.kml";
        std::cout << "reading grid " << grid << std::endl;
        GDALAllRegister();
        std::cout << "grid " << grid.c_str() << std::endl;
        GDALDataset * DS =  (GDALDataset*) GDALOpenEx( grid.c_str(), GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL );
        OGRFeature *poFeature;
        std::cout << "looping "<< std::endl;
        std::cout << " tile X range :"  <<phl->tx[0] << " to "<<phl->tx[1] << std::endl;
        std::cout << " tile Y range :"  <<phl->ty[0] << " to "<<phl->ty[1] << std::endl;
        while( (poFeature = DS->GetLayer(0)->GetNextFeature()) != NULL )
        {
            int x=poFeature->GetFieldAsInteger("Tile_X");
            int y=poFeature->GetFieldAsInteger("Tile_Y");
            if (phl->tx[0] < x+1 & phl->tx[1] > x-1 & phl->ty[0] < y+1 & phl->ty[1] > y-1){

            dc datacube(phl,x,y);
            alldc.push_back(datacube);
            } else {
                std::cout << " tuile " << x << ", " << y << " n'est pas dans tile_range" << std::endl;
            }
        }
        GDALClose(DS);


    } else  {
        //phl->tsa.index
        std::cout << " tile X range :"  <<phl->tx[0] << " to "<<phl->tx[1] << std::endl;
        std::cout << " tile Y range :"  <<phl->ty[0] << " to "<<phl->ty[1] << std::endl;
        for (int i(0); i<allow_k;i++){

            if (phl->tx[0] < allow_x[i]+1 & phl->tx[1] > allow_x[i]-1 & phl->ty[0] < allow_y[i]+1 & phl->ty[1] > allow_y[i]-1){
            std::cout << "allow_x " << allow_x[i] << ", allow_y " << allow_y[i]<< std::endl;

            dc datacube(phl, allow_x[i],allow_y[i]);
            if (datacube.exist()){
                alldc.push_back(datacube);
            } else {
                std::cout << "datacube " << datacube.dirName << "n'existe pas!" << std::endl;
            }
            }
        }
        std::cout << "Inititalisation des datacubes ; " << alldc.size() << " tuiles" << std::endl;

    }

}

void dcs::genClassRaster(std::string inputShpHouppier, std::string inputShpZone, std::string aOut){
    // lecture du geopackage
    std::cout << " gen Class Raster" << std::endl;
    //GDALDataset * DShouppier =  (GDALDataset*) GDALOpenEx( inputShpHouppier.c_str(), GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL );
    // si je souhaite faire une buffer positif, il faut ouvrir en mode édition. copier en premier lieu la couche.
    GDALDataset * DShouppier =  (GDALDataset*) GDALOpenEx( inputShpHouppier.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE, NULL, NULL, NULL );
    GDALDataset * DSzone =  (GDALDataset*) GDALOpenEx( inputShpZone.c_str(), GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL );
    if( DShouppier != NULL & DSzone !=0 )
    {
        for (dc datacube : alldc){
            datacube.genClassRaster(DShouppier, DSzone, aOut);
        }

        GDALClose(DShouppier);
        GDALClose(DSzone);
    } else {
        std::cout << "Nope, fichier n'existe pas" << std::endl;
    }

}

void dcs::modifyClassRaster(std::string aOut, std::string aExp){

        for (dc datacube : alldc){
            datacube.modifyClassRaster(aOut, aExp);
        }


}

void dcs::exportTS2txt(std::string dirOut){
    std::string fileOut(dirOut+"/datacube_depe.csv");
    std::ofstream ofs (fileOut, std::ofstream::out);
    ofs << "#U;V;BAND;classe" ;
    char fname[NPOW_10];
    int ni = std::ceil((phl->date_range[_MAX_].ce-phl->date_range[_MIN_].ce+1)/phl->tsa.tsi.step);
    for (int t=0; t<ni; t++){
        //date ce : current era
        date_t date;
        set_date_ce(&date, phl->date_range[_MIN_].ce + t*phl->tsa.tsi.step);
        snprintf(fname, NPOW_10, "%04d%02d%02d",date.year, date.month, date.day);
        ofs << ";" << fname;
    }
    ofs << "\n";

    for (dc datacube : alldc){

        for (int idx=0; idx<phl->tsa.n; idx++){
            std::map<std::tuple<int, int>, std::vector<double>> mTS=datacube.exportIndex2txt(idx);
            for (auto & kv: mTS){
                ofs << std::get<0>(kv.first) << ";"<<  std::get<1>(kv.first) ;
                snprintf(fname, NPOW_10, "%s",phl->tsa.index_name[idx]);
                ofs << ";" << fname ;
                for (double & d : kv.second){
                    // je gère les na ici pour plus de facilité par après.

                    ofs << ";" ;
                    if (d!=-9999){ofs<< d ;}
                }
                ofs << "\n";
            }
        }

    }
}

void dcs::exportSample2txt(std::string dirOut){
    std::string fileOut(dirOut+"/sample_depe.csv");
    std::ofstream ofs (fileOut, std::ofstream::out);
    ofs.precision(7);
    // il faut augmenter le nombre de chiffre après la virgule qui sont écrit dans le fichier txt
    ofs << "longitude,latitude,start_date,end_date,label\n" ;

    char startD[NPOW_10],endD[NPOW_10];
    snprintf(startD, NPOW_10, "%04d-%02d-%02d",phl->date_range[_MIN_].year, phl->date_range[_MIN_].month, phl->date_range[_MIN_].day);
    snprintf(endD, NPOW_10, "%04d-%02d-%02d",phl->date_range[_MAX_].year, phl->date_range[_MAX_].month, phl->date_range[_MAX_].day);

    OGRSpatialReference source, target;
    target.importFromEPSG(4326);
    source.importFromEPSG(31370);

    OGRPoint p;
    //p.assignSpatialReference(&source);
    //p.transformTo(&target);
    for (dc datacube : alldc){

        double tr[6];
        datacube.getTransform(tr);

        for (int idx=0; idx<1; idx++){
            std::map<std::tuple<int, int>, std::vector<double>> mTS=datacube.exportIndex2txt(idx);
            for (auto & kv: mTS){
                p.setX(tr[0]+std::get<0>(kv.first)*tr[1]+0.5*tr[1]);
                p.setY(tr[3]+std::get<1>(kv.first)*tr[5]+0.5*tr[5]);
                p.assignSpatialReference(&source);
                p.transformTo(&target);

                int depe= kv.second.at(0);

                ofs << p.getY() << "," << p.getX() << "," << startD << "," << endD << ",label"<<  depe << "\n";
            }
        }

    }
}

// dans un premier temps je fait le process pour un seul datacube car l'écriture de toute les tuilettes de force dans des raster partagés ce n'est pas si facile
void dcs::exportDC2Sits_local(std::string dirOut){
    for (dc datacube : alldc){
        char fname[NPOW_10];
        snprintf(fname, NPOW_10, "X%04d_Y%04d", datacube.tileX, datacube.tileY);
        boost::filesystem::path dir(dirOut+"/"+fname);
        boost::filesystem::create_directory(dir);
        if(1){
            for (int idx=0; idx<phl->tsa.n; idx++){
                datacube.exportIndex2Sits_cube(dir.string(),idx);
            }
        }


    }

}

void dcs::exportallDC2OneSits_local(std::string dirOut, std::string aOut){
    std::cout << "export all DC 2 one decoy DC sits-compatible" << std::endl;
    std::cout << "for a total of " << alldc.size() << " datacubes"<< std::endl;

    // je ne peux pas mettre le DC en WGS84, je ne sais pas pk mais sits n'aime pas ça.
    double tr[6];
    /* tr[0]=5.0;
    tr[1]=0.01;
    tr[2]=0;
    tr[3]=50.0;
    tr[4]=0;
    tr[5]=-0.01;*/

    tr[0]=200000;
    tr[1]=100;
    tr[2]=0;
    tr[3]=100000;
    tr[4]=0;
    tr[5]=-100;

    OGRSpatialReference target, source;
    target.importFromEPSG(4326);
    source.importFromEPSG(31370);

    OGRPoint p;/*
    p.assignSpatialReference(&source);
    p.transformTo(&target);
    */

    std::string fileOut(dirOut+"/"+aOut);
    std::string fileOut2(dirOut+"/"+aOut+"_classDepePI.csv");
    std::ofstream ofs (fileOut, std::ofstream::out);
    ofs.precision(7);
    // il faut augmenter le nombre de chiffre après la virgule qui sont écrit dans le fichier txt
    ofs << "longitude,latitude,start_date,end_date,label\n" ;

    std::ofstream ofs2 (fileOut2, std::ofstream::out);
    ofs2.precision(7);
    // il faut augmenter le nombre de chiffre après la virgule qui sont écrit dans le fichier txt
    ofs2 << "xBidon,yBidon,label,x,y,b1,b2,b3,b4,b5,b6,b7\n" ;

    char startD[NPOW_10],endD[NPOW_10];
    snprintf(startD, NPOW_10, "%04d-%02d-%02d",phl->date_range[_MIN_].year, phl->date_range[_MIN_].month, phl->date_range[_MIN_].day);
    snprintf(endD, NPOW_10, "%04d-%02d-%02d",phl->date_range[_MAX_].year, phl->date_range[_MAX_].month, phl->date_range[_MAX_].day);

    // 1 déterminer le nombre de pixels au total et exporte le fichier csv
    int nb(0);
    for (dc datacube : alldc){
        int idx=0;
        std::map<std::tuple<int, int>, std::vector<double>> mTS=datacube.exportIndex2txt(idx);
        std::cout << "pixels pour datacube " << datacube.tileX << ", " << datacube.tileY << ": " << mTS.size() << "." << std::endl;
        nb=nb+mTS.size();
    }

    // taille en pixels du decoy DC
    int c=std::sqrt(nb)+1;
    std::cout << "nombre de pixel de mon DC : " << nb << ", soit un raster de " << c << " pixels de large" << std::endl;

    // export fichier sample.csv avec coordonnée bidon mais en cohérence avec le DC bidon
    std::cout << "\nexport Csv sample" << std::endl;
    int i(0);
    for (dc datacube : alldc){
        // std::map<std::tuple<int, int>, std::vector<double>> mTS=datacube.exportIndex2txt(0,true);
        std::map<std::tuple<int, int>, std::vector<double>> mTS=datacube.exportIndex2txt(-1,true);
        for (auto & kv: mTS){
            // il ne sont pas géoréférencé comme je veux, c'est pas cohérent avec les valeurs foireux :-)
            // j'ai trouvé ; c'est parce que la map des tupples trie les pixels par u, v alors que la lecture ce fait dans l'autre sens
            // donc je dois faire en sorte de ne pas retourner une map, ou de trier la map dans le sens identique à le lecture de mes rasters.
            int rowDC= std::ceil(i/c);
            int colDC= i-(rowDC*c);

            p.setX(tr[0]+colDC*tr[1]+0.5*tr[1]);
            p.setY(tr[3]+rowDC*tr[5]+0.5*tr[5]);
            p.assignSpatialReference(&source);
            p.transformTo(&target);
            int depe= kv.second.at(0);
            ofs << p.getY() << "," << p.getX() << "," << startD << "," << endD << ",label"<<  depe << "\n";
            // ofs << p.getY() << "," << p.getX() << "," << startD << "," << endD << ",label"<<  i << "\n";
            //je dois ajouter ici la position réelle du pixel pour pouvoir effectuer la conversion dans l'autre sens, càd depuis la géométrie du datacube agregated vers le monde réel
            int uIni=std::get<1>(kv.first);
            int vIni=std::get<0>(kv.first);
            // calculer position réelle
            double x=datacube.tulx + uIni*10;
            double y=datacube.tuly - vIni*10;
            ofs2 << p.getY() << "," << p.getX()  << ",label"<< depe << "," << x << "," << y ;

            for (int j(1);j<kv.second.size();j++){
                ofs2 << "," << kv.second.at(j);
            }
            ofs2 <<  "\n";

            i++;
        }
    }

    std::cout << "initialize des couches raster vides" << std::endl;

    // 2 initialise des rasters vides pour toutes les bandesxdates
    const char *pszFormat = "GTiff";
    GDALDriver *pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);


    for (int idx=0; idx<phl->tsa.n; idx++){
        std::cout << "init band " << phl->tsa.index_name[idx] << std::endl;
        std::string bandName= phl->tsa.index_name[idx];
        if (mIndexName2Band.find(phl->tsa.index_name[idx])!=mIndexName2Band.end()){
            bandName=mIndexName2Band.at(phl->tsa.index_name[idx]);
        }
        char fname[NPOW_10];
        char dateDesc[NPOW_10];
        snprintf(fname, NPOW_10, "%s/%04d%02d%02d-%04d%02d%02d_%03d-%03d_HL_TSA_%s_%s_TSI.tif",
                 dirOut.c_str(),
                 phl->date_range[_MIN_].year, phl->date_range[_MIN_].month, phl->date_range[_MIN_].day,
                 phl->date_range[_MAX_].year, phl->date_range[_MAX_].month, phl->date_range[_MAX_].day,
                 phl->doy_range[_MIN_], phl->doy_range[_MAX_],
                 phl->sen.target, phl->tsa.index_name[idx]);
        int ni = std::ceil((phl->date_range[_MAX_].ce-phl->date_range[_MIN_].ce+1)/phl->tsa.tsi.step);
        for (int t=0; t<ni; t++){
            //date ce : current era
            date_t date;
            set_date_ce(&date, phl->date_range[_MIN_].ce + t*phl->tsa.tsi.step);
            snprintf(dateDesc, NPOW_10, "%04d%02d%02d",date.year, date.month, date.day);

            char outName[NPOW_10];
            //"SENTINEL-2_MSI_20LKP_B02_2018-07-18.jp2" are accepted names. The user has to provide parsing information to allow sits to extract values of tile, band, and date. In the examples above,
            //the parsing info is c("X1", "X2", "tile", "band", "date") and the delimiter is "_", which are the default values.
            snprintf(outName, NPOW_10, "%s/SEN2L_FORCETSI_T1_%s_%04d-%02d-%02d.tif",dirOut.c_str(),bandName.c_str(),date.year, date.month, date.day);

            GDALDataset * out = pDriver->Create(outName, c,c, 1, GDT_Int16, NULL);
            out->GetRasterBand(1)->SetNoDataValue(-9999);
            out->SetSpatialRef(&source);
            out->SetGeoTransform(tr);
            GDALClose(out);
        }

    }

    // 3 écrire la série temporelle dans ces raster, pixel by pixel?
    int pix(0);
    nb=0;
    for (dc datacube : alldc){
        for (int idx=0; idx<phl->tsa.n; idx++){
            nb= datacube.writeIndexInSits_cube(idx, dirOut,pix);
        }
        std::cout << "pixels pour datacube " << datacube.tileX << ", " << datacube.tileY << ": " << nb << "." << std::endl;

        pix=pix+nb;
    }
    std::cout << "nombre de pixel total : " << pix << std::endl;
}

void dcs::maskStat(){
    std::vector<std::string> allowedList;
    for (dc datacube : alldc){
        int nbPix=datacube.maskStat();
        // 250 ha c'est 1/100 de la surface du datacube
        if (nbPix>(250*100)){
            //std::cout << "chenaie pour datacube " << datacube.tileX << ", " << datacube.tileY << ": " << nbPix << "." << std::endl;
            allowedList.push_back(datacube.getName());
        }
    }
    std::cout << "allowed list with " << allowedList.size() << " elements " << std::endl;
    std::ofstream ofs (phl->f_tile, std::ofstream::out);
    ofs << allowedList.size() << "\n" ;
    for (std::string & s : allowedList){
        ofs << s << "\n";
    }
}


void dcs::level3ToDo(){
    std::vector<std::string> allowedList;

    for (dc datacube : alldc){
        int nbPix=datacube.maskStat();
        if (nbPix>0){
        std::string f1=datacube.getHL()+"/20170101-20250615_001-365_HL_TSA_SEN2L_CSW_TSI.tif";
        std::string f2=datacube.getHL()+"/20170101-20250615_001-365_HL_TSA_SEN2L_NDV_TSI.tif";
        if (!fs::exists(f1) | !fs::exists(f2)){
            allowedList.push_back(datacube.getName());
        }
        }
    }
    std::cout << "allowed list with " << allowedList.size() << " elements " << std::endl;
    std::ofstream ofs (phl->f_tile, std::ofstream::out);
    ofs << allowedList.size() << "\n" ;
    for (std::string & s : allowedList){
        ofs << s << "\n";
    }
}

void dcs::l3tol2(std::string dirOut){
    std::cout <<" l3 to l2 " << std::endl;
    for (dc datacube : alldc){
        std::cout <<"one dc " << std::endl;
        datacube.l3tol2(dirOut);
    }
}

