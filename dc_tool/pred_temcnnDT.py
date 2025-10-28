import sys
import breizhcrops
from breizhcrops.models import TempCNN
import torch
import os
import numpy as np
import re
from osgeo import gdal,osr
from datetime import datetime, timedelta
import configparser
import argparse
import pandas as pd

idxCode=['BLU', 'GRN', 'RED', 'BNR', 'NDV', 'CSW']

def read_raster(raster, x_block_size, y_block_size, x, y):
    ds = gdal.Open(raster)
    if ds is None:
        raise FileNotFoundError(f"cannot open raster: {raster}")
    band = ds.GetRasterBand(1)
    xsize = band.XSize
    ysize = band.YSize
    if y + y_block_size < ysize:
        rows = y_block_size
    else:
        rows = ysize - y
    if x + x_block_size < xsize:
        cols = x_block_size
    else:
        cols = xsize - x
    array = band.ReadAsArray(x, y, cols, rows)
    band = None
    ds = None
    return array

def readDatacubeBloc(root, xBlockSize=100,yBlockSize=500,xoffset=0,yoffset=0):
    print("start datacube loading")
    first_date_str = "2017-01-01"
    date1 = datetime.strptime(first_date_str, "%Y-%m-%d")
    lp = 0
    valuesAllTime = None
    for i in range(192):
        #print("time " + str(i))
        if (i == 73 or i == 164):
            lp += 1
        # A partir de 73 ça bugg à cause de la leap year, Force n'as pas compter le 29 fevrier...
        cur_date = date1 + timedelta(days=(16 * i) + lp)
        file1 = os.path.join(root, "SEN2L_FORCETSI_T1_NDV_" + cur_date.strftime("%Y-%m-%d") + ".tif")
        file2 = os.path.join(root, "SEN2L_FORCETSI_T1_CSW_" + cur_date.strftime("%Y-%m-%d") + ".tif")
        if not (os.path.exists(file1) and os.path.exists(file2)):
            print(f"warning: missing files for {cur_date}: {file1} / {file2}")
            continue
        bandval1 = read_raster(file1, xBlockSize,yBlockSize, xoffset, yoffset)
        bandval2 = read_raster(file2, xBlockSize, yBlockSize, xoffset, yoffset)
        # stack deux bandes pour ce pas de temps : shape -> (2, rows, cols)
        valuesOneTime = np.stack((bandval2, bandval1), axis=0)
        if valuesAllTime is None:
            valuesAllTime = valuesOneTime[np.newaxis, ...]
        else:
            # concaténer le long de l'axe 0 (temps) : si valuesAllTime shape (t,2,r,c)
            valuesAllTime = np.concatenate((valuesAllTime, valuesOneTime[np.newaxis, ...]), axis=0)
    print("datacube from aggregated observation loaded")
    return valuesAllTime

def read_band_raster(band, x_block_size, y_block_size, x, y):
    xsize = band.XSize
    ysize = band.YSize
    if y + y_block_size < ysize:
        rows = y_block_size
    else:
        rows = ysize - y
    if x + x_block_size < xsize:
        cols = x_block_size
    else:
        cols = xsize - x
    array = band.ReadAsArray(x, y, cols, rows)
    band = None
    ds = None
    return array

def readDCl3Bloc(root, xBlockSize=100,yBlockSize=500,xoffset=0,yoffset=0):
    #print("start datacube loading")
    file=[]
    ds=[]
   
    for nidx in range(len(idxCode)):
            file.append(root+"/20170101-20250615_001-365_HL_TSA_SEN2L_"+idxCode[nidx]+"_TSI.tif")
            if (os.path.exists(file[nidx])==False):
                print("file not found: "+file[nidx])
                continue
            ds.append(gdal.Open(file[nidx]))
    
    #file1 = os.path.join(root, "20170101-20250615_001-365_HL_TSA_SEN2L_NDV_TSI.tif")
    #file2 = os.path.join(root, "20170101-20250615_001-365_HL_TSA_SEN2L_CSW_TSI.tif")
    valuesAllTime = None

    #ds1 = gdal.Open(file1)
    #ds2 = gdal.Open(file2)
    for i in range(0,ds[0].RasterCount):
        #bandval=[]
        blocval=[]
        for nidx in range(len(idxCode)):
            #bandval.append(ds[nidx].GetRasterBand(i+1))
            blocval.append(read_band_raster(ds[nidx].GetRasterBand(i+1), xBlockSize,yBlockSize, xoffset, yoffset))
        #bandR2 = ds2.GetRasterBand(i+1)
        #blocval1 = read_band_raster(bandR1, xBlockSize,yBlockSize, xoffset, yoffset)
        #blocval2 = read_band_raster(bandR2, xBlockSize, yBlockSize, xoffset, yoffset)   
        #valuesOneTime = np.stack((blocval2, blocval1), axis=0)
        valuesOneTime = np.stack(blocval, axis=0)
        if valuesAllTime is None:
            valuesAllTime = valuesOneTime[np.newaxis, ...]
        else:
            # concaténer le long de l'axe 0 (temps) : si valuesAllTime shape (t,2,r,c)
            valuesAllTime = np.concatenate((valuesAllTime, valuesOneTime[np.newaxis, ...]), axis=0)
    #print("datacube l3 loaded")
    for nidx in range(len(idxCode)):
        ds[nidx]=None
    #ds1 = None
    #ds2 = None
    return valuesAllTime

