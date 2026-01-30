# Configure a Solar Terminator layer using AdagucServer

- [Back to readme](./Readme.md)

## Prerequisites

Make sure adaguc-server is running, follow the instructions at [Starting the adaguc-server with docker](../Running.md)

## Step 1: Configure a dataset for this datafile

Create the following file at the filepath `$ADAGUC_DATASET_DIR/solt.xml`

```xml
<?xml version="1.0" encoding="UTF-8" ?>
<Configuration>
<?xml version="1.0" encoding="UTF-8" ?>
  <Style name="solt_twilight">
    <Legend fixedclasses="true" tickinterval="0.1" tickround=".01">no2</Legend>
    <Min>0</Min>
    <Max>180</Max>
  
    <ShadeInterval min="0" max="90.0"    label="Day"    fillcolor="#FFFFFF"/>
    <ShadeInterval min="90.00" max="96.00"    label="Civil Twilight"    fillcolor="#999999"/>
    <ShadeInterval min="96.00" max="102.00"    label="Nautical Twilight"    fillcolor="#666666"/>
    <ShadeInterval min="102.00" max="108.00"    label="Astronomical Twilight"    fillcolor="#333333"/>
    <ShadeInterval min="108.00" max="180.00"    label="Night"    fillcolor="#000000"/>

    <NameMapping name="shadedcontour"   title="Shaded categories" abstract="Displays different phases of twilight and day using shades of gray, with black for night and white for day."/>
    <RenderMethod>shadedcontour</RenderMethod>

  </Style>

  <Layer type="liveupdate">
    <Name>solarterminator</Name>
    <DataPostProc algorithm="solarterminator"/>
    <Variable>solarterminator</Variable>
    <Styles>solt_twilight</Styles>
    <Dimension interval="PT10M">time</Dimension>
  </Layer>
</Configuration>

```

Feel free to customize the style to your preference, such as adding a fill color with transparency. However, keep in mind that the solar terminator calculation returns fixed categories rather than precise solar zenith angle values.

The `offset` attribute is optional, with the default being one year. The time range that will be made available is `(now-offset, now+offset)`.

## Step 3: Scan the new data

```
docker exec -i -t my-adaguc-server /adaguc/adaguc-server-updatedatasets.sh solt
```

## Step 4: Check if the layer works 
You can use the following URL to try out your local Solar Terminator layer. 

Visit:
- https://adaguc.knmi.nl/adaguc-viewer/index.html?autowms=https://yourhostname/autowms#addlayer('https://yourhostname/adagucserver?dataset=solt&','solarterminator')
