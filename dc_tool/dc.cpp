#include "dc.h"
//std::string path_otb("");
//std::string compr_otb="?&gdal:co:INTERLEAVE=BAND&gdal:co:TILED=YES&gdal:co:BIGTIFF=YES&gdal:co:COMPRESS=DEFLATE&gdal:co:ZLEVEL=9";
/* from table-ll.c de FORCE
+++ Sentinel-2A MSI:
+++    30: AEROSOL, 31: BLUE, 32: GREEN, 33: RED,
+++    34: REDEDGE1, 35: REDEDGE2, 36: NIREDGE, 37: LAI, 38: NIR
+++    39: WVP, 40: CIRRUS, 41: SWIR1, 42: SWIR2
FROM
*/
//std::map<char, char> mIndexName2Band = {{'BLU', 'B02'}, {'GRN', 'B03'}, {'RED', 'B04'},  {'RE1', 'B05'}, {'RE2', 'B06'}, {'RE3', 'B07'}, {'NIR', 'B08'}, {'N1N', 'B8A'}, {'SWIR1', 'B11'}, {'SWIR2', 'B12'}};
std::map<std::string, std::string> mIndexName2Band = {{"BLU", "B02"}, {"GRN", "B03"}, {"RED", "B04"},  {"RE1", "B05"}, {"RE2", "B06"}, {"RE3", "B07"}, {"NIR", "B08"}, {"N1N", "B8A"}, {"SW1", "B11"}, {"SW2", "B12"}, {"NDV", "NDV"}, {"CSW", "CSW"}};
std::vector bandsName{"BLUE","GREEN","RED","REDEDGE1","REDEDGE2","REDEDGE3","BROADNIR","NIR","SWIR1","SWIR2"};

dc::dc(par_hl_t *param, int &x, int &y):phl(param), tileX(x), tileY(y)
{
    char fname[NPOW_10];
    char tilename[NPOW_10];
    snprintf(fname, NPOW_10, "%s/X%04d_Y%04d", phl->d_lower, tileX, tileY);
    snprintf(tilename, NPOW_10, "X%04d_Y%04d", tileX, tileY);
    //std::cout << fname << std::endl;

    name=tilename;
    std::cout << tilename << std::endl;
    dirName=fname;
    snprintf(fname, NPOW_10, "%s/X%04d_Y%04d", phl->d_higher, tileX, tileY);
    dirHighLev=fname;
}

bool dc::exist(){
    return fs::exists(dirHighLev);

}

void dc::getTransform(double * tr){
    char reclassFile[NPOW_10];
    snprintf(reclassFile, NPOW_10, "%s/X%04d_Y%04d/%s.tif", phl->d_mask, tileX, tileY, "classDepe");
    GDALDataset * maskRaster = (GDALDataset *) GDALOpen( reclassFile, GA_ReadOnly );
    maskRaster->GetGeoTransform(tr);
    GDALClose(maskRaster);
}


