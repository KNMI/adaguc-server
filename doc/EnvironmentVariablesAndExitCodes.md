# Exit Codes

The following exit codes are supported:

## When in server mode:

| Id | Code | Description |
| -------- | ------- | ------- |
| HTTP_STATUSCODE_200_OK | 0 | All OK |
| HTTP_STATUSCODE_404_NOT_FOUND | 32 | Triggers when something is not found, also when the timerange is not found |
| HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY | 33 | The client requested something unprocessable and the server encountered an error handling the request |

## When in command mode when scanning files or datasets:

| Id | Code | Description |
| -------- | ------- | ------- |
|SCAN_EXITCODE_FILENOMATCH|64| File is available but does not match any of the available datasets |
|SCAN_EXITCODE_DATASETNOEXIST|65| The dataset configuration file does not exist. |
|SCAN_EXITCODE_SCANERROR|66|  An error occured during scanning |
|SCAN_EXITCODE_FILENOEXIST|67| The file does not exist on the file system |
|SCAN_EXITCODE_FILENOMATCH_ISDELETED|68| File does not match any of the available datasets and is deleted  |
|SCAN_EXITCODE_TIMEOUT|124| The scan process timed out |

# HTTP status codes

The Python server runs adaguc-server for each WMS, WCS and OpenDAP request, and turns its exit code into the HTTP status code of the response. It can also reject a request itself, before adaguc-server runs. Exit codes 34 and 35 are set by the Python server, not by adaguc-server.

For WMS and WCS requests:

