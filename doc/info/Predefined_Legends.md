Predefined Legends
==================

Legends from soliton:
---------------------

http://soliton.vm.bytemark.co.uk/pub/cpt-city/views/totp-cpt.html

Isoluminant legend:
-------------------

```
<Legend name="isoluminant" type="colorRange">
<palette index="0" red="216" green="15" blue="15"/>
<palette index="48" red="134" green="134" blue="0"/>
<palette index="96" red="0" green="151" blue="0"/>
<palette index="144" red="0" green="143" blue="143"/>
<palette index="192" red="81" green="253" blue="247"/>
<palette index="240" red="183" green="0" blue="183"/>
</Legend>
```
The min and max parameters refer directly to the position in the
palette. The colors are not interpolated, which results in discrete
intervals. For an interval legend, both index and min/max parameters can
be used.

Climate and weather physical quantities
---------------------------------------

```
<Legend name="temperature" type="interval">
<palette index="0" color="#2E2E73"/> <!-- ~~14 -~~>
<palette index="9" color="#282898"/> <!-- ~~12 -~~>
<palette index="18" color="#201FBB"/> <!-- ~~10 -~~>
<palette index="27" color="#1A1ADC"/> <!-- ~~8 -~~>
<palette index="36" color="#3654DE"/> <!-- ~~6 -~~>
<palette index="45" color="#548EDC"/> <!-- ~~4 -~~>
<palette index="54" color="#72CADE"/> <!-- ~~2 -~~>
<palette index="63" color="#6DD8DF"/> <!-- 0-->
<palette index="72" color="#55CDE2"/> <!-- 2-->
<palette index="81" color="#38BBDC"/> <!-- 4 -->
<palette index="90" color="#20B0DC"/> <!-- 6 -->
<palette index="99" color="#19BAA6"/> <!-- 8 -->
<palette index="108" color="#1CCE6A"/> <!-- 10 -->
<palette index="117" color="#1BDF22"/> <!-- 12 -->
<palette index="126" color="#82C319"/> <!-- 14 -->
<palette index="135" color="#DCA819"/> <!-- 16 -->
<palette index="144" color="#DD921A"/> <!-- 18 -->
<palette index="153" color="#DE7C1A"/> <!-- 20 -->
<palette index="162" color="#DF671A"/> <!-- 22 -->
<palette index="171" color="#DE501A"/> <!-- 24 -->
<palette index="180" color="#DD3819"/> <!-- 26 -->
<palette index="189" color="#DD2319"/> <!-- 28 -->
<palette index="198" color="#D21A1E"/> <!-- 30 -->
<palette index="207" color="#C31927"/> <!-- 32 -->
<palette index="216" color="#AD1A30"/> <!-- 34 -->
<palette index="225" color="#9A1A3B"/> <!-- 36 -->
<palette index="234" color="#871A44"/> <!-- 38 -->
<palette index="240" color="#871A44"/> <!-- 39,33 -->
</Legend>

<Legend name="precipitation" type="interval">
<palette index="0" color="#BACDCB"/> <!-- 0 -->
<palette index="30" color="#A3C3C9"/> <!-- 0.1 -->
<palette index="60" color="#8BBCC8"/> <!-- 0.2 -->
<palette index="90" color="#6AAEC1"/> <!-- 0.5 -->
<palette index="120" color="#42A1C0"/> <!-- 1 -->
<palette index="150" color="#377EAF"/> <!-- 2 -->
<palette index="180" color="#46669C"/> <!-- 5 -->
<palette index="210" color="#56528D"/> <!-- 10-->
<palette index="239" color="#86008D"/> <!-- 30-->
</Legend>

<Legend name="pressure" type="interval">
<palette index="0" color="#5C5797"/> <!-- 960 -->
<palette index="12" color="#1D5791"/> <!-- 964 -->
<palette index="24" color="#1E8BC5"/> <!-- 968 -->
<palette index="36" color="#1EA6B9"/> <!-- 972 -->
<palette index="48" color="#1A99A0"/> <!-- 976 -->
<palette index="60" color="#1C7976"/> <!-- 980 -->
<palette index="72" color="#1C7244"/> <!-- 984 -->
<palette index="84" color="#72B060"/> <!-- 988 -->
<palette index="96" color="#AFC560"/> <!-- 992 -->
<palette index="108" color="#C9CF3C"/> <!-- 996 -->
<palette index="120" color="#E3DA36"/> <!-- 1000 -->
<palette index="132" color="#EFE686"/> <!-- 1004 -->
<palette index="144" color="#EFE3BB"/> <!-- 1008 -->
<palette index="156" color="#ECCB90"/> <!-- 1012 -->
<palette index="168" color="#E7A34A"/> <!-- 1016 -->
<palette index="180" color="#DE7222"/> <!-- 1020 -->
<palette index="192" color="#D93531"/> <!-- 1024 -->
<palette index="204" color="#C41F32"/> <!-- 1028 -->
<palette index="216" color="#843537"/> <!-- 1032 -->
<palette index="228" color="#5E3F3A"/> <!-- 1036 -->
<palette index="239" color="#2E3F2A"/> <!-- 1040 -->
</Legend>

<Style name="temperature">
<Legend fixed="true" tickinterval="2">temperature</Legend>
<Min>-14</Min>
<Max>39,33333333</Max> <!-- 39,33333333 = (240 / (234/(38
- ~~14)))~~ 14 -->
<NameMapping name="point" title="Temperature"
abstract="Temperature"/>
<Point plotstationid="false" pointstyle="point" discradius="15"
textradius="0" dot="false" fontsize="8" textcolor="#000000" />
</Style>

<Style name="precipitation">
<Legend fixed="true"
tickinterval="2">precipitation</Legend>
<Min>0</Min>
<Max>30</Max>
<NameMapping name="point" title="precipitation"
abstract="precipitation"/>
<Point plotstationid="false" pointstyle="point" discradius="15"
textradius="0" dot="false" fontsize="8" textcolor="#000000" />
</Style>

<Style name="pressure">
<Legend fixed="true" tickinterval="4">pressure</Legend>
<Min>960</Min>
<Max>1040</Max>
<NameMapping name="point" title="pressure" abstract="pressure"/>
<Point plotstationid="false" pointstyle="point" discradius="15"
textradius="0" dot="false" fontsize="8" textcolor="#000000"
textformat="%0.0f"/>
</Style>
```

