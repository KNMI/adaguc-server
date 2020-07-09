
# ADAGUC scanner container

The ADAGUC scanner container can be used to scan incoming files and datasets. It can also be used to clean the database and or filesystem. This container does not start a webservice, it is meant to be used for scanning incoming files. It is for example used in a cloud environment.

## To obtain or build the scanner:

Pull or build the dataset scanner image:
```
docker pull openearth/adaguc-dataset-scanner
```
or build locally:
```
docker build -t openearth/adaguc-dataset-scanner -f dataset-scanner.Dockerfile .
```

## Possible commands:

The following scripts will automatically connect with the running adaguc-server container.

### Script `adaguc-docker-listdatasets.sh`

1. List the datasets

To list all datasets do:
```
bash ./Docker/scanner/adaguc-docker-listdatasets.sh
```

### Script `adaguc-docker-scanner.sh`


These are the possible commands:
```
    -f: Optional file on the filesystem
    -d: Optional dataset
```

1. Scan all files for all datasets:

```
bash ./Docker/scanner/adaguc-docker-scanner.sh
```


1. To scan all files for a specic dataset:
```
bash ./Docker/scanner/adaguc-docker-scanner.sh -d <datasetname>
```

3. To scan/add a file for any dataset (adaguc will find out which is the correct dataset):

```
bash ./Docker/scanner/adaguc-docker-scanner.sh -f <filename>
```

4. To scan/add a file for a specific dataset:

```
bash ./Docker/scanner/adaguc-docker-scanner.sh -f <filename> -d <datasetname>
```

### Script `adaguc-docker-cleanoldfiles.sh` ###

These are the possible commands:
```
    -p: is for the filepath as configured in your Layers FilePath value
    -f: is for the filefilter as configured in your Layers FilePath filter attribute
    -d: Specify how many days old the files need to be for removal
    -q: Querytype, currently filetimedate, this is the date inside the time variable of the NetCDF file
    -t: Deletetype, delete_db means it will be removed from the db only, delete_db_and_fs will also delete the files from disk
```    

To clean files older than n days from the database:

```
bash adaguc-docker-cleanoldfiles.sh -p /data/adaguc-autowms/EGOWS_radar -f ".*\.nc$" -d 7 -q filetimedate -t delete_db
```

The command above is based on the filepath and filter as specified in the Layer configuration. For example:
```
<Layer type="database">
 ...
  <FilePath filter=".*\.nc$">/data/adaguc-autowms/EGOWS_radar/</FilePath>
  ...
</Layer>
```