| HTTP status | Cause | What to do |
| -------- | ------- | ------- |
| 200 OK | Exit code 0. `/healthcheck` always returns 200 and is not checked against `ADAGUC_TRUSTED_HOSTS`. | - |
| 400 Bad Request | Exit code 35. Only in [fork mode](/doc/fork_server.md) (`ADAGUC_FORK_ENABLE=TRUE`): the request is too large for the fork server, or a request value contains a newline. | Send a smaller request, for example with fewer coordinates. |
| 400 Invalid host header | The host of the request is not in `ADAGUC_TRUSTED_HOSTS`. When the request comes from a proxy in `ADAGUC_TRUSTED_PROXIES`, this is the host from its `X-Forwarded-Host` header. Only checked when `EXTERNALADDRESS` is unset. | Add the host to `ADAGUC_TRUSTED_HOSTS`. See [Running adaguc-server behind a reverse proxy](tutorials/Running_adaguc_server_behind_a_reverse_proxy.md). |
| 404 Not Found | Exit code 32 (`HTTP_STATUSCODE_404_NOT_FOUND`): something is not found, for example a layer or the requested time range. | Check the request parameters against GetCapabilities. |
| 413 Content Too Large | The parameters after url encoding are larger than 64 KB. Encoding makes characters like `<`, `>`, `"` and spaces three bytes. | Send a large `STYLESHEET` compressed, see [Compress the style](tutorials/Post_a_custom_style_with_STYLESHEET.md#10-compress-the-style). |
| 422 Unprocessable Entity | Exit code 33 (`HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY`): adaguc-server could not process the request. For a `STYLESHEET` or `SLD_BODY`: it is not enabled (`ADAGUC_STYLESHEET_ENABLE`), the XML is invalid, not allowed or not supported, `SLD_VERSION=1.1.0` is missing, or more than one layer is requested; the body says why. | Check the request parameters. The log has the details. For a `STYLESHEET`, see [Send a custom style with STYLESHEET](tutorials/Post_a_custom_style_with_STYLESHEET.md). |
| 500 Internal Server Error | Exit code 34: the request took longer than `ADAGUC_MAX_PROC_TIMEOUT` seconds and was stopped. | Request less data, or raise `ADAGUC_MAX_PROC_TIMEOUT`. |
| 500 Internal Server Error | Any other non-zero exit code: adaguc-server ran into an error. | See the log. |

OpenDAP requests return 500 for every non-zero exit code. The OGC API and EDR endpoints check their own query parameters, and return 400 or 404 with a description of the problem.

# Environment variables

| Environment variable      | Description | Default value |
|---------------------------|-------------|---------------|
| `ADAGUC_AUTOWMS_DIR`      | Directory used for automatic WMS generation. NetCDF, PNG, CSV, or HDF5 files placed here are automatically exposed through a generated WMS service. | `/data/adaguc-autowms` |
| `ADAGUC_CONFIG`           | Path to the main server configuration XML file. | `${ADAGUC_PATH}/python/lib/adaguc/adaguc-server-config-python-postgres.xml` |
| `ADAGUC_DATA_DIR`         | Directory containing data files, such as NetCDF files used for realtime updates. | `/data/adaguc-data` |
| `ADAGUC_DATARESTRICTION`  | Controls access restrictions for certain services. Possible values: `ALLOW_WCS`, `ALLOW_GFI`, `ALLOW_METADATA`, `SHOW_QUERYINFO`.<br>Values can be combined with `\|`. `FALSE` means no restrictions. | `FALSE` (`ALLOW_WCS\|ALLOW_GFI\|ALLOW_METADATA`) |
| `ADAGUC_DATASET_DIR`      | Directory containing dataset configuration XML files. | `/data/adaguc-datasets` |
| `ADAGUC_DB`               | PostgreSQL connection string used by adaguc. | `host=adaguc-db port=5432 user=adaguc password=adaguc dbname=adaguc` |
| `ADAGUC_ENABLELOGBUFFER`  | Controls logging behaviour:<br>`FALSE`: unbuffered logging with live output (may slow service).<br>`TRUE`: buffered logging.<br>`DISABLELOGGING`: disables logging completely. | `FALSE` |
| `ADAGUC_FONT`             | Path to a default TrueType font (TTF) used for rendering text in generated imagery (e.g. `FreeSans.ttf`). | -|
| `ADAGUC_FORK_ENABLE`      | Enables running adaguc-server in [fork mode](/doc/fork_server.md) when set to `TRUE`. | `FALSE` |
| `ADAGUC_LOGFILE`          | File where log messages are written. | -|
| `ADAGUC_MAX_COMMAND_TIMEOUT` | Maximum processing time in seconds for command-style invocations such as `--updatedb` and `--updatelayermetadata`. | `300` |
| `ADAGUC_MAX_PROC_TIMEOUT` | Maximum allowed processing time (in seconds) for a single adaguc-server request. | `60` |
| `ADAGUC_NUMPARALLELPROCESSES` | Number of parallel worker processes used by the adaguc-server. | `4` |
| `ADAGUC_ONLINERESOURCE`   | Optional override for the [OnlineResource](configuration/OnlineResource.md) URL used by the CGI service. Can also be configured in the XML configuration file. | -|
| `ADAGUC_PATH`             | Root directory of the adaguc-server code base. Available as`{ADAGUC_PATH}` substitution in configuration files. | Current working directory (`pwd`) |
| `ADAGUC_STYLESHEET_ENABLE` | Allows clients to send their own style with the `STYLESHEET` parameter, or in an SLD with `SLD_BODY`, when set to `TRUE`. GetCapabilities then advertises SLD support. The XML is checked against an allowlist; still put a rate or concurrency limit in front of a public server, because a valid style can be expensive to render. | `FALSE` |
| `ADAGUC_TMP`              | Directory used for temporary files. Available as `{ADAGUC_TMP}` substitution in configuration files. | -|
| `ADAGUC_TRACE_TIMINGS`    | Enables timing measurements for internal operations such as database queries, file access, and image generation.<br>Results are returned in the `x-trace-timings` HTTP header. | `FALSE` |
| `ADAGUCENV_ENABLECLEANUP` | Enables or disables automatic cleanup of files based on the retention period.<br>See also [environment.md](/doc/configuration/Environment.md). | -|
| `ADAGUCENV_RETENTIONPERIOD` | ISO 8601 duration specifying how long generated or cached files should be retained.<br>See also [environment.md](/doc/configuration/Environment.md). | -|
| `EXTERNALADDRESS`         | Hostname where Adaguc Viewer and Adaguc Explorer are reachable. | -|
| `ADAGUC_SERVER_PORT`      | The http port on which adaguc-server will be available. | 8080 |
| `ADAGUC_TRUSTED_HOSTS`    | When EXTERNALADDRESS is unset, this is the allowed hosts list that can be advertised as online resource in the WMS GetCapabilities. Requests for any other host get `400 Invalid host header`, see [HTTP status codes](#http-status-codes). | - (all hosts allowed) |
| `ADAGUC_TRUSTED_PROXIES`  | When EXTERNALADDRESS is unset, this is the list of trusted proxies from which adaguc-server will determine its external address (via the `X-Forwarded-` headers). The headers of other clients are ignored. | `127.0.0.1` (the Docker Compose setup sets `*`) |
| `PGBOUNCER_DISABLE_SSL`   | Disables SSL when connecting through PgBouncer. | `true` |
| `PGBOUNCER_ENABLE`        | Enables or disables PostgreSQL connection pooling through PgBouncer. | `true` |
