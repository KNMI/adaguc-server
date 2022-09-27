OnlineResource (value)
======================

Back to [Configuration](./Configuration.md)

Sets the external location of the server. There are three options to
configure this:

1). Specify the full path in the configuration:

```xml
<OnlineResource value="http://yourserver/adagucserver?"/>
```

2). Specify the suffix, the server will prepend the HTTP_HOST
environment variable for you:

```xml
<OnlineResource value="/adagucserver?"/>
```

3). Specify it in the environment via ADAGUC_ONLINERESOURCE
You can leave out the OnlineResource and specify it as environment
variable ADAGUC_ONLINERESOURCE in the cgi shell script.

```
export ADAGUC_ONLINERESOURCE="http://yourserver/adagucserver?"
```
