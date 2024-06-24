#! /bin/bash

/home/gef/app/build-s2_ts/s2_timeSerie --xmlIn /home/gef/app/s2/xml/s2_sco_GE.xml --mode 1
/home/gef/app/build-s2_ts/s2_timeSerie --xmlIn /home/gef/app/s2/xml/s2_sco_GE.xml --mergeES 1 --anaTS 0

/home/gef/app/build-s2_postProcess/s2_postProcess --outils 0 --rasterIn /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/etatSanitaire_ANNEEGE.tif --seuilPP 1 --probPres /home/gef/Documents/input/merge_compoTS_RF_GE_reclass.tif

/home/gef/app/build-s2_postProcess/s2_postProcess --outils 1 --rasterIn /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/etatSanitaire_ANNEEGE_tmp_masq.tif
# evolution
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 2 --rasterIn /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/etatSanitaire_ANNEEGE_tmp_masq.tif
# compress
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 3 --rasterIn /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/etatSanitaire_ANNEEGE_tmp_masq_evol.tif

# les résultats finaux sont donc ave _co à la fin
mkdir /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/ES_GE
rm /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/ES_GE/*
mv -f /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/*_co.tif /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/ES_GE/

# suppression des résultats intermédiaires
rm /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/merge_*.txt
