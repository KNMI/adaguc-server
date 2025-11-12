FilterPoints - Definition of set of points to skip or to use
============================================================

Back to [Configuration](./Configuration.md)

The Filterpoints element can define the set of stations to use
(use="...").

The use attribute contain a comma separated list of station ids to use.
The use attribute starts by deselecting all station id's and the
selecting the id's to use.

The following example only plots KNMI's main observation stations:
```xml
<Point .../>
<FilterPoints use="06235,06280,06260,06290,06310,06380"/>
...
```
