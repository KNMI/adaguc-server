const char *WCS_1_0_0_GetCapabilities_Header =
    R""""(<?xml version='1.0' encoding="ISO-8859-1" standalone="no" ?>
<WCS_Capabilities
   version="1.0.0" 
   updateSequence="0" 
   xmlns="http://www.opengis.net/wcs" 
   xmlns:xlink="http://www.w3.org/1999/xlink" 
   xmlns:gml="http://www.opengis.net/gml" 
   xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
   xsi:schemaLocation="http://www.opengis.net/wcs http://schemas.opengis.net/wcs/1.0.0/wcsCapabilities.xsd">
<Service>
  <name>[SERVICENAME]</name>
  <label>[SERVICETITLE]</label>
  <abstract>[SERVICEABSTRACT]</abstract>
  <ServerInfo>[SERVICEINFO]</ServerInfo>
  <fees>NONE</fees>
  <accessConstraints>
    NONE
  </accessConstraints>
</Service>
<Capability>
  <Request>
    <GetCapabilities>
      <DCPType>
        <HTTP>
          <Get><OnlineResource xlink:type="simple" xlink:href="[SERVICEONLINERESOURCE]" /></Get>
        </HTTP>
      </DCPType>
      <DCPType>
        <HTTP>
          <Post><OnlineResource xlink:type="simple" xlink:href="[SERVICEONLINERESOURCE]" /></Post>
        </HTTP>
      </DCPType>
    </GetCapabilities>
    <DescribeCoverage>
      <DCPType>
        <HTTP>
          <Get><OnlineResource xlink:type="simple" xlink:href="[SERVICEONLINERESOURCE]" /></Get>
        </HTTP>
      </DCPType>
      <DCPType>
        <HTTP>
          <Post><OnlineResource xlink:type="simple" xlink:href="[SERVICEONLINERESOURCE]" /></Post>
        </HTTP>
      </DCPType>
    </DescribeCoverage>
    <GetCoverage>
      <DCPType>
        <HTTP>
          <Get><OnlineResource xlink:type="simple" xlink:href="[SERVICEONLINERESOURCE]" /></Get>
        </HTTP>
      </DCPType>
      <DCPType>
        <HTTP>
          <Post><OnlineResource xlink:type="simple" xlink:href="[SERVICEONLINERESOURCE]" /></Post>
        </HTTP>
      </DCPType>
    </GetCoverage>
  </Request>
  <Exception>
    <Format>application/vnd.ogc.se_xml</Format>
  </Exception>
</Capability>
<ContentMetadata>)"""";

const char *WMS_1_0_0_GetCapabilities_Header =
    R""""(<?xml version='1.0' encoding="ISO-8859-1" standalone="no" ?>
<!DOCTYPE WMT_MS_Capabilities SYSTEM "http://schemas.opengis.net/wms/1.0.0/capabilities_1_0_0.dtd"
 [
 <!ELEMENT VendorSpecificCapabilities EMPTY>
 ]>  <!-- end of DOCTYPE declaration -->

<WMT_MS_Capabilities version="1.0.0">
<Service>
  <Name>OGC:WMS</Name>
  <Title>[SERVICETITLE]</Title>
  <Abstract>[SERVICEABSTRACT]</Abstract> <OnlineResource>[SERVICEONLINERESOURCE]</OnlineResource>
  <ServerInfo>[SERVICEINFO]</ServerInfo>
</Service>

<Capability>
  <Request>
    <Map>
      <Format><PNG /><JPEG /><WBMP /><SVG /></Format>
      <DCPType>
        <HTTP>
          <Get onlineResource="[SERVICEONLINERESOURCE]" />
          <Post onlineResource="[SERVICEONLINERESOURCE]" />
        </HTTP>
      </DCPType>
    </Map>
    <Capabilities>
      <Format><WMS_XML /></Format>
      <DCPType>
        <HTTP>
          <Get onlineResource="[SERVICEONLINERESOURCE]" />
          <Post onlineResource="[SERVICEONLINERESOURCE]" />
        </HTTP>
      </DCPType>
    </Capabilities>
    <FeatureInfo>
      <Format><MIME /><GML.1 /></Format>
      <DCPType>
        <HTTP>
          <Get onlineResource="[SERVICEONLINERESOURCE]" />
          <Post onlineResource="[SERVICEONLINERESOURCE]" />
        </HTTP>
      </DCPType>
    </FeatureInfo>
  </Request>
  <Exception>
    <Format><BLANK /><INIMAGE /><WMS_XML /></Format>
  </Exception>
  <VendorSpecificCapabilities />
  <Layer>
    <Title>[GLOBALLAYERTITLE]</Title>)"""";

