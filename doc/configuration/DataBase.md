DataBase (parameters, maxquerylimit)
===========================================

Back to [Configuration](./Configuration.md)

Configuration to access a PostgreSQL or SQLite database, for example:

```xml
<DataBase maxquerylimit="365" parameters="dbname=thedatabase host=thehostname user=theuser password=asecretpassword"/>
```

```xml
<DataBase parameters="databasefile.db"/>
```

-   `parameters` are the database parameters required for making the
    connection. If the `parameters` string ends with `.db`, we assume adaguc should use `sqlite`.
-   `maxquerylimit` is the maximum number of results for a getfeatureinfo
    call.

