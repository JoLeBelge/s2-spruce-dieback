#include "rasterfile.h"

rasterFiles::rasterFiles(std::string aPathTif):mPathRaster(aPathTif){
    //std::cout << "rasterFiles " << std::endl;
}

int rasterFiles::getValue(double x, double y){
    int aRes(0);
    GDALDataset  * mGDALDat = (GDALDataset *) GDALOpen( getPathTif().c_str(), GA_ReadOnly );
    if( mGDALDat == NULL )
    {
        std::cout << "je n'ai pas lu l'image " << getPathTif() << std::endl;
    } else {
        GDALRasterBand * mBand = mGDALDat->GetRasterBand( 1 );

        double transform[6];
        mGDALDat->GetGeoTransform(transform);
        double xOrigin = transform[0];
        double yOrigin = transform[3];
        double pixelWidth = transform[1];
        double pixelHeight = -transform[5];

        int col = int((x - xOrigin) / pixelWidth);
        int row = int((yOrigin - y ) / pixelHeight);

        if (col<mBand->GetXSize() && row < mBand->GetYSize()){
            float *scanPix;
            scanPix = (float *) CPLMalloc( sizeof( float ) * 1 );
            // lecture du pixel
            mBand->RasterIO( GF_Read, col, row, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
            aRes=scanPix[0];
            CPLFree(scanPix);
            GDALClose( mGDALDat );
            mBand=NULL;
        }
    }
    return aRes;
}


double rasterFiles::getValueDouble(double x, double y){

    double aRes(0);
    GDALDataset  * mGDALDat = (GDALDataset *) GDALOpen( getPathTif().c_str(), GA_ReadOnly );
    if( mGDALDat == NULL )
    {
        std::cout << "je n'ai pas lu l'image " << getPathTif() << std::endl;
    } else {
        GDALRasterBand * mBand = mGDALDat->GetRasterBand( 1 );

        double transform[6];
        mGDALDat->GetGeoTransform(transform);
        double xOrigin = transform[0];
        double yOrigin = transform[3];
        double pixelWidth = transform[1];
        double pixelHeight = -transform[5];

        int col = int((x - xOrigin) / pixelWidth);
        int row = int((yOrigin - y ) / pixelHeight);

        if (col<mBand->GetXSize() && row < mBand->GetYSize()){
            float *scanPix;
            scanPix = (float *) CPLMalloc( sizeof( float ) * 1 );
            // lecture du pixel
            mBand->RasterIO( GF_Read, col, row, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
            aRes=scanPix[0];
            CPLFree(scanPix);
            GDALClose( mGDALDat );
            mBand=NULL;
        }
    }
    return aRes;
}

