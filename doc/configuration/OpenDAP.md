OpenDAP (enabled, path) - Settings for the ADAGUC OpenDAP server
================================================================

```
<OpenDAP enabled="true" path="opendap"/>
```

When enabled, all configured layers become opendap enabled. The layer is
served over opendap and can be used by clients which support opendap.
ADAGUC is both an opendap server and client, these settings are for
serving layers over opendap.

When a layer called "yourlayer" is configured, it can be accessed in the
following way:
https://yourdomain.com/cgi-bin/adagucserver.cgi/opendap/yourlayer

Example usages:

https://yourdomain.com/cgi-bin/adagucserver.cgi/opendap/yourlayer.das
https://yourdomain.com/cgi-bin/adagucserver.cgi/opendap/yourlayer.dds
ncdump -h
https://yourdomain.com/cgi-bin/adagucserver.cgi/opendap/yourlayer
ncview https://yourdomain.com/cgi-bin/adagucserver.cgi/opendap/yourlayer
