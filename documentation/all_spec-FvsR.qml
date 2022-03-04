<!DOCTYPE qgis PUBLIC 'http://mrcc.com/qgis.dtd' 'SYSTEM'>
<qgis minScale="1e+08" version="3.18.2-Zürich" styleCategories="AllStyleCategories" hasScaleBasedVisibilityFlag="0" maxScale="0">
  <flags>
    <Identifiable>1</Identifiable>
    <Removable>1</Removable>
    <Searchable>0</Searchable>
    <Private>0</Private>
  </flags>
  <temporal mode="0" enabled="0" fetchMode="0">
    <fixedRange>
      <start></start>
      <end></end>
    </fixedRange>
  </temporal>
  <customproperties>
    <property key="WMSBackgroundLayer" value="false"/>
    <property key="WMSPublishDataSourceUrl" value="false"/>
    <property key="embeddedWidgets/count" value="0"/>
    <property key="identify/format" value="Value"/>
  </customproperties>
  <pipe>
    <provider>
      <resampling enabled="false" zoomedInResamplingMethod="nearestNeighbour" maxOversampling="2" zoomedOutResamplingMethod="nearestNeighbour"/>
    </provider>
    <rasterrenderer opacity="1" type="paletted" band="1" alphaBand="-1" nodataColor="">
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
        <paletteEntry color="#ffffff" label="NL - Non_Ligneous" alpha="0" value="0"/>
        <paletteEntry color="#2d6716" label="AU - Others" alpha="255" value="1"/>
        <paletteEntry color="#2d6716" label="BO - Birchs" alpha="255" value="2"/>
        <paletteEntry color="#2d6716" label="CH - Oaks" alpha="255" value="3"/>
        <paletteEntry color="#980c34" label="DO - Douglas_fir" alpha="255" value="4"/>
        <paletteEntry color="#980c34" label="EP - Spruces" alpha="255" value="5"/>
        <paletteEntry color="#2d6716" label="HE - Beech" alpha="255" value="6"/>
        <paletteEntry color="#980c34" label="MZ - Larchs" alpha="255" value="7"/>
        <paletteEntry color="#2d6716" label="PE - Poplars" alpha="255" value="8"/>
        <paletteEntry color="#980c34" label="PI - Pines" alpha="255" value="9"/>
      </colorPalette>
    </rasterrenderer>
    <brightnesscontrast gamma="1" contrast="0" brightness="0"/>
    <huesaturation colorizeStrength="100" colorizeGreen="128" grayscaleMode="0" colorizeOn="0" colorizeRed="255" colorizeBlue="128" saturation="0"/>
    <rasterresampler maxOversampling="2"/>
    <resamplingStage>resamplingFilter</resamplingStage>
  </pipe>
  <blendMode>0</blendMode>
</qgis>