def prediction(paramFile):
    config = configparser.ConfigParser()
    config.read(paramFile)
    #templ_name = "SEN2L_FORCETSI_T1_NDV_2017-01-01.tif"
    #templ_name = "20170101-20250615_001-365_HL_TSA_SEN2L_CSW_TSI.tif"
    templ_name = config['DIR']['TEMPLATENAME']
    device="cuda" if torch.cuda.is_available() else "cpu"
    modelPath=config['DEFAULT']['MODELPATH']
    outputName= config['DEFAULT']['OUTPUTNAME']
    dir_lower= config['DIR']['DIR_LOWER']
    dir_higher= config['DIR']['DIR_HIGHER']
    fileListTile=config['TILE']['FILE_TILE']
    x_block_size=int(config['TILE']['X_BLOCK_SIZE'])
    y_block_size=int(config['TILE']['Y_BLOCK_SIZE'])
    x_tile_range_0=int(config['TILE']['X_TILE_RANGE_0'])
    x_tile_range_1=int(config['TILE']['X_TILE_RANGE_1'])
    y_tile_range_0=int(config['TILE']['Y_TILE_RANGE_0'])
    y_tile_range_1=int(config['TILE']['Y_TILE_RANGE_1'])
    # 500 x 100 c'est déjà trop pour la mémoire GPU (20 Go sur scotty)
    debug=config['DEFAULT'].getboolean('DEBUG')
    overwrite=config['DEFAULT'].getboolean('OVERWRITE')
    if not (os.path.exists(fileListTile)):
        print("list of tiles from range")
        tile_rows = []
        for x in range(x_tile_range_0, x_tile_range_1 + 1):
            for y in range(y_tile_range_0, y_tile_range_1 + 1):
                tileName = f"X{x:04d}_Y{y:04d}"
                tilePath = os.path.join(dir_lower, tileName)
                if os.path.exists(tilePath):
                    if overwrite or not os.path.exists(os.path.join(dir_higher,tileName, outputName)):
                        tile_rows.append([tileName])
                else:
                    print(f"warning: missing tile directory: {tilePath}")
        tileList = pd.DataFrame(tile_rows, columns=[0])
    else:   
        print(f"reading tile list from {fileListTile}") 
        tileList = pd.read_csv(fileListTile)
    print("total of ",len(tileList)," tiles")

    model=torch.load(modelPath, map_location=torch.device(device),weights_only=False)
    driver = gdal.GetDriverByName( 'GTiff' )

    for tile_idx in range(tileList.shape[0]):
        tile=tileList.iloc[tile_idx,0]
        print("Processing tile:", tile)
        tilePath=os.path.join(dir_lower, tile)
        dirOut =os.path.join(dir_higher,tile)
        if not os.path.exists(dirOut):
            os.makedirs(dirOut)
        dst_filename= os.path.join(dir_higher,tile, outputName)
        template_filename = os.path.join(tilePath, templ_name)
        if not (os.path.exists(template_filename) ):
            raise FileNotFoundError(f"missing raster template: {template_filename}")
        ds_template = gdal.Open(template_filename)
        cols = ds_template.GetRasterBand(1).XSize
        rows = ds_template.GetRasterBand(1).YSize
        bands = 2
        dst_ds = driver.Create(dst_filename, cols, rows, bands, gdal.GDT_Byte, options=["INTERLEAVE=PIXEL"])
        dst_ds.SetGeoTransform(ds_template.GetGeoTransform())
        dst_ds.SetProjection(ds_template.GetProjection())
        dst_ds.GetRasterBand(1).SetNoDataValue(255)
        dst_ds.GetRasterBand(2).SetNoDataValue(255)

        for y in range(0, rows, y_block_size):
            for x in range(0, cols, x_block_size):
                if debug:
                    print("bloc x", x, "y", y)
            
                #ts=readDatacubeBloc(path,xBlockSize=x_block_size,yBlockSize=y_block_size,xoffset=x,yoffset=y)
                ts=readDCl3Bloc(tilePath,xBlockSize=x_block_size,yBlockSize=y_block_size,xoffset=x,yoffset=y)
                reshaped=ts.reshape((192,len(idxCode),ts.shape[2]*ts.shape[3]))
                obs=np.moveaxis(reshaped, [0, 1], [-2, -1])/10000  # shape (192,2,25000) -> (25000,2,192)
                t = torch.from_numpy(obs).type(torch.FloatTensor).to(device)
                predLogSoftMax = model(t)
                # pred shape attendu : (n_pixels, 2) -> on remet en (2, rows, cols)
                pred = torch.exp(predLogSoftMax).to("cpu").detach().numpy()
                arr = (pred * 100).astype(np.uint8)        # scale + cast
                arr = arr.T.reshape((len(idxCode), ts.shape[2], ts.shape[3]))  
                #noter là ou les inputs étaient des no-data (-9999)  
                m =np.max(ts,axis=0)
                arr2 = np.where(m==-9999,255,arr)

                # écrire chaque bande de résultat
                for b in range(bands):
                    dst_ds.GetRasterBand(b + 1).WriteArray(arr2[b, :, :], xoff=x, yoff=y)

                dst_ds.FlushCache()
                del t
                del predLogSoftMax
                del pred
                torch.cuda.empty_cache()
        # fermer les datasets
        dst_ds = None
        ds_template = None
        if debug:
            print("Prediction raster saved to:", dst_filename)

def parse_args():
    parser = argparse.ArgumentParser(description='Prediction of dead tree from FORCE level-3 datacube with NDVI and CRSWIR Time Serie Interpolation')
    parser.add_argument(
        '-P', '--param', type=str, default="param_pred.ini", help='file with parameters for prediction')
    
    args = parser.parse_args()
    return args


if __name__ == "__main__":
    args = parse_args()

    prediction(args.param)