StandardNames (standard\_name,variable\_name, units)
====================================================

Apply this style to all layers if requirements match.

-   standard\_name - Optional, comma separated list of standard\_names
    to match, or a \* (default) or a regular expression starting with
    \^.
-   variable\_name - Optional, the variable name to match, or a \*
    (default) or a regular expression starting with \^.
-   units - Optional, units to match or a regular expression starting
    with \^

Default or not configured is considered as a "\*". If all three
properties match the style will be added to all layers.

Styles can be assigned automatically to a Layer by configuring one or
more StandardNames element(s). When the Layers standard\_name matches
with one of the names configured in a Style, this Style will also be
assigned to that Layer. Optionally the units attribute can also be used
to discriminate in for example temperature layers with units in Kelvin
and units in Celsius

```
<Style name="air\_temp\_celsius">
<StandardNames standard\_name="air\_temperature,temperature"
units="Celsius"/>
...
</Style>

<Style name="air\_temp\_kelvin">
<StandardNames standard\_name="air\_temperature,temperature"
units="Kelvin"/>
...
</Style>

```
