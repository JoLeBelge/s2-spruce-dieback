#! /bin/bash

/home/gef/app/build-s2_ts/s2_timeSerie --xmlIn /home/gef/app/s2/xml/s2_sco.xml --mode 1
#/home/gef/app/build-s2_ts/s2_timeSerie --xmlIn /home/gef/app/s2/xml/s2_sco.xml --mode 2
/home/gef/app/build-s2_ts/s2_timeSerie --xmlIn /home/gef/app/s2/xml/s2_sco.xml --mergeES 1 --anaTS 0

# projection en BL72
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 5 --rasterIn /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/etatSanitaire_ANNEE.tif
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 5 --rasterIn /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/delaisCoupe_ANNEE.tif
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 5 --rasterIn /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/FirstDateSco_ANNEE.tif
# masque
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 0 --rasterIn /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/etatSanitaire_ANNEE_BL72.tif --seuilPP 80 --probPres /home/gef/Documents/input/compo/compo_5EP10_withNDasZero.tif
# nettoyage
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 1 --rasterIn /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/etatSanitaire_ANNEE_BL72_masq.tif
# evolution
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 2 --rasterIn /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/etatSanitaire_ANNEE_BL72_masq.tif
#carte de surveillance nouveau scolyte résolution 5km
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 7 --rasterIn /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffaS2Scolyte/merge/etatSanitaire_ANNEE_BL72_masq_evol_co.tif
# compresse les résultats
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 3 --rasterIn /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/etatSanitaire_ANNEE_BL72_masq_evol.tif
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 3 --rasterIn /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/FirstDateSco_ANNEE_BL72.tif
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 3 --rasterIn /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/delaisCoupe_ANNEE_BL72.tif

# les résultats finaux sont donc ave _co à la fin, ou avec surv
mkdir /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/ES
rm /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/ES/*
mv -f /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/*_co.tif /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/ES/
mv -f /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/*_surv.tif /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/ES/

# suppression des résultats intermédiaires - non j'ose pas
rm /media/gef/598c5e48-4601-4dbf-ae86-d15388a3dffa/S2Scolyte/merge/merge_*.txt