void dc::genClassRaster(GDALDataset *DShouppiers, GDALDataset *DSzone, std::string aOut){
    std::cout << "genClassRaster for tile " << dirName << std::endl;
    char out[NPOW_10], mask[NPOW_10], reclassFile[NPOW_10];
    snprintf(out, NPOW_10, "%s/X%04d_Y%04d/%s.tif", phl->d_mask, tileX, tileY, "classDepePI");
    snprintf(reclassFile, NPOW_10, "%s/X%04d_Y%04d/%s.tif", phl->d_mask, tileX, tileY, aOut.c_str());
    snprintf(mask, NPOW_10, "%s/X%04d_Y%04d/%s", phl->d_mask, tileX, tileY, phl->b_mask);

    GDALDriver *pDriver;
    GDALDataset *pOut=NULL;
    //GDALDataset *pOutReclass=NULL;
    const char *pszFormat = "GTiff";
    pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);

    // création du raster de résultat
    GDALDataset * maskRaster = (GDALDataset *) GDALOpen( mask, GA_ReadOnly );
    int nx=maskRaster->GetRasterBand(1)->GetXSize();
    int ny=maskRaster->GetRasterBand(1)->GetYSize();
    double tr[6];
    maskRaster->GetGeoTransform(tr);

    pOut = pDriver->Create(out, nx,ny, 7, GDT_Byte, NULL);
    pOut->SetProjection( maskRaster->GetProjectionRef() );
    pOut->SetGeoTransform(tr);

    // je commence par les zones PI, en band numéro 1
    char ** argv = NULL;
    //ALL_TOUCHED option
    argv = CSLAddString( argv, "-at" );
    argv = CSLAddString( argv, "-l" );
    argv = CSLAddString( argv, DSzone->GetLayer(0)->GetName() );
    argv = CSLAddString( argv, "-b" );
    argv = CSLAddString( argv, "1");
    argv = CSLAddString( argv, "-burn" );
    argv = CSLAddString( argv, "1" );

    GDALRasterizeOptions * pOptions = GDALRasterizeOptionsNew( argv, NULL );
    std::cout << "run rasterized zone PI" << std::endl;
    GDALRasterize(NULL,pOut,DSzone,pOptions,NULL);
    for (int depe(0);depe<6;depe++){
        int bandNum=depe+3;
        // reorder band by dieback intensity
        if (depe==4){bandNum=2;}
        if (depe==5){bandNum=7;}// ce sont les "coupés" détecté manuellement avec diff de MNH
        argv = NULL;
        //ALL_TOUCHED option
        argv = CSLAddString( argv, "-at" );
        argv = CSLAddString( argv, "-l" );
        argv = CSLAddString( argv, DShouppiers->GetLayer(0)->GetName() );
        argv = CSLAddString( argv, "-b" );
        argv = CSLAddString( argv, std::to_string(bandNum).c_str());
        argv = CSLAddString( argv, "-where" );
        argv = CSLAddString( argv, (std::string("num_value=")+std::to_string(depe)).c_str() );
        argv = CSLAddString( argv, "-burn" );
        argv = CSLAddString( argv, "1" );

        /* attention, si j'utilise l'argument init gdal ne veux pas modifier un raster existant et veux en recréer un. mais il a besoin de la taille en pixel du coup.
        argv = CSLAddString( argv, "-init" );
        argv = CSLAddString( argv, "0" );
        pRasterized=static_cast<GDALDataset *>(GDALRasterize(rasterizedFile,NULL,lay,pOptions,NULL));
        */
        GDALRasterizeOptions * pOptions = GDALRasterizeOptionsNew( argv, NULL );
        std::cout << "run rasterized houppiers PI" << std::endl;
        GDALRasterize(NULL,pOut,DShouppiers,pOptions,NULL);

        // maintenant je lis tout les pixels du rasterize et je calcule un pourcentage de surface pour le pixel
        float *scanPix = (float *) CPLMalloc( sizeof( float ) * 1 );
        for ( int row = 0; row < ny; row++ ){
            for (int col = 0; col < nx; col++)
            {
                pOut->GetRasterBand(bandNum)->RasterIO( GF_Read, col, row, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
                if (scanPix[0]==1){
                    OGRLinearRing * ring = new OGRLinearRing();
                    OGRPolygon * square= new OGRPolygon();
                    OGRFeature *poFeature;
                    ring->addPoint(tr[0]+col*tr[1], tr[3]+row*tr[5]);
                    ring->addPoint(tr[0]+(col+1)*tr[1], tr[3]+row*tr[5]);
                    ring->addPoint(tr[0]+(col+1)*tr[1], tr[3]+(row+1)*tr[5]);
                    ring->addPoint(tr[0]+(col)*tr[1], tr[3]+(row+1)*tr[5]);
                    ring->closeRings();
                    square->addRingDirectly(ring);
                    DShouppiers->GetLayer(0)->SetSpatialFilter(square);
                    double areaIntersect(0);
                    while( (poFeature = DShouppiers->GetLayer(0)->GetNextFeature()) != NULL )
                    {
                        int dep=poFeature->GetFieldAsInteger("num_value");
                        if (dep==depe){
                            areaIntersect+=poFeature->GetGeometryRef()->Intersection(square)->toMultiPolygon()->get_Area();
                        }
                    }
                    DShouppiers->GetLayer(0)->SetSpatialFilter(NULL);
                    DShouppiers->GetLayer(0)->ResetReading();

                    scanPix[0]=areaIntersect;
                    pOut->GetRasterBand(bandNum)->RasterIO( GF_Write, col, row, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
                }
            }
        }
        CPLFree(scanPix);
    }

    GDALClose(pOut);
    // GDALClose(pRasterized);
    GDALClose(maskRaster);

    // call à OTB pour reclassifier en 3 classes
    //std::string exp("(im1b2/5.5+im1b3*2.0/5.0 +im1b4*3.0/5.0 +im1b5*4.0/5.0 +im1b6)>30 ? 2 : (im1b2/5.5+im1b3*2.0/5.0 +im1b4*3.0/5.0 +im1b5*4.0/5.0 +im1b6)>0 ? 3 : im1b1 ==1 ? 1 : 0");

    // classe depe 4 0_10 % -> je retire
    // classe depe 0 10-25 % -> je les retire car ambigu
    // classe depe 1 25-50 %-> compte pour 1/2
    // classe depe 2, 3, 5(coupé) -> compte pour 1
    //std::string exp("(im1b4*2.5/5.0 +im1b5 +im1b6+im1b7)>30 ? 2 : (im1b2+im1b3+im1b4+im1b5+im1b6+im1b7)>10 ? 3 : im1b1 ==1 ? 1 : 0");
    //std::string exp("(im1b2+im1b3+im1b4+im1b5 +im1b6+im1b7)>35 ? 2 : (im1b2+im1b3+im1b4+im1b5+im1b6+im1b7)>5 ? 3 : im1b1 ==1 ? 1 : 0");

    //std::string exp("(im1b6+im1b7)>35 ? 2 : (im1b2+im1b3+im1b4+im1b5+im1b6+im1b7)>5 ? 3 : im1b1 ==1 ? 1 : 0");
    std::string exp("(im1b3+im1b4+im1b5+im1b6+im1b7)>40 ? 2 : (im1b2+im1b3+im1b4+im1b5+im1b6+im1b7)>5 ? 3 : im1b1 ==1 ? 1 : 0");

    // je souhaite cartographier les arbres morts sur pied pour le projet OGF. test d'utilisation mon dataset dépé chêne, qui est quand même assez conséquent.
     exp="(A[5]+A[6])>35 ? 2 : (A[2]+A[3]+A[4]+A[5]+A[6]+A[7])>10 ? 3 : A[1] ==1 ? 1 : 0";

    std::string aCommand(std::string("gdal raster calc -i ")+out+" --calc '"+exp+"' --ot Int16 --co 'COMPRESS=DEFLATE' --overwrite -o "+ reclassFile);
  // il faut définir le type, int16
    //gdal raster calc -i "A=classDepePI.tif" --calc "(A[5]+A[6])>35 ? 2 : (A[2]+A[3]+A[4]+A[5]+A[6]+A[7])>10 ? 3 : A[1] ==1 ? 1 : 0" -o test.tif

    std::cout << aCommand << std::endl;
    system(aCommand.c_str());
}


std::map<std::tuple<int, int>, std::vector<double>> dc::exportIndex2txt(int idx, bool inverseUV){
    std::cout<< "export TS for index " << phl->tsa.index_name[idx] << std::endl;
    // conteneur pour la TS, seulement les pixels qui m'intéressent
    std::map<std::tuple<int, int>, std::vector<double>> mTS;
    char fname[NPOW_10];
    snprintf(fname, NPOW_10, "%s/%04d%02d%02d-%04d%02d%02d_%03d-%03d_HL_TSA_%s_%s_TSI.tif",
             dirHighLev.c_str(),
             phl->date_range[_MIN_].year, phl->date_range[_MIN_].month, phl->date_range[_MIN_].day,
             phl->date_range[_MAX_].year, phl->date_range[_MAX_].month, phl->date_range[_MAX_].day,
             phl->doy_range[_MIN_], phl->doy_range[_MAX_],
             phl->sen.target, phl->tsa.index_name[idx]);

    int step(100);

    if (fs::exists(fname)){
        // ouvre raster série tempo et raster de masque/classe en //
        GDALDataset * ts_index = (GDALDataset *) GDALOpen( fname, GA_ReadOnly );
        char reclassFile[NPOW_10];
        snprintf(reclassFile, NPOW_10, "%s/X%04d_Y%04d/%s.tif", phl->d_mask, tileX, tileY, "classDepe");
        GDALDataset * segLab = (GDALDataset *) GDALOpen( reclassFile, GA_ReadOnly );

        float *lineSegLab = (float *) CPLMalloc( sizeof( float ) * segLab->GetRasterXSize() );
        float *lineIndex = (float *) CPLMalloc( sizeof( float ) * segLab->GetRasterXSize() );

        for ( int row = 0; row < segLab->GetRasterYSize(); row++ ){
            if (row%step==0){
                std::cout << row << "n rows" << std::endl;
            }

            segLab->GetRasterBand(1)->RasterIO( GF_Read, 0, row, segLab->GetRasterXSize(), 1, lineSegLab, segLab->GetRasterXSize(),1, GDT_Float32, 0, 0 );

            // test pour voir si au moins un pixel d'intérêt dans la ligne
            bool test(0);
            for (int col = 0; col < segLab->GetRasterYSize(); col++)
            { if(lineSegLab[col]>0){test=1; break;}}

            // boucle sur toutes les bandes de la série temporelle et lecture
            if (test){
                //  std::cout << "got one line " <<std::endl;
                for (int band(1);band < ts_index->GetBands().size()+1 ; band++ ){
                    ts_index->GetRasterBand(band)->RasterIO( GF_Read, 0, row, segLab->GetRasterXSize(), 1, lineIndex, segLab->GetRasterXSize(),1, GDT_Float32, 0, 0 );

                    for (int col = 0; col < segLab->GetRasterXSize(); col++)
                    {
                        if (lineSegLab[col]>0){

                            std::tuple<int, int> pixId=std::make_tuple(col,row);
                            if (inverseUV){
                                pixId=std::make_tuple(row,col);
                            }
                            if (mTS.find(pixId)!=mTS.end()){
                                mTS.at(pixId).push_back(lineIndex[col]);
                            } else {
                                mTS.emplace(std::make_pair(pixId, std::vector<double>{lineSegLab[col],lineIndex[col]}));
                            }

                        }
                    }
                }
            }
        }

        CPLFree(lineIndex);
        CPLFree(lineSegLab);
        GDALClose(segLab);
        GDALClose(ts_index);

    } else {
        std::cout<< "file " << fname << " do not exist" <<std::endl;
    }
    return mTS;
}




void dc::exportIndex2Sits_cube(std::string dirOut,int idx){
    std::cout<< "exportIndex2Sits_cube " << phl->tsa.index_name[idx] << " to directory " << dirOut << std::endl;

    std::string bandName=mIndexName2Band.at(phl->tsa.index_name[idx]);
    std::cout<< "band name is  " << bandName << std::endl;
    char fname[NPOW_10];
    char dateDesc[NPOW_10];
    snprintf(fname, NPOW_10, "%s/%04d%02d%02d-%04d%02d%02d_%03d-%03d_HL_TSA_%s_%s_TSI.tif",
             dirHighLev.c_str(),
             phl->date_range[_MIN_].year, phl->date_range[_MIN_].month, phl->date_range[_MIN_].day,
             phl->date_range[_MAX_].year, phl->date_range[_MAX_].month, phl->date_range[_MAX_].day,
             phl->doy_range[_MIN_], phl->doy_range[_MAX_],
             phl->sen.target, phl->tsa.index_name[idx]);

    // j'ouvre l'input uniquement pour avoir la transformation
    GDALDataset * inRast = (GDALDataset *) GDALOpen( fname, GA_ReadOnly );
    double tr[6];
    inRast->GetGeoTransform(tr);
    GDALClose(inRast);

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

        // approche vrt
        const char *vrt_xml = CPLSPrintf("<VRTDataset rasterXSize=\"500\" rasterYSize=\"500\">"
                                         "<SRS dataAxisToSRSAxisMapping=\"1,2\">PROJCS[\"BD72 / Belgian Lambert 72\",GEOGCS[\"BD72\",DATUM[\"Reseau_National_Belge_1972\",SPHEROID[\"International 1924\",6378388,297,AUTHORITY[\"EPSG\",\"7022\"]],AUTHORITY[\"EPSG\",\"6313\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4313\"]],PROJECTION[\"Lambert_Conformal_Conic_2SP\"],PARAMETER[\"latitude_of_origin\",90],PARAMETER[\"central_meridian\",4.36748666666667],PARAMETER[\"standard_parallel_1\",51.1666672333333],PARAMETER[\"standard_parallel_2\",49.8333339],PARAMETER[\"false_easting\",150000.013],PARAMETER[\"false_northing\",5400088.438],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH],AUTHORITY[\"EPSG\",\"31370\"]]</SRS>"
                                         "<GeoTransform>  %.2f,  1.0000000000000000e+01,  0.0000000000000000e+00,  %.2f,  0.0000000000000000e+00, -1.0000000000000000e+01</GeoTransform>"
                                         "<Metadata>"
                                         "  <MDI key=\"AREA_OR_POINT\">Area</MDI>"
                                         "</Metadata>"
                                         "<Metadata domain=\"IMAGE_STRUCTURE\">"
                                         "<MDI key=\"COMPRESSION\">LZW</MDI>"
                                         "<MDI key=\"INTERLEAVE\">BAND</MDI>"
                                         "</Metadata>"
                                         "<VRTRasterBand dataType=\"Int16\" band=\"1\" blockYSize=\"250\">"
                                         "<Description>%s</Description>"
                                         "<NoDataValue>-9999</NoDataValue>"
                                         "<ColorInterp>Gray</ColorInterp>"
                                         "<SimpleSource>"
                                         "<SourceFilename relativeToVRT=\"1\">%s</SourceFilename>"
                                         "<SourceBand>%d</SourceBand>"
                                         "<SourceProperties RasterXSize=\"500\" RasterYSize=\"500\" DataType=\"Int16\" BlockXSize=\"500\" BlockYSize=\"250\" />"
                                         "<SrcRect xOff=\"0\" yOff=\"0\" xSize=\"500\" ySize=\"500\" />"
                                         "<DstRect xOff=\"0\" yOff=\"0\" xSize=\"500\" ySize=\"500\" />"
                                         "</SimpleSource>"
                                         "</VRTRasterBand>"
                                         "</VRTDataset>",tr[0],tr[3],dateDesc, fname, t+1);
        GDALDataset * vrt_ds = (GDALDataset *) GDALOpen(vrt_xml, GA_ReadOnly);
        const char *pszFormat = "GTiff";
        GDALDriver *pDriver =GetGDALDriverManager()->GetDriverByName(pszFormat);
        GDALDataset * out = pDriver->CreateCopy( outName, vrt_ds,FALSE, NULL,NULL, NULL ) ;
        GDALClose(vrt_ds);
        GDALClose(out);
    }

}

