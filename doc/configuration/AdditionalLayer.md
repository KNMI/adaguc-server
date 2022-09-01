AdditionalLayer (replace, style) <value>
==============================================

Back to [Configuration](./Configuration.md)

Multiple additional layers can be stacked on top of each other. The
replace attribute decides whether the previous layer or all previous
layers need to be replaced.

```xml
<AdditionalLayer replace="true" >layer1</AdditionalLayer>
```

-   value is the layer Name of a configured layer

\* replace can be:

1.  false: just overlay next additional layers, this is the default if
    replace is not configured
2.  previous: replace the previous layer within this layer, this can
    also be the parent layer
3.  all: replaces all previous layers within this layer
4.  true: same as previous

-   style is the name to use for this layer as advertised in the
    getcapabilities document, e.g. style="radar/nearest". When the style
    is not found, a list with possible entries is given in the logging.

Additionallayer and replace will only function when data was found for
the additional layer. This implies that different layers spanning
overlapping time domains can be combined.

e.g.
```
                      time axis ->
parentlayer      (1): Y   Y   Y   Y   Y   Y   Y   Y   Y   Y   Y   Y   Y   Y
additionallayer1 (2): N   N   Y   Y   Y   Y   Y   Y   Y   Y   N   N   N   N 
additionallayer2 (3): N   N   N   N   N   N   Y   Y   Y   Y   Y   Y   Y   Y
         combines to:
2+3 set to false    : 1   1   12  12  12  12  123 123 123 123 13  13  13  13
2+3 set to  previous: 1   1   2   2   2   2   13  13  13  13  3   3   3   3
2=previous, 3=all   : 1   1   2   2   2   2   3   3   3   3   3   3   3   3
2=all, 3=false      : 1   1   2   2   2   2   23  23  23  23  13  13  13  13


```

Example config of table above:
```xml
<!-- parentlayer -->
<Layer type="database">
  <Name>composite</Name>
  <AdditionalLayer replace="true" >layer1</AdditionalLayer>
  <AdditionalLayer replace="false">layer2</AdditionalLayer>
  ... etc (Dimension, FilePath, Variable, etc) ...
</Layer>

<!-- layer1 -->
<Layer type="database">
  <Name>layer1</Name>
  ... etc (Dimension, FilePath, Variable, etc) ...
</Layer>

<!-- layer2 -->
<Layer type="database">
  <Name>layer2</Name>
  ... etc (Dimension, FilePath, Variable, etc) ...
</Layer>
```

Still todo: Aggregate dimensions from all additional layers and
advertise them in the parent layer. Currently, the parent layer needs to
cover all possible dates itself, this is also shown in table above, the
parentlayer has 'Y' everywhere.
