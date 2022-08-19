FilterPoints - Definition of set of points to skip or to use
============================================================

The Filterpoints element can define the set of stations to use
(use="...") or to not use (skip="...").
The use and skip attributes contain a comma separated list of station
ids to use or to skip.
The use attribute starts by deselecting all station id's and the
selecting the id's to use.
The skip attribute starts by selecting all station id's and unselecting
the specified id's.

The following example only plots KNMI's main observation stations:
```
<Point .../>
<FilterPoints skip="" use="06235,06280,06260,06290,06310,06380"/>
...
```
