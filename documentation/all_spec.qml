<!DOCTYPE qgis PUBLIC 'http://mrcc.com/qgis.dtd' 'SYSTEM'>
<qgis minScale="1e+08" hasScaleBasedVisibilityFlag="0" version="3.10.9-A Coruña" maxScale="0" styleCategories="AllStyleCategories">
  <flags>
    <Identifiable>1</Identifiable>
    <Removable>1</Removable>
    <Searchable>1</Searchable>
  </flags>
  <customproperties>
    <property key="WMSBackgroundLayer" value="false"/>
    <property key="WMSPublishDataSourceUrl" value="false"/>
    <property key="embeddedWidgets/count" value="0"/>
    <property key="identify/format" value="Value"/>
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
        <paletteEntry color="#ffffff" label="NL - Non_Ligneous" value="0" alpha="0"/>
        <paletteEntry color="#a6cee3" label="AU - Others" value="1" alpha="255"/>
        <paletteEntry color="#1f78b4" label="BO - Birchs" value="2" alpha="255"/>
        <paletteEntry color="#b2df8a" label="CH - Oaks" value="3" alpha="255"/>
        <paletteEntry color="#33a02c" label="DO - Douglas_fir" value="4" alpha="255"/>
        <paletteEntry color="#fb9a99" label="EP - Spruces" value="5" alpha="255"/>
        <paletteEntry color="#e31a1c" label="HE - Beech" value="6" alpha="255"/>
        <paletteEntry color="#fdbf6f" label="MZ - Larchs" value="7" alpha="255"/>
        <paletteEntry color="#ff7f00" label="PE - Poplars" value="8" alpha="255"/>
        <paletteEntry color="#cab2d6" label="PI - Pines" value="9" alpha="255"/>
      </colorPalette>
      <colorramp type="randomcolors" name="[source]"/>
    </rasterrenderer>
    <brightnesscontrast brightness="0" contrast="0"/>
    <huesaturation colorizeOn="0" colorizeGreen="128" colorizeBlue="128" colorizeRed="255" grayscaleMode="0" saturation="0" colorizeStrength="100"/>
    <rasterresampler maxOversampling="2"/>
  </pipe>
  <blendMode>0</blendMode>
</qgis>
