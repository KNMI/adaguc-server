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
  <!-- Shaded categories -->
  <Style name="solt_twilight">
    <Legend fixedclasses="true" tickinterval="0.1" tickround=".01">no2</Legend>
    <Min>0</Min>
    <Max>180</Max>
  
    <ShadeInterval min="0" max="90.0" label="Day (0–90°)" fillcolor="#FFFFFF"/>
    <ShadeInterval min="90.00" max="96.00" label="Civil Twilight (90–96°)" fillcolor="#999999"/>
    <ShadeInterval min="96.00" max="102.00" label="Nautical Twilight (96–102°)" fillcolor="#666666"/>
    <ShadeInterval min="102.00" max="108.00" label="Astronomical Twilight (102–108°)" fillcolor="#333333"/>
    <ShadeInterval min="108.00" max="180.00" label="Night (108–180°)" fillcolor="#000000"/>

    <RenderMethod>shadedcontour</RenderMethod>

  </Style>

  <Layer type="liveupdate">
    <Title>Solar Zenith Angle (SZA)</Title>
    <Abstract>Displays the solar zenith angle (SZA). The SZA ranges from 0 to 90 degrees during the day, and from 90 and up to 180 degrees at night.</Abstract>

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
