#ifndef SLD_HANDLER_H
#define SLD_HANDLER_H

#include <string>
#include <string_view>

// Namespaces of SLD 1.1.0 (OGC 05-078r4), Symbology Encoding 1.1.0 (OGC 05-077r4) and the ADAGUC symbolizer
inline constexpr const char *SLD_NAMESPACE = "http://www.opengis.net/sld";
inline constexpr const char *SE_NAMESPACE = "http://www.opengis.net/se";
inline constexpr const char *ADAGUC_SLD_NAMESPACE = "https://github.com/KNMI/adaguc-server";

enum class SldResult {
  ok,          // SldStyle has the style for the layer
  notForLayer, // The SLD has no style for the requested layer and style: the server's own style is used
  invalid      // Logged with CDBError
};

struct SldStyle {
  std::string layerName;  // Name of the NamedLayer
  std::string styleSheet; // The ADAGUC Legend and Style elements of the AdagucSymbolizer, as for STYLESHEET
};

/**
 * Reads an SLD_BODY: a StyledLayerDescriptor 1.1.0 (OGC 05-078r4) whose style is ADAGUC style XML.
 *
 * ADAGUC styles are not expressed in Symbology Encoding, so a UserStyle carries them in a vendor symbolizer. SE 1.1
 * defines se:Symbolizer as abstract, and a se:Rule takes any element of its substitution group:
 *
 *   <StyledLayerDescriptor version="1.1.0" xmlns="http://www.opengis.net/sld" xmlns:se="http://www.opengis.net/se">
 *     <NamedLayer>
 *       <se:Name>layer</se:Name>
 *       <UserStyle>
 *         <se:Name>my_style</se:Name>
 *         <IsDefault>1</IsDefault>
 *         <se:CoverageStyle><se:Rule>
 *           <AdagucSymbolizer xmlns="https://github.com/KNMI/adaguc-server">
 *             <Legend name="...">...</Legend>
 *             <Style>...</Style>
 *           </AdagucSymbolizer>
 *         </se:Rule></se:CoverageStyle>
 *       </UserStyle>
 *     </NamedLayer>
 *   </StyledLayerDescriptor>
 *
 * The content of the AdagucSymbolizer is the same as a STYLESHEET, its namespace is ignored.
 * A NamedStyle refers to a configured style, it becomes an IncludeStyle.
 *
 * Library mode (05-078r4 9.2.2), when layerName is given: the NamedLayer with that name is used, and its style named
 * styleName or, when styleName is empty, its style with IsDefault. Without such a layer or style the result is
 * SldResult::notForLayer, and the server's own style is used.
 * Literal mode, when layerName is empty: the SLD must have one NamedLayer with at most one style.
 *
 * @param sld The SLD XML
 * @param layerName The requested layer (LAYERS), or empty
 * @param styleName The requested style (STYLES), or empty
 * @param sldStyle Receives the layer name and the ADAGUC style XML when the result is SldResult::ok. The style XML is
 *                 empty when the NamedLayer has no style (literal mode): the layer's default style is used.
 */
SldResult readSldBody(const std::string &sld, std::string_view layerName, std::string_view styleName, SldStyle &sldStyle);

#endif
