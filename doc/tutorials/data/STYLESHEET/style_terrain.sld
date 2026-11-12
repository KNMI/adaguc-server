<?xml version="1.0" encoding="UTF-8"?>
<StyledLayerDescriptor version="1.1.0" xmlns="http://www.opengis.net/sld" xmlns:se="http://www.opengis.net/se">
  <NamedLayer>
    <se:Name>ahn_utrechtseheuvelrug_500m</se:Name>
    <UserStyle>
      <se:Name>terrain</se:Name>
      <IsDefault>1</IsDefault>
      <se:CoverageStyle>
        <se:Rule>
          <AdagucSymbolizer xmlns="https://github.com/KNMI/adaguc-server">
            <Legend name="terrain" type="colorRange">
              <palette index="0"   color="#2B83BA"/>
              <palette index="20"  color="#ABDDA4"/>
              <palette index="60"  color="#FFFFBF"/>
              <palette index="140" color="#D7A35B"/>
              <palette index="240" color="#7A3B10"/>
            </Legend>
            <Style name="terrain">
              <Legend fixedclasses="true" tickinterval="10">terrain</Legend>
              <Min>-5</Min>
              <Max>75</Max>
              <RenderSettings interpolationmethod="bilinear"/>
            </Style>
          </AdagucSymbolizer>
        </se:Rule>
      </se:CoverageStyle>
    </UserStyle>
  </NamedLayer>
</StyledLayerDescriptor>