// ajoute la série temporelle dans un DC sits de raster existant en commancant à la position posPix0
int dc::writeIndexInSits_cube(int idx, std::string outDir, int posPix0){

    std::cout<< "writeIndexInSits_cube, index " << phl->tsa.index_name[idx] << " and pos first pixel " << posPix0 << std::endl;
    char fname[NPOW_10];
    snprintf(fname, NPOW_10, "%s/%04d%02d%02d-%04d%02d%02d_%03d-%03d_HL_TSA_%s_%s_TSI.tif",
             dirHighLev.c_str(),
             phl->date_range[_MIN_].year, phl->date_range[_MIN_].month, phl->date_range[_MIN_].day,
             phl->date_range[_MAX_].year, phl->date_range[_MAX_].month, phl->date_range[_MAX_].day,
             phl->doy_range[_MIN_], phl->doy_range[_MAX_],
             phl->sen.target, phl->tsa.index_name[idx]);

    int step(500);
    int posPix=posPix0;

    if (fs::exists(fname)){
        // ouvre raster série tempo et raster de masque/classe en //
        GDALDataset * ts_index = (GDALDataset *) GDALOpen( fname, GA_ReadOnly );
        char reclassFile[NPOW_10];
        snprintf(reclassFile, NPOW_10, "%s/X%04d_Y%04d/%s.tif", phl->d_mask, tileX, tileY, "classDepe");
        GDALDataset * segLab = (GDALDataset *) GDALOpen( reclassFile, GA_ReadOnly );

        float *lineSegLab = (float *) CPLMalloc( sizeof( float ) * segLab->GetRasterXSize() );
        float *lineIndex = (float *) CPLMalloc( sizeof( float ) * segLab->GetRasterXSize() );
        float *scanPix = (float *) CPLMalloc( sizeof( float ) * 1 );

        for ( int row = 0; row < segLab->GetRasterYSize(); row++ ){

            if (row%step==0){
                std::cout << row << "n rows" << std::endl;
            }

            segLab->GetRasterBand(1)->RasterIO( GF_Read, 0, row, segLab->GetRasterXSize(), 1, lineSegLab, segLab->GetRasterXSize(),1, GDT_Float32, 0, 0 );

            // test pour voir si au moins un pixel d'intérêt dans la ligne
            bool test(0);
            for (int col = 0; col < segLab->GetRasterYSize(); col++)
            { if(lineSegLab[col]>0){test=1; break;}}

            // boucle sur toutes les bandes de la série temporelle et lecture
            if (test){
                int nbInLine(0);
                for (int band(1);band < ts_index->GetBands().size()+1 ; band++ ){
                    nbInLine=0;
                    date_t date;
                    set_date_ce(&date, phl->date_range[_MIN_].ce + (band-1)*phl->tsa.tsi.step);

                    // char index2Name[NPOW_10];
                    //snprintf(index2Name, NPOW_10, "%s",phl->tsa.index_name[idx]);
                    // std::string index2Name=std::string(phl->tsa.index_name[idx]);
                    if (mIndexName2Band.find(phl->tsa.index_name[idx])==mIndexName2Band.end()){
                        //index2Name=mIndexName2Band.at(phl->tsa.index_name[idx]).c_str();
                        std::cout << " ca va pas aller!!" << std::endl;
                    }

                    char rastSitsName[NPOW_10];
                    snprintf(rastSitsName, NPOW_10, "%s/SEN2L_FORCETSI_T1_%s_%04d-%02d-%02d.tif",outDir.c_str(),mIndexName2Band.at(phl->tsa.index_name[idx]).c_str(),date.year, date.month, date.day);
                    GDALDataset * rastSitsCube = (GDALDataset *) GDALOpen( rastSitsName, GA_Update);

                    // lecture d'une ligne
                    ts_index->GetRasterBand(band)->RasterIO( GF_Read, 0, row, segLab->GetRasterXSize(), 1, lineIndex, segLab->GetRasterXSize(),1, GDT_Float32, 0, 0 );

                    for (int col = 0; col < segLab->GetRasterXSize(); col++)
                    {
                        if (lineSegLab[col]>0){
                            // ouvre le raster sits cube
                            int rowDC= std::ceil((posPix+nbInLine)/rastSitsCube->GetRasterXSize());
                            // si posPix=0, ça ne fait -1.
                            int colDC= (posPix+nbInLine)-(rowDC*rastSitsCube->GetRasterXSize());
                            scanPix[0]=lineIndex[col];
                            rastSitsCube->GetRasterBand(1)->RasterIO( GF_Write, colDC, rowDC, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
                            //if (band==1){ std::cout << " pos pixel is " << posPix+nbInLine << std::endl;}
                            nbInLine++;
                        }
                    }
                    GDALClose(rastSitsCube);

                }
                posPix=posPix+nbInLine;
            }
        }

        CPLFree(lineIndex);
        CPLFree(lineSegLab);
        GDALClose(segLab);
        GDALClose(ts_index);

    } else {
        std::cout<< "file " << fname << " do not exist" <<std::endl;
    }
    return (posPix-posPix0);
}

int dc::maskStat(){
    int aRes(0);
    char maskFile[NPOW_10];

    snprintf(maskFile, NPOW_10, "%s/X%04d_Y%04d/%s", phl->d_mask, tileX, tileY, phl->b_mask);
    if (fs::exists(maskFile)){
        GDALDataset * mask = (GDALDataset *) GDALOpen( maskFile, GA_ReadOnly );

        float *line = (float *) CPLMalloc( sizeof( float ) * mask->GetRasterXSize() );

        for ( int row = 0; row < mask->GetRasterYSize(); row++ ){
            mask->GetRasterBand(1)->RasterIO( GF_Read, 0, row, mask->GetRasterXSize(), 1, line, mask->GetRasterXSize(),1, GDT_Float32, 0, 0 );
            for (int col = 0; col < mask->GetRasterYSize(); col++){

                if (line[col]==1){
                    aRes++;
                }
            }
        }

        CPLFree(line);
        GDALClose(mask);
    }

    return aRes;
}

void dc::l3tol2(std::string dirOut){
    char fname[NPOW_10];
    char outname[NPOW_10];
    char dateDesc[NPOW_10];
    char templateQAI[NPOW_10];
    std::string aCommand("");

    // loop on time, then on index

    int ni = std::ceil((phl->date_range[_MAX_].ce-phl->date_range[_MIN_].ce+1)/phl->tsa.tsi.step);
    std::cout << "dc l3 to l2, number of dates : " << ni << std::endl;
    for (int t=0; t<ni; t++){

        //date ce : current era
        date_t date;
        set_date_ce(&date, phl->date_range[_MIN_].ce + t*phl->tsa.tsi.step);
        snprintf(dateDesc, NPOW_10, "%04d%02d%02d",date.year, date.month, date.day);
        // pas intuitif, mais le paramètre -b dois être utilisé une seule et unique fois sinon remet les bandes +ieures fois
        aCommand="gdal raster stack --overwrite -b "+ std::to_string(t+1)+" ";

        for (int idx=0; idx<phl->tsa.n; idx++){

            /* old ; work on exploded TSI, mais je ne veux pas les générer en exploded systématiquement.
            snprintf(fname, NPOW_10, "%s/%04d%02d%02d-%04d%02d%02d_%03d-%03d_HL_TSA_%s_%s_TSI_%s.tif",
                     dirHighLev.c_str(),
                     phl->date_range[_MIN_].year, phl->date_range[_MIN_].month, phl->date_range[_MIN_].day,
                     phl->date_range[_MAX_].year, phl->date_range[_MAX_].month, phl->date_range[_MAX_].day,
                     phl->doy_range[_MIN_], phl->doy_range[_MAX_],
                     phl->sen.target, phl->tsa.index_name[idx], dateDesc);
            aCommand+= std::string(fname)+" ";
            */
            snprintf(fname, NPOW_10, "%s/%04d%02d%02d-%04d%02d%02d_%03d-%03d_HL_TSA_%s_%s_TSI.tif",
                     dirHighLev.c_str(),
                     phl->date_range[_MIN_].year, phl->date_range[_MIN_].month, phl->date_range[_MIN_].day,
                     phl->date_range[_MAX_].year, phl->date_range[_MAX_].month, phl->date_range[_MAX_].day,
                     phl->doy_range[_MIN_], phl->doy_range[_MAX_],
                     phl->sen.target, phl->tsa.index_name[idx]);

            aCommand+=  std::string(fname)+" ";

            if (!fs::exists(fname)){break;
            std::cout << " file do not exists!" << std::endl;}

            if (t==0 & idx==0){
                // création d'un unique tif pour le QAI qui sert de ref pour tout les vrt
                snprintf(templateQAI, NPOW_10, "%s/%s/QAI_%s.tif",
                         dirOut.c_str(),
                         name.c_str(),
                         phl->tsa.index_name[idx]);
                std::string aCommand2 ="gdal raster create -i "+std::string(fname)+ " --overwrite --burn 0 --band-count 1 -o "+ std::string(templateQAI);
                //std::cout << aCommand2   << std::endl;
                system(aCommand2.c_str());
            }
        }

        // il faut le mm nombre de bande que pour force level2 SENSOR SEN2A
        for (int i(0); i<10-phl->tsa.n;i++ ){
            //aCommand+= std::string(fname)+" ";
            aCommand+= std::string(fname)+" ";
        }

        // output: LEVEL2 force convention for BOA. Attention, le nom du sensor entre l2 et l3 n'est plus le meme arggg!!!!!!!!!!!
        snprintf(outname, NPOW_10, "%s/%s/%s_LEVEL2_SEN2A_BOA.vrt",
                 dirOut.c_str(),
                 name.c_str(),
                 dateDesc);
        aCommand+= outname;
        if (t==ni-1){
        std::cout << aCommand   << std::endl;
        }
        system(aCommand.c_str());

        /* USELESS
       GDALDataset * rast = (GDALDataset *) GDALOpen(fname, GA_Update);
       for (int band(1);band < rast->GetBands().size()+1 ; band++ ){
        rast->GetRasterBand(band)->SetDescription(bandsName.at(band-1));
       }
       GDALClose(rast);
       */

        snprintf(outname, NPOW_10, "%s/%s/%s_LEVEL2_SEN2A_QAI.vrt",
                 dirOut.c_str(),
                 name.c_str(),
                 dateDesc);

        // ce que veux force c'est un raster de QAI également, sinon ne démarre pas
        aCommand = "gdalbuildvrt --quiet "+std::string(outname) +" "+std::string(templateQAI);
        system(aCommand.c_str());
    }
}
