Include
=======

Inludes additional configuration files

-   location - The XML file to include, must be an absolute path.

For example:

<Include location="/data/services/config/datasets/mystyles.xml"/>

Where mystyles.xml can be like:

```

<?xml version="1.0" encoding="UTF-8" ?>
<Configuration>
<Legend name="temperature" type="colorRange">
<palette index="0" red="0" green="60" blue="123"/>
<palette index="30" red="0" green="100" blue="140"/>
<palette index="60" red="8" green="130" blue="206"/>
<palette index="85" red="132" green="211" blue="255"/>
<palette index="120" red="247" green="247" blue="247"/>
<palette index="155" red="255" green="195" blue="57"/>
<palette index="180" red="232" green="28" blue="0"/>
<palette index="210" red="165" green="0" blue="0"/>
<palette index="240" red="90" green="0" blue="0"/>
</Legend>

<Style name="tg">
<Legend>temperature</Legend>
<Min>-20</Min>
<Max>30</Max>
<RenderMethod>nearest,bilinear</RenderMethod>
</Style>
</Configuration>
```
