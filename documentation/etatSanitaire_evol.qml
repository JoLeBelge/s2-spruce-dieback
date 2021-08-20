<!DOCTYPE qgis PUBLIC 'http://mrcc.com/qgis.dtd' 'SYSTEM'>
<qgis styleCategories="AllStyleCategories" version="3.18.2-Zürich" maxScale="0" hasScaleBasedVisibilityFlag="0" minScale="1e+08">
  <flags>
    <Identifiable>1</Identifiable>
    <Removable>1</Removable>
    <Searchable>1</Searchable>
    <Private>0</Private>
  </flags>
  <temporal mode="0" fetchMode="0" enabled="0">
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
      <resampling zoomedInResamplingMethod="nearestNeighbour" zoomedOutResamplingMethod="nearestNeighbour" enabled="false" maxOversampling="2"/>
    </provider>
    <rasterrenderer alphaBand="-1" opacity="1" nodataColor="" type="paletted" band="1">
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
        <paletteEntry color="#ffffff" value="0" label="0" alpha="0"/>
        <paletteEntry color="#7bef4d" value="1" label="Epicea sain" alpha="255"/>
        <paletteEntry color="#ff0501" value="2" label="Epicea Scolyté" alpha="255"/>
        <paletteEntry color="#131dde" value="3" label="Coupé" alpha="255"/>
        <paletteEntry color="#981019" value="4" label="Coupe sanitaire" alpha="255"/>
        <paletteEntry color="#e4ef08" value="5" label="Stress passager" alpha="255"/>
        <paletteEntry color="#efad1d" value="6" label="Mélange Pessière-sol-feuillus" alpha="255"/>
        <paletteEntry color="#a75db9" value="21" label="Ancien Epicea scolyté" alpha="255"/>
        <paletteEntry color="#ff0501" value="22" label="Epicea scolyté" alpha="255"/>
        <paletteEntry color="#3a1f0f" value="41" label="Ancienne Coupe sanitaire" alpha="255"/>
        <paletteEntry color="#981019" value="42" label="Coupe sanitaire" alpha="255"/>
      </colorPalette>
      <colorramp name="[source]" type="randomcolors">
        <Option/>
      </colorramp>
    </rasterrenderer>
    <brightnesscontrast contrast="0" brightness="0" gamma="1"/>
    <huesaturation colorizeGreen="128" saturation="0" colorizeBlue="128" grayscaleMode="0" colorizeOn="0" colorizeRed="255" colorizeStrength="100"/>
    <rasterresampler maxOversampling="2"/>
    <resamplingStage>resamplingFilter</resamplingStage>
  </pipe>
  <blendMode>0</blendMode>
</qgis>
