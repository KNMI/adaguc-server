ContourLine (width,linecolor,textcolor,textformatting,interval,classes)
=======================================================================

-   width - The width of the line
-   linecolor - The color of the line in hexadecimal format \#RRGGBB
-   textcolor - The color of the text in hexadecimal format \#RRGGBB
-   textformatting - How the text is displayed, using standard string
    formatting. Use "" for no text in contourlines
-   interval - Draw lines at every <n> value
-   classes - Comma separated list of values to draw lines on.

Draw Contourline at every degree:
```
<ContourLine width="0.3" linecolor="\#444444" textcolor="\#444444"
textformatting="%2.0f" interval="1"/>
```

Draw Contourline at defined values:
```
<ContourLine width="0.3" linecolor="\#444444" textcolor="\#444444"
textformatting="%2.0f" classes="10,25,50,100,150"/>
```
