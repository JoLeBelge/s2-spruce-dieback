The deposit is a compilation of 3 research projects, all related to the use of Sentinel 2 time series.

Each project represent one c++ application. Project managers is Qt (see pro file)

1) s2_ts

Detection of picea abies under stress caused by Ips Typographus using copernicus Sentinel 2 time serie analysis

This application performs the downloading of Sentinel 2 - 2A product, the computation of Complex SWIR Ratio and the comparison of this spectral indice with a theorical CRswir for this date in order to discriminate pixel that are suffering from water stress.

3) s2_postProcess

related to project 1, this application performed post-processing of result from the detection of bark beetle damaged from S2 images. This project require micmac c++ library.

3) s2_carteEss

The idea is to compute one cloud-free mosaic of S2 tiles for every year quarter and then to use this time series of images ( 4 pseudo-dates x 12 bands) in order to train and apply Random Forest  for species classification (or any other classification models). This sofware is twofold. Its first section is devoted to the training of a RF model (optimal sampling of observation balanced for each forest species classes and preparation of data to train RF with the ranger implementation of RF). The second part serves to apply the resulting model, so each pixels requires to run a RF.

 