const char *WMS_1_1_1_GetCapabilities_Header =
    R""""(<?xml version='1.0' encoding="ISO-8859-1" standalone="no" ?>
<!DOCTYPE WMT_MS_Capabilities SYSTEM "http://schemas.opengis.net/wms/1.1.1/WMS_MS_Capabilities.dtd"
 [
 <!ELEMENT VendorSpecificCapabilities EMPTY>
 ]>  <!-- end of DOCTYPE declaration -->

<WMT_MS_Capabilities version="1.1.1">

<Service>
  <Name>OGC:WMS</Name>
  <Title>[SERVICETITLE]</Title>
  <Abstract>[SERVICEABSTRACT]</Abstract>
  <ServerInfo>[SERVICEINFO]</ServerInfo>
  <OnlineResource xmlns:xlink="http://www.w3.org/1999/xlink" xlink:href="[SERVICEONLINERESOURCE]"/>
  <ContactInformation>
  </ContactInformation>
</Service>

<Capability>
  <Request>
    <GetCapabilities>
      <Format>application/vnd.ogc.wms_xml</Format>
      <DCPType>
        <HTTP>
          <Get><OnlineResource xmlns:xlink="http://www.w3.org/1999/xlink" xlink:href="[SERVICEONLINERESOURCE]"/></Get>
        </HTTP>
      </DCPType>
    </GetCapabilities>
    <GetMap>
      <Format>image/png</Format>
      <Format>image/png32</Format>
      <DCPType>
        <HTTP>
          <Get><OnlineResource xmlns:xlink="http://www.w3.org/1999/xlink" xlink:href="[SERVICEONLINERESOURCE]"/></Get>
        </HTTP>
      </DCPType>
    </GetMap>
    <GetFeatureInfo>
      <Format>text/plain</Format>
      <Format>text/html</Format>
      <Format>text/xml</Format>
      <Format>application/vnd.ogc.gml</Format>
      <DCPType>
        <HTTP>
          <Get><OnlineResource xmlns:xlink="http://www.w3.org/1999/xlink" xlink:href="[SERVICEONLINERESOURCE]"/></Get>
        </HTTP>
      </DCPType>
    </GetFeatureInfo>
    <DescribeLayer>
      <Format>text/xml</Format>
      <DCPType>
        <HTTP>
          <Get><OnlineResource xmlns:xlink="http://www.w3.org/1999/xlink" xlink:href="[SERVICEONLINERESOURCE]"/></Get>
        </HTTP>
      </DCPType>
    </DescribeLayer>
    <GetLegendGraphic>
      <Format>image/png</Format>
      <Format>image/png32</Format>
      <DCPType>
        <HTTP>
          <Get><OnlineResource xmlns:xlink="http://www.w3.org/1999/xlink" xlink:href="[SERVICEONLINERESOURCE]"/></Get>
        </HTTP>
      </DCPType>
    </GetLegendGraphic>
    <GetStyles>
      <Format>text/xml</Format>
      <DCPType>
        <HTTP>
          <Get><OnlineResource xmlns:xlink="http://www.w3.org/1999/xlink" xlink:href="[SERVICEONLINERESOURCE]"/></Get>
        </HTTP>
      </DCPType>
    </GetStyles>
  </Request>
  <Exception>
    <Format>application/vnd.ogc.se_xml</Format>
    <Format>application/vnd.ogc.se_inimage</Format>
    <Format>application/vnd.ogc.se_blank</Format>
  </Exception>
  <VendorSpecificCapabilities />
  <UserDefinedSymbolization SupportSLD="1" UserLayer="0" UserStyle="1" RemoteWFS="0"/>
  <Layer>
    <Title>[GLOBALLAYERTITLE]</Title>)"""";

const char *WMS_1_3_0_GetCapabilities_Header = R""""(<?xml version="1.0" encoding="UTF-8"?>
<WMS_Capabilities
        version="1.3.0"
        updateSequence="0"
        xmlns="http://www.opengis.net/wms"
        xmlns:xlink="http://www.w3.org/1999/xlink"
        xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
        [SCHEMADEFINITION]
     >
    <Service>
        <Name>WMS</Name>
        <Title>[SERVICETITLE]</Title>
        <Abstract>[SERVICEABSTRACT]</Abstract>
        <KeywordList>
            <Keyword>view</Keyword>
            <Keyword>infoMapAccessService</Keyword>
            <Keyword>[SERVICEINFO] </Keyword>
        </KeywordList>
        <OnlineResource xlink:type="simple" xlink:href="[SERVICEONLINERESOURCE]"/>
        <ContactInformation>
          [CONTACTINFORMATION]
        </ContactInformation>
        <Fees>no conditions apply</Fees>
        <AccessConstraints>None</AccessConstraints>
        <MaxWidth>8192</MaxWidth>
        <MaxHeight>8192</MaxHeight>
    </Service>
    <Capability>
        <Request>
            <GetCapabilities>
                <Format>text/xml</Format>
                <DCPType><HTTP><Get><OnlineResource xlink:type="simple" xlink:href="[SERVICEONLINERESOURCE]"/></Get></HTTP></DCPType>
            </GetCapabilities>
            <GetMap>
                <Format>image/png</Format>
                <Format>image/png;mode=8bit</Format>
                <Format>image/png;mode=8bit_noalpha</Format>
                <Format>image/png;mode=24bit</Format>
                <Format>image/png;mode=32bit</Format>

                <Format>image/jpeg</Format>
                <!--<Format>image/webp</Format>-->
                <DCPType><HTTP><Get><OnlineResource xlink:type="simple" xlink:href="[SERVICEONLINERESOURCE]"/></Get></HTTP></DCPType>
            </GetMap>
            <GetFeatureInfo>
                <Format>image/png</Format>
                <Format>text/plain</Format>
                <Format>text/html</Format>
                <Format>text/xml</Format>
                <Format>application/json</Format>
                <DCPType><HTTP><Get><OnlineResource xlink:type="simple" xlink:href="[SERVICEONLINERESOURCE]"/></Get></HTTP></DCPType>
            </GetFeatureInfo>
        </Request>
        <Exception>
            <Format>XML</Format>
            <Format>INIMAGE</Format>
            <Format>BLANK</Format>
        </Exception>
        <UserDefinedSymbolization SupportSLD="1" />
)"""";
