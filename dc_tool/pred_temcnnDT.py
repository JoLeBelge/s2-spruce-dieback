import sys
import breizhcrops
from breizhcrops.models import TempCNN
import torch
import os
import numpy as np
import re
from osgeo import gdal,osr
from datetime import datetime, timedelta

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
    print("start datacube loading")
    file1 = os.path.join(root, "20170101-20250615_001-365_HL_TSA_SEN2L_NDV_TSI.tif")
    file2 = os.path.join(root, "20170101-20250615_001-365_HL_TSA_SEN2L_CSW_TSI.tif")
    valuesAllTime = None
    if not (os.path.exists(file1) and os.path.exists(file2)):
        print(f"warning: missing files : {file1} / {file2}")
    ds1 = gdal.Open(file1)
    ds2 = gdal.Open(file2)
    for i in range(0,ds1.RasterCount):
        bandR1 = ds1.GetRasterBand(i+1)
        bandR2 = ds2.GetRasterBand(i+1)
        blocval1 = read_band_raster(bandR1, xBlockSize,yBlockSize, xoffset, yoffset)
        blocval2 = read_band_raster(bandR2, xBlockSize, yBlockSize, xoffset, yoffset)   
        valuesOneTime = np.stack((blocval2, blocval1), axis=0)
        if valuesAllTime is None:
            valuesAllTime = valuesOneTime[np.newaxis, ...]
        else:
            # concaténer le long de l'axe 0 (temps) : si valuesAllTime shape (t,2,r,c)
            valuesAllTime = np.concatenate((valuesAllTime, valuesOneTime[np.newaxis, ...]), axis=0)
    print("datacube l3 loaded")
    ds1 = None
    ds2 = None
    return valuesAllTime

pathModel="/home/jo/Documents/S2/deadTree-obAgr"
pathModel="/mnt/wildAIssd/RS/deadTree-obAgr"
path="/mnt/wildAIssd/RS/level3-5km/X0036_Y0019"
templ_name = "SEN2L_FORCETSI_T1_NDV_2017-01-01.tif"
templ_name = "20170101-20250615_001-365_HL_TSA_SEN2L_CSW_TSI.tif"
device="cuda" if torch.cuda.is_available() else "cpu"
#print(ts.shape) --- (192, 2, 500, 50)
modelPath=os.path.join(pathModel, "TempCNN.pt")
model=torch.load(modelPath, map_location=torch.device(device),weights_only=False)
# création du raster de sortie
driver = gdal.GetDriverByName( 'GTiff' )
dst_filename = os.path.join(path, "prediction.tif")
template_filename = os.path.join(path, templ_name)
if not (os.path.exists(template_filename) ):
    raise FileNotFoundError("missing raster template: {template_filename}")
ds = gdal.Open(template_filename)
cols = ds.GetRasterBand(1).XSize
rows = ds.GetRasterBand(1).YSize
bands = 2
dst_ds = driver.Create(dst_filename, cols, rows, bands, gdal.GDT_Byte, options=["INTERLEAVE=PIXEL"])
dst_ds.SetGeoTransform(ds.GetGeoTransform())
dst_ds.SetProjection(ds.GetProjection())
y_block_size=500
x_block_size=50# 500 x 100 c'est déjà trop pour la mémoire GPU (20 Go sur scotty)

for y in range(0, rows, y_block_size):
    for x in range(0, cols, x_block_size):
        print("bloc x", x, "y", y)
# pred shape attendu : (n_pixels, 2) -> on remet en (2, rows, cols)
# adapter l'opération de mise à l'échelle selon vos besoins
        #ts=readDatacubeBloc(path,xBlockSize=x_block_size,yBlockSize=y_block_size,xoffset=x,yoffset=y)
        ts=readDCl3Bloc(path,xBlockSize=x_block_size,yBlockSize=y_block_size,xoffset=x,yoffset=y)
        reshaped=ts.reshape((192,2,ts.shape[2]*ts.shape[3]))
        obs=np.moveaxis(reshaped, [0, 1], [-2, -1])/10000  # shape (192,2,25000) -> (25000,2,192)
        t = torch.from_numpy(obs).type(torch.FloatTensor).to(device)
        predLogSoftMax = model(t)
        pred = torch.exp(predLogSoftMax).to("cpu").detach().numpy()
        arr = (pred * 100).astype(np.uint8)        # scale + cast
       
        arr = arr.T.reshape((bands, ts.shape[2], ts.shape[3]))   

# écrire chaque bande (GDAL bands are 1-indexed)
        for b in range(bands):
            dst_ds.GetRasterBand(b + 1).WriteArray(arr[b, :, :], xoff=x, yoff=y)

        dst_ds.FlushCache()
        del t
        del predLogSoftMax
        del pred
        torch.cuda.empty_cache()
# fermer les datasets
dst_ds = None
ds = None

print("Prediction raster saved to:", dst_filename)