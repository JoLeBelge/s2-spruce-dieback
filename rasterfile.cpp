#include "rasterfile.h"

rasterFiles::rasterFiles(std::string aPathTif):mPathRaster(aPathTif){
    //std::cout << "rasterFiles " << std::endl;
}

int rasterFiles::getValue(double x, double y,bool quiet){
    int aRes(0);
    if (boost::filesystem::exists(getPathTif())){

        GDALDataset  * mGDALDat = (GDALDataset *) GDALOpen( getPathTif().c_str(), GA_ReadOnly );
        if( mGDALDat == NULL )
        {
            if (!quiet){std::cout << "je n'ai pas lu l'image " << getPathTif() << std::endl;}
        } else {
            // std::cout << "j'ai lu l'image " << getPathTif() << "\n\n\n\n"<< std::endl;
            GDALRasterBand * mBand = mGDALDat->GetRasterBand( 1 );

            double transform[6];
            mGDALDat->GetGeoTransform(transform);
            double xOrigin = transform[0];
            double yOrigin = transform[3];
            double pixelWidth = transform[1];
            double pixelHeight = -transform[5];

            int col = int((x - xOrigin) / pixelWidth);
            int row = int((yOrigin - y ) / pixelHeight);

            if (col<mBand->GetXSize() && col>=0  && row < mBand->GetYSize() && row>=0){
                float *scanPix;
                scanPix = (float *) CPLMalloc( sizeof( float ) * 1 );
                // lecture du pixel
                mBand->RasterIO( GF_Read, col, row, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
                aRes=scanPix[0];
                CPLFree(scanPix);
                mBand=NULL;
            }

            GDALClose( mGDALDat );
        }
    } else if (!quiet){std::cout << "image introuvable : " << getPathTif() << std::endl;}

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

        if (col<mBand->GetXSize() && col>=0  && row < mBand->GetYSize() && row>=0){
            float *scanPix;
            scanPix = (float *) CPLMalloc( sizeof( float ) * 1 );
            // lecture du pixel
            mBand->RasterIO( GF_Read, col, row, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
            aRes=scanPix[0];
            CPLFree(scanPix);

            mBand=NULL;
        }
        GDALClose( mGDALDat );
    }
    return aRes;
}
