Environment variables
=====================

The following environment variables allow you to change behaviour of the Adaguc server.

Some of these environment variables might be set via the `docker-compose.yml` file, see [Running adaguc-server with docker](/doc/Running.md)

| Environment variable | Description |
| -------------------- | ----------- |
| `ADAGUC_CONFIG` | pointer to the configuration file
| `ADAGUC_LOGFILE` | pointer where log messages should be stored, includes information logs and error logs
| `ADAGUC_ERRORFILE` | optional pointer which logs only error messages
| `ADAGUC_FONT` | Place where a TrueType font is stored, e.g. FreeSans.ttf
| `ADAGUC_DATARESTRICTION` | Optional pointer which controls access restrictions, by default set to FALSE, can be combinations of `ALLOW_WCS\|ALLOW_GFI\|ALLOW_METADATA\|SHOW_QUERYINFO`, separated with the \| token.<br /><br />`FALSE`: No restrictions (default, same as `ALLOW_WCS\|ALLOW_GFI\|ALLOW_METADATA`)<br>`ALLOW_WCS`: Allows the Web Coverage Service, download of data<br />`ALLOW_GFI`: Allows GetFeatureInfo requests, e.g. getting information about a certain location<br>`ALLOW_METADATA`: Allows getting NetCDF header metadata information<br>`SHOW_QUERYINFO`: When a query has failed, the corresponding query will be presented to the user. This feature is disabled by default.
| `ADAGUC_PATH` | optional, is used as variable substitution {ADAGUC_PATH} in the configuration files, should point to the adagucserver installation
| `ADAGUC_TMP` | optional, is used as variable substitution {ADAGUC_TMP} in the configuration files, location where tempfiles need to be written
| `ADAGUC_ONLINERESOURCE` | optional, specify the online resource in the CGI script itself, see [OnlineResource](/doc/configuration/OnlineResource.md) to configure in the xml file.
| `PGBOUNCER_ENABLE` | Enable or disable the usage of PostgreSQL connection pooling by PGBouncer. Default: `true`.
| `PGBOUNCER_DISABLE_SSL` | If PGBouncer is used, disable the usage of SSL. Default: `true`.
| `ADAGUC_DB` | The connection string used by PostgreSQL. Default: `host=adaguc-db port=5432 user=adaguc password=adaguc dbname=adaguc`.
| `ADAGUC_ENABLELOGBUFFER` | Enable or disable log buffering. If disabled, no debug logging is shown at all. Default: `true`.
| `ADAGUC_AUTOWMS_DIR` | Default: `/data/adaguc-autowms`
| `ADAGUC_DATA_DIR` | Default: `/data/adaguc-data`
| `ADAGUC_DATASET_DIR` | Default: `/data/adaguc-datasets`
| `ADAGUC_FORK_SOCKET_PATH` | Enables or disables the usage of an Adaguc fork server. If set to an adaguc-writable path, e.g. `adaguc.socket`, Adaguc will launch a server in the background and fork itself when handling requests. Communication will take place through this socket. If left empty or removed, Adaguc will launch a subprocess which comes with overhead.
| `ADAGUC_MAX_PROC_TIMEOUT` | Every request made to Adaguc will timeout after this many seconds. Default: `300` seconds.
| `ADAGUC_PORT` | Port to listen to for the webserver, used by `docker-compose.yml`. Default: port `443`.
| `EXTERNALADDRESS` | Adaguc-viewer and adaguc-explorer will be reachable on this hostname.
| `ADAGUCENV_RETENTIONPERIOD` | ISO 8601 time period string indicating how long files should be retained.
| `ADAGUCENV_ENABLECLEANUP` | Enable or disable cleaning.

### TODO:
- How/where you can apply these environment variables (.env, docker-compose.yaml, adaguc xml config)?
- What are the defaults?
- Which vars are optional or required?
- What is the format (string, caps only, path, etc)?