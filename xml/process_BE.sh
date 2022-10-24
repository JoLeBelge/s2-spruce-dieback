#! /bin/bash

/home/gef/app/build-s2_ts/s2_timeSerie --xmlIn ../s2/xml/s2_sco.xml
/home/gef/app/build-s2_ts/s2_timeSerie --xmlIn ../s2/xml/s2_sco.xml --mergeES 1 --anaTS 0

/home/gef/app/build-s2_postProcess/s2_postProcess --outils 5 --rasterIn /media/gef/Data2/S2Scolyte/merge/etatSanitaire_ANNEE.tif
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 5 --rasterIn /media/gef/Data2/S2Scolyte/merge/delaisCoupe_ANNEE.tif
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 5 --rasterIn /media/gef/Data2/S2Scolyte/merge/FirstDateSco_ANNEE.tif
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 0 --rasterIn /media/gef/Data2/S2Scolyte/merge/etatSanitaire_ANNEE_BL72.tif --seuilPP 80 --probPres /home/gef/Documents/input/compo/compo_5_EP10m.tif
# nettoyage
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 1 --rasterIn /media/gef/Data2/S2Scolyte/merge/etatSanitaire_ANNEE_BL72_masq.tif
# evolution
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 2 --rasterIn /media/gef/Data2/S2Scolyte/merge/etatSanitaire_ANNEE_BL72_masq.tif
# compresse les résultats
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 3 --rasterIn /media/gef/Data2/S2Scolyte/merge/etatSanitaire_ANNEE_BL72_masq_evol.tif
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 3 --rasterIn /media/gef/Data2/S2Scolyte/merge/FirstDateSco_ANNEE_BL72.tif
/home/gef/app/build-s2_postProcess/s2_postProcess --outils 3 --rasterIn /media/gef/Data2/S2Scolyte/merge/delaisCoupe_ANNEE_BL72.tif

# les résultats finaux sont donc ave _co à la fin
mkdir /media/gef/Data2/S2Scolyte/merge/ES
mv /media/gef/Data2/S2Scolyte/merge/*_co.tif /media/gef/Data2/S2Scolyte/merge/ES/
# suppression des résultats intermédiaires
rm /media/gef/Data2/S2Scolyte/merge/*.tif
rm /media/gef/Data2/S2Scolyte/merge/*_BL72.tif
rm /media/gef/Data2/S2Scolyte/merge/etatSanitaire_*_BL72_masq.tif
rm /media/gef/Data2/S2Scolyte/merge/etatSanitaire_*_BL72.tif
rm /media/gef/Data2/S2Scolyte/merge/etatSanitaire_*_BL72_masq_clean.tif
rm /media/gef/Data2/S2Scolyte/merge/etatSanitaire_*_BL72_masq_evol.tif
rm /media/gef/Data2/S2Scolyte/merge/merge_*.txt
