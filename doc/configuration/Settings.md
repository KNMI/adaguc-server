Settings (enablemetadatacache, enablecleanupsystem, cleanupsystemlimit, cache_age_cacheableresources, cache_age_volatileresources)

===============

Back to [Configuration](./Configuration.md)


- enablemetadatacache (false | true): Defaults to true, uses a layermetadata table in the db to cache layer information. This information is updated during a scan.
- enablecleanupsystem (false | true | dryrun): Enables the auto cleanup system. See [FilePath](./FilePath.md) and retentionperiod for details
- cleanupsystemlimit (number): Configures how many files will be deleted at once, defaults to 10 (CDBFILESCANNER_CLEANUP_DEFAULT_LIMIT).
- cache_age_cacheableresources (number) defaults to 0 (disabled), Sets the cache header for fully specified getmap requests
- cache_age_volatileresources (number), defaults to 0 (disabled), Sets the cache header for things which often change, like a getcapabilities document.

Allowed values for enablecleanupsystem are:
- false (or nothing)
- true - Enables the cleanup, deletes files from the filesystem and from the database
- dryrun - Informs which files would be removed, like a dry run.



```xml
<Settings enablecleanupsystem="true" cleanupsystemlimit="10"/>
```

This can be configured in the main Configuration section or a dataset
configuration. The last one found is used.

## Caching example


<Settings cache_age_volatileresources="60" cache_age_cacheableresources="7200"/>

- Volatile resources like the GetCapabilities document, or GetMap requests without fully specified dimensions are only cached for 60 seconds.
- Fully cacheable resources, in this case a GetMap request with fully specified dimensions are cached for 7200 seconds.