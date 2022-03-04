<!DOCTYPE qgis PUBLIC 'http://mrcc.com/qgis.dtd' 'SYSTEM'>
<qgis version="3.18.2-Zürich" styleCategories="AllStyleCategories" hasScaleBasedVisibilityFlag="0" minScale="1e+08" maxScale="0">
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
    <property value="false" key="WMSBackgroundLayer"/>
    <property value="false" key="WMSPublishDataSourceUrl"/>
    <property value="0" key="embeddedWidgets/count"/>
    <property value="Value" key="identify/format"/>
  </customproperties>
  <pipe>
    <provider>
      <resampling maxOversampling="2" zoomedOutResamplingMethod="nearestNeighbour" enabled="false" zoomedInResamplingMethod="nearestNeighbour"/>
    </provider>
    <rasterrenderer band="1" nodataColor="" alphaBand="-1" type="paletted" opacity="1">
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
        <paletteEntry alpha="0" value="0" color="#ffffff" label="NL - Non_Ligneous"/>
        <paletteEntry alpha="255" value="1" color="#2d6716" label="AU - Others"/>
        <paletteEntry alpha="255" value="2" color="#2d6716" label="BO - Birchs"/>
        <paletteEntry alpha="255" value="3" color="#2d6716" label="CH - Oaks"/>
        <paletteEntry alpha="255" value="4" color="#980c34" label="DO - Douglas_fir"/>
        <paletteEntry alpha="255" value="5" color="#980c34" label="EP - Spruces"/>
        <paletteEntry alpha="255" value="6" color="#2d6716" label="HE - Beech"/>
        <paletteEntry alpha="255" value="7" color="#2d6716" label="MZ - Larchs"/>
        <paletteEntry alpha="255" value="8" color="#2d6716" label="PE - Poplars"/>
        <paletteEntry alpha="255" value="9" color="#2d6716" label="PI - Pines"/>
      </colorPalette>
      <colorramp name="[source]" type="randomcolors">
        <Option/>
      </colorramp>
    </rasterrenderer>
    <brightnesscontrast brightness="0" contrast="0" gamma="1"/>
    <huesaturation grayscaleMode="0" saturation="0" colorizeBlue="128" colorizeStrength="100" colorizeGreen="128" colorizeOn="0" colorizeRed="255"/>
    <rasterresampler maxOversampling="2"/>
    <resamplingStage>resamplingFilter</resamplingStage>
  </pipe>
  <blendMode>0</blendMode>
</qgis>
