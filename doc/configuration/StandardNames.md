StandardNames (standard_name,variable_name, units)
====================================================

Back to [Configuration](./Configuration.md)

Apply this style to all layers if requirements match.

-   standard_name - Optional, comma separated list of standard_names
    to match, or a \* (default) or a regular expression starting with
    \^.
-   variable_name - Optional, the variable name to match, or a \*
    (default) or a regular expression starting with \^.
-   units - Optional, units to match or a regular expression starting
    with \^

Default or not configured is considered as a "\*". If all three
properties match the style will be added to all layers.

Styles can be assigned automatically to a Layer by configuring one or
more StandardNames element(s). When the Layers standard_name matches
with one of the names configured in a Style, this Style will also be
assigned to that Layer. Optionally the units attribute can also be used
to discriminate in for example temperature layers with units in Kelvin
and units in Celsius

```xml
<Style name="air_temp_celsius">
  <StandardNames standard_name="air_temperature,temperature" units="Celsius"/>
 ...
</Style>

<Style name="air_temp_kelvin">
  <StandardNames standard_name="air_temperature,temperature" units="Kelvin"/>
 ...
</Style>

```
