palette (index,min,max,red,green,blue,alpha,color)
==================================================

palette is used in either a colorRange or interval based [Legend](Legend.md)

-   index - Used to position colors when defining a continuous
    (colorRange) and discrete (interval) legend
-   min - Used to position the beginning of a color when defining a
    discrete (interval) legend
-   max - Used to position the end of a color when defining a discrete
    (interval) legend
-   red - The intensity of the color red, range 0-255
-   green - The intensity of the color green, range 0-255
-   blue - The intensity of the color blue, range 0-255
-   alpha - The alpha channel, works only when requesting image/png32
    images. 0 is transparent, 255 is opaque (visible).
-   color - Shorthand for red,green,blue or red,green,blue alpha. E.g.
    \#A41D23 or \#A41D23FF

