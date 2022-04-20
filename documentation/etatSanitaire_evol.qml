<!DOCTYPE qgis PUBLIC 'http://mrcc.com/qgis.dtd' 'SYSTEM'>
<qgis minScale="1e+08" maxScale="0" hasScaleBasedVisibilityFlag="0" version="3.10.4-A Coruña" styleCategories="AllStyleCategories">
  <flags>
    <Identifiable>1</Identifiable>
    <Removable>1</Removable>
    <Searchable>1</Searchable>
  </flags>
  <customproperties>
    <property value="false" key="WMSBackgroundLayer"/>
    <property value="false" key="WMSPublishDataSourceUrl"/>
    <property value="0" key="embeddedWidgets/count"/>
    <property value="Value" key="identify/format"/>
  </customproperties>
  <pipe>
    <rasterrenderer opacity="1" alphaBand="-1" band="1" type="paletted">
      <rasterTransparency/>
      <minMaxOrigin>
        <limits>None</limits>
        <extent>WholeRaster</extent>
        <statAccuracy>Estimated</statAccuracy>
        <cumulativeCutLower>0.02</cumulativeCutLower>
        <cumulativeCutUpper>0.98</cumulativeCutUpper>
        <stdDevFactor>2</stdDevFactor>
      </minMaxOrigin>
      <colorPalette>
        <paletteEntry value="0" color="#ffffff" label="0" alpha="0"/>
        <paletteEntry value="1" color="#7bef4d" label="Epicea sain" alpha="255"/>
        <paletteEntry value="2" color="#ff0501" label="Epicea Scolyté" alpha="255"/>
        <paletteEntry value="3" color="#131dde" label="Coupé" alpha="255"/>
        <paletteEntry value="4" color="#981019" label="Coupe sanitaire" alpha="255"/>
        <paletteEntry value="5" color="#e4ef08" label="Stress passager" alpha="255"/>
        <paletteEntry value="6" color="#efad1d" label="Mélange Pessière-sol-feuillus" alpha="255"/>
        <paletteEntry value="21" color="#a75db9" label="Ancien Epicea scolyté" alpha="255"/>
        <paletteEntry value="22" color="#ff0501" label="Epicea scolyté" alpha="255"/>
        <paletteEntry value="41" color="#3a1f0f" label="Ancienne Coupe sanitaire" alpha="255"/>
        <paletteEntry value="42" color="#981019" label="Coupe sanitaire nouveau scolyte" alpha="255"/>
        <paletteEntry value="43" color="#480649" label="Coupe sanitaire ancien scolyte" alpha="255"/>
      </colorPalette>
      <colorramp name="[source]" type="randomcolors"/>
    </rasterrenderer>
    <brightnesscontrast brightness="0" contrast="0"/>
    <huesaturation colorizeBlue="128" colorizeOn="0" colorizeRed="255" saturation="0" colorizeGreen="128" grayscaleMode="0" colorizeStrength="100"/>
    <rasterresampler maxOversampling="2"/>
  </pipe>
  <blendMode>0</blendMode>
</qgis>
