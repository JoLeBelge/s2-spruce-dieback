# dc_tool

I discover force-eo and sits [R] packages that are both great. I create a set of tool to manipulate datacube, in particular to go from FORCE format to SITS format and to FORCE-level3 TSI to FORCE-level2 ARD in order to use UDF module

voici le workflow que je met en oeuvre

1) création des TSI pour les indices qui m'intéressent, dir level3-5km
force-higher-level /home/jo/Documents/S2/param/tsi-onlyVIpred.prm

2) mise en forme ARD like, dans le même dossier
./dc_tool --param_hl /home/jo/Documents/S2/param/tsi-onlyVIpred.prm --dirOut /toto --mode 5

3) UDF block pour prédiction avec tempCNN
export R_HOME=$(R RHOME)
export LD_LIBRARY_PATH=$R_HOME/lib
force-higher-level /home/jo/Documents/S2/param/udf-block.prm



