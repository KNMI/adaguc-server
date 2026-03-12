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


# Environment variables

| Environment variable      | Description | Default value |
|---------------------------|-------------|---------------|
| `ADAGUC_CONFIG`           | Path to the main server configuration XML file. | `${ADAGUC_PATH}/python/lib/adaguc/adaguc-server-config-python-postgres.xml` |
| `ADAGUC_LOGFILE`          | File where log messages are written (both informational and error logs). | Not set |
| `ADAGUC_ERRORFILE`        | Optional file that logs only error messages. | Not set |
| `ADAGUC_FONT`             | Path to a default TrueType font (TTF) used for rendering text in generated imagery (e.g. `FreeSans.ttf`). | Not set |
| `ADAGUC_DATARESTRICTION`  | Controls access restrictions for certain services. Possible values: `ALLOW_WCS`, `ALLOW_GFI`, `ALLOW_METADATA`, `SHOW_QUERYINFO`. Values can be combined with `\|`. `FALSE` means no restrictions. | `FALSE` (`ALLOW_WCS\|ALLOW_GFI\|ALLOW_METADATA`) |
| `ADAGUC_PATH`             | Root directory of the adaguc-server code base. Also used for variable substitution (`{ADAGUC_PATH}`) in configuration files. | Current working directory (`pwd`) |
| `ADAGUC_TMP`              | Directory used for temporary files and available as `{ADAGUC_TMP}` substitution in configuration files. | Not set |
| `ADAGUC_ONLINERESOURCE`   | Optional override for the [OnlineResource](configuration/OnlineResource.md) URL used by the CGI service. Can also be configured in the XML configuration file. | Not set |
| `ADAGUC_TRACE_TIMINGS`    | Enables timing measurements for internal operations such as database queries, file access, and image generation. Results are returned in the `x-trace-timings` HTTP header. | `FALSE` |
| `ADAGUC_ENABLELOGBUFFER`  | Controls logging behaviour. `FALSE`: unbuffered logging with live output (may slow service). `TRUE`: buffered logging. `DISABLELOGGING`: disables logging completely. | `FALSE` |
| `ADAGUC_AUTOWMS_DIR`      | Directory used for automatic WMS generation. NetCDF, PNG, CSV, or HDF5 files placed here are automatically exposed through a generated WMS service. | `/data/adaguc-autowms` |
| `ADAGUC_DATA_DIR`         | Directory containing data files, such as NetCDF files used for realtime updates. | `/data/adaguc-data` |
| `ADAGUC_DATASET_DIR`      | Directory containing dataset configuration XML files. | `/data/adaguc-datasets` |
| `ADAGUC_NUMPARALLELPROCESSES` | Number of parallel worker processes used by the adaguc-server. | `4` |
| `ADAGUC_DB`               | PostgreSQL connection string used by the server. | `host=adaguc-db port=5432 user=adaguc password=adaguc dbname=adaguc` |
| `PGBOUNCER_ENABLE`        | Enables or disables PostgreSQL connection pooling through PgBouncer. | `true` |
| `PGBOUNCER_DISABLE_SSL`   | Disables SSL when connecting through PgBouncer. | `true` |
| `ADAGUCENV_RETENTIONPERIOD` | ISO 8601 duration specifying how long generated or cached files should be retained. | Not set |
| `ADAGUCENV_ENABLECLEANUP` | Enables or disables automatic cleanup of files based on the retention period. | Not set |
| `EXTERNALADDRESS`         | Hostname where Adaguc Viewer and Adaguc Explorer are reachable. | Not set |
| `ADAGUC_MAX_PROC_TIMEOUT` | Maximum allowed processing time (in seconds) for a single adaguc-server request. If a request exceeds this limit, the server terminates the process and returns an HTTP 500 error to the client. This prevents requests from running indefinitely and blocking server resources. | 180 |
| `ADAGUC_FORK_ENABLE`      | Enables running adaguc-server in [fork mode](/doc/fork_server.md) when set to `TRUE`. | `FALSE` |