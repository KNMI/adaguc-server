Settings (enablecleanupsystem)
===============

Back to [Configuration](./Configuration.md)

Available since: 2.8.6

- enablecleanupsystem (false | true | dryrun): Enables the auto cleanup system. See [FilePath](./FilePath.md) and retentionperiod for details
- cleanupsystemlimit (number): Configures how many files will be deleted at once, defaults to 10 (CDBFILESCANNER_CLEANUP_DEFAULT_LIMIT).

Allowed values for enablecleanupsystem are:
- false (or nothing)
- true - Enables the cleanup, deletes files from the filesystem and from the database
- dryrun - Informs which files would be removed, like a dry run.



```xml
<Settings enablecleanupsystem="true" cleanupsystemlimit="10"/>
```

This can be configured in the main Configuration section or a dataset
configuration. The last one found is used.