Seismological data
------------------

```xml
<Legend name="magnitude_int" type="interval">
  <palette index="0" color="#55005550"/> <!-- 0 Nothing -->
  <palette index="20" color="#66009980"/> <!-- 1 Insignificant -->
  <palette index="40" color="#0099FF80"/> <!-- 2 Low -->
  <palette index="60" color="#00CC99B0"/> <!-- 3 Minor -->
  <palette index="80" color="#99CC66B0"/> <!-- 4 Moderate -->
  <palette index="100" color="#99FF33B0"/> <!-- 5 Intermediate -->
  <palette index="120" color="#FFFF33B0"/> <!-- 6 Noteworthy -->
  <palette index="140" color="#FFCC66C0"/> <!-- 7 High -->
  <palette index="160" color="#FF9966D0"/> <!-- 8 Far-reaching -->
  <palette index="180" color="#FF3300E0"/> <!-- 9 Outstanding -->
  <palette index="200" color="#CC0000FF"/> <!-- 10 Extraordinary -->
  <palette index="220" color="#880000FF"/> <!-- 11 ! -->
  <palette index="239" color="#000000FF"/> <!-- 12 !! -->
</Legend>

<Legend name="magnitude_col" type="colorRange">
  <palette index="0" color="#55005550"/> <!-- 0 Nothing -->
  <palette index="20" color="#66009980"/> <!-- 1 Insignificant -->
  <palette index="40" color="#0099FF80"/> <!-- 2 Low -->
  <palette index="60" color="#00CC99B0"/> <!-- 3 Minor -->
  <palette index="80" color="#99CC66B0"/> <!-- 4 Moderate -->
  <palette index="100" color="#99FF33B0"/> <!-- 5 Intermediate -->
  <palette index="120" color="#FFFF33B0"/> <!-- 6 Noteworthy -->
  <palette index="140" color="#FFCC66C0"/> <!-- 7 High -->
  <palette index="160" color="#FF9966D0"/> <!-- 8 Far-reaching -->
  <palette index="180" color="#FF3300E0"/> <!-- 9 Outstanding -->
  <palette index="200" color="#CC0000FF"/> <!-- 10 Extraordinary -->
  <palette index="220" color="#880000FF"/> <!-- 11 ! -->
  <palette index="239" color="#000000FF"/> <!-- 12 !! -->
</Legend>

<Legend name="magnitude_noalpha" type="colorRange">
  <palette index="0" color="#550055"/> <!-- 0 Nothing -->
  <palette index="20" color="#660099"/> <!-- 1 Insignificant -->
  <palette index="40" color="#0099FF"/> <!-- 2 Low -->
  <palette index="60" color="#00CC99"/> <!-- 3 Minor -->
  <palette index="80" color="#99CC66"/> <!-- 4 Moderate -->
  <palette index="100" color="#99FF33"/> <!-- 5 Intermediate -->
  <palette index="120" color="#FFFF33"/> <!-- 6 Noteworthy -->
  <palette index="140" color="#FFCC66"/> <!-- 7 High -->
  <palette index="160" color="#FF9966"/> <!-- 8 Far-reaching -->
  <palette index="180" color="#FF3300"/> <!-- 9 Outstanding -->
  <palette index="200" color="#CC0000"/> <!-- 10 Extraordinary -->
  <palette index="220" color="#880000"/> <!-- 11 ! -->
  <palette index="239" color="#000000"/> <!-- 12 !! -->
</Legend>

<Style name="magnitude_int">
  <Legend fixed="true" tickinterval="1">magnitude_int</Legend>
  <Min>0</Min>
  <Max>12</Max>
  <NameMapping name="point" title="Richter magnitude scale" abstract="With discrete colors"/>
  <Point plotstationid="false" pointstyle="point" discradius="20" textradius="0" dot="false" fontsize="14" textcolor="#FFFFFF"/>
</Style>

<Style name="magnitude_col">
  <Legend fixed="true" tickinterval="1">magnitude_col</Legend>
  <Min>0</Min>
  <Max>12</Max>
  <NameMapping name="point" title="Richter magnitude scale" abstract="Wth continuous colors"/>
  <Point plotstationid="false" pointstyle="point" discradius="20" textradius="0" dot="false" fontsize="14" textcolor="#FFFFFF"/>
</Style>

<Style name="magnitude2">
  <Legend fixed="true" tickinterval="1">magnitude_int</Legend>
  <Min>0</Min>
  <Max>12</Max>
  <NameMapping name="point" title="Richter magnitude scale >=2.0" abstract="magnitude"/>
  <Point plotstationid="false" pointstyle="point" discradius="20" textradius="0" dot="false" fontsize="14" textcolor="#FFFFFF"/>
  <ValueRange min="2" max="1000"/>
</Style>

<Style name="magnitude3">
  <Legend fixed="true" tickinterval="1">magnitude_int</Legend>
  <Min>0</Min>
  <Max>12</Max>
  <NameMapping name="point" title="Richter magnitude scale >=3.0" abstract="magnitude"/>
  <Point plotstationid="false" pointstyle="point" discradius="20" textradius="0" dot="false" fontsize="14" textcolor="#FFFFFF"/>
  <ValueRange min="3" max="1000"/>
</Style>
```
