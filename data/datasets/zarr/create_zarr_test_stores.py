"""
Writes the test data for the Zarr reader: one NetCDF file and the same data as Zarr v2 stores with different encodings.
The tests read the NetCDF file and each store, and expect the same variables, attributes and data.

The stores are written without the zarr package, so that every part of the format can be chosen:

- consolidated_zlib_shuffle.zarr: consolidated metadata, zlib with the shuffle filter, C order, chunks with partial edges,
  chunks that only hold the fill value are not written
- unconsolidated_gzip_forder.zarr: no consolidated metadata, gzip, F order, big endian, dimension separator /
- consolidated_zstd.zarr: zstd, one chunk per time step
- consolidated_blosc.zarr: blosc with its own byte shuffle
- consolidated_uncompressed.zarr: no compressor, one chunk per array

Usage (writes next to this script): python3 create_zarr_test_stores.py
zstd and blosc are used through their C libraries (libzstd, libblosc).
"""

import ctypes
import ctypes.util
import gzip
import json
import os
import shutil
import zlib

import netCDF4
import numpy as np

OUTPUT_DIR = os.path.dirname(os.path.abspath(__file__))
NETCDF_FILE = os.path.join(OUTPUT_DIR, "zarrtest.nc")

NUM_TIME, NUM_Z, NUM_LAT, NUM_LON, NUM_STATION, NUM_LEVEL = 2, 3, 30, 40, 5, 4


def make_data():
    """The variables as (name, dimensions, numpy array, attributes, zarr fill_value)"""
    rng = np.random.default_rng(42)
    lat = np.linspace(50.0, 55.8, NUM_LAT, dtype="f4")
    lon = np.linspace(2.0, 9.8, NUM_LON, dtype="f4")
    time = np.array([1716854400.0, 1716854700.0], dtype="f8")
    z = np.array([0.5, 1.0, 1.5], dtype="f4")

    temperature = (rng.normal(280, 5, (NUM_TIME, NUM_Z, NUM_LAT, NUM_LON))).astype("f4")
    temperature[:, :, :18, :26] = np.nan  # Whole chunks with only the fill value
    temperature[0, 0, 20, 30] = np.nan

    reflectivity = rng.integers(0, 6000, (NUM_TIME, NUM_Z, NUM_LAT, NUM_LON)).astype("u2")
    reflectivity[:, 1, :, :] = 65535  # A whole level with only the fill value
    reflectivity[1, :, 16:, 25:] = 65535

    counts = rng.integers(-100000, 100000, (NUM_LAT, NUM_LON)).astype("i4")
    flag = (rng.random((NUM_LAT, NUM_LON)) > 0.5).astype("u1")
    names = ["De Bilt", "Cabauw", "Schiphol", "Eelde", "Vlissingen éè"]
    codes = ["DBLT", "CBW", "SPL", "EELD", "VLS"]
    labels = ["bilt", "cabauw", "échi", "eelde", "vliss"]
    humidity = rng.random((NUM_LAT, NUM_LON)).astype("f8")
    level_values = np.array([1000, 850, 500, 250], dtype="i2")

    lat_lon = ["lat", "lon"]
    grid = ["time", "z", "lat", "lon"]
    return [
        ("lat", ["lat"], lat, {"units": "degrees_north", "standard_name": "latitude"}, "NaN"),
        ("lon", ["lon"], lon, {"units": "degrees_east", "standard_name": "longitude"}, "NaN"),
        ("time", ["time"], time, {"units": "seconds since 1970-01-01 00:00:00", "standard_name": "time"}, "NaN"),
        ("z", ["z"], z, {"units": "km", "long_name": "height"}, "NaN"),
        ("temperature", grid, temperature, {"units": "K", "standard_name": "air_temperature", "_FillValue": "NaN", "valid_range": [200.0, 350.0]}, "NaN"),
        ("reflectivity", grid, reflectivity, {"units": "dBZ", "scale_factor": 0.01, "add_offset": -30.0, "_FillValue": 65535, "flag_values": [1, 2, 3]}, 65535),
        ("counts", lat_lon, counts, {"long_name": "counts", "big_number": 5000000000}, None),
        ("flag", lat_lon, flag, {"long_name": "a flag"}, None),
        ("station_name", ["station"], names, {"long_name": "station name"}, None),
        ("station_code", ["station"], codes, {"long_name": "station code"}, None),
        ("station_label", ["station"], labels, {"long_name": "station label"}, None),
        ("extra/humidity", lat_lon, humidity, {"units": "1", "_FillValue": -1.0}, -1.0),
        ("extra/level_values", ["level"], level_values, {"units": "hPa"}, None),
    ]


GLOBAL_ATTRIBUTES = {"Conventions": "CF-1.8", "title": "Zarr reader test data", "version": 3}


def write_netcdf(variables):
    with netCDF4.Dataset(NETCDF_FILE, "w") as dataset:
        dataset.setncatts(GLOBAL_ATTRIBUTES)
        dataset.setncattr_string("keywords", ["zarr", "test"])
        extra = dataset.createGroup("extra")
        for name, size in [("time", NUM_TIME), ("z", NUM_Z), ("lat", NUM_LAT), ("lon", NUM_LON), ("station", NUM_STATION)]:
            dataset.createDimension(name, size)
        extra.createDimension("level", NUM_LEVEL)
        for name, dimensions, data, attributes, _ in variables:
            group = extra if name.startswith("extra/") else dataset
            short_name = name.split("/")[-1]
            if isinstance(data, list):
                variable = group.createVariable(short_name, str, dimensions)
                for index, text in enumerate(data):
                    variable[index] = text
            else:
                fill = attributes.get("_FillValue")
                fill_value = np.nan if fill == "NaN" else fill
                variable = group.createVariable(short_name, data.dtype, dimensions, fill_value=fill_value)
                variable.set_auto_maskandscale(False)
                variable[:] = data
            for key, value in attributes.items():
                if key != "_FillValue":
                    variable.setncattr(key, value)


class Encoding:
    def __init__(self, name, compressor, filters=None, order="C", byte_order="<", separator=".", consolidated=True, chunking="small"):
        self.name = name
        self.compressor = compressor
        self.filters = filters
        self.order = order
        self.byte_order = byte_order
        self.separator = separator
        self.consolidated = consolidated
        self.chunking = chunking


def load_library(name):
    path = ctypes.util.find_library(name)
    if path is None:
        raise RuntimeError(f"lib{name} is needed to write the test stores")
    return ctypes.CDLL(path)


def compress(data: bytes, compressor, element_size):
    if compressor is None:
        return data
    if compressor["id"] == "zlib":
        return zlib.compress(data, compressor["level"])
    if compressor["id"] == "gzip":
        return gzip.compress(data, compressor["level"], mtime=0)
    if compressor["id"] == "zstd":
        zstd = load_library("zstd")
        zstd.ZSTD_compressBound.restype = ctypes.c_size_t
        zstd.ZSTD_compress.restype = ctypes.c_size_t
        bound = zstd.ZSTD_compressBound(ctypes.c_size_t(len(data)))
        output = ctypes.create_string_buffer(bound)
        size = zstd.ZSTD_compress(output, ctypes.c_size_t(bound), data, ctypes.c_size_t(len(data)), compressor["level"])
        return output.raw[:size]
    if compressor["id"] == "blosc":
        blosc = load_library("blosc")
        blosc.blosc_set_compressor(compressor["cname"].encode())
        output = ctypes.create_string_buffer(len(data) + 16)
        size = blosc.blosc_compress(compressor["clevel"], compressor["shuffle"], element_size, ctypes.c_size_t(len(data)), data, output, ctypes.c_size_t(len(data) + 16))
        if size <= 0:
            raise RuntimeError("blosc_compress failed")
        return output.raw[:size]
    raise ValueError(compressor)


def shuffle(data: bytes, element_size):
    count = len(data) // element_size
    return np.frombuffer(data, dtype="u1").reshape(count, element_size).T.tobytes()


def encode_vlen_utf8(strings):
    parts = [np.uint32(len(strings)).tobytes()]
    for text in strings:
        encoded = text.encode("utf-8")
        parts.append(np.uint32(len(encoded)).tobytes() + encoded)
    return b"".join(parts)


def chunk_shape(encoding, shape):
    if encoding.chunking == "single" or len(shape) < 2:
        return list(shape)
    if encoding.chunking == "per_time":
        return [1] * (len(shape) - 2) + list(shape[-2:])
    if encoding.chunking == "odd":
        sizes = {4: [2, 3, 20, 17], 2: [20, 17]}
        return sizes[len(shape)]
    sizes = {4: [1, 2, 16, 25], 2: [16, 25]}
    return sizes[len(shape)]


def zarr_dtype(name, data, encoding):
    """The dtype as written in .zarray, the numpy dtype of the chunks, and the filters"""
    if name == "station_name":
        return "|O", None, [{"id": "vlen-utf8"}]
    if name == "station_code":
        dtype = np.dtype("S4")
    elif name == "station_label":
        dtype = np.dtype(f"{encoding.byte_order}U6")
    elif name == "flag":
        dtype = np.dtype("?")
    else:
        dtype = data.dtype.newbyteorder(encoding.byte_order) if data.dtype.itemsize > 1 else data.dtype
    # Like numcodecs, the shuffle filter works on the size of the elements of the array
    filters = [{**f, "elementsize": dtype.itemsize} if f["id"] == "shuffle" else f for f in encoding.filters or []]
    return dtype.str, dtype, filters


def write_store(variables, encoding):
    store = os.path.join(OUTPUT_DIR, encoding.name)
    shutil.rmtree(store, ignore_errors=True)
    os.makedirs(store)
    metadata = {}

    def put_json(key, value):
        metadata[key] = value
        path = os.path.join(store, key)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w") as file:
            json.dump(value, file, indent=2)

    put_json(".zgroup", {"zarr_format": 2})
    put_json(".zattrs", {**GLOBAL_ATTRIBUTES, "keywords": ["zarr", "test"]})
    put_json("extra/.zgroup", {"zarr_format": 2})
    put_json("extra/.zattrs", {"description": "a group"})

    for name, dimensions, data, attributes, fill_value in variables:
        dtype_text, chunk_dtype, filters = zarr_dtype(name, data, encoding)
        is_strings = isinstance(data, list)
        array = np.array(data, dtype=object) if is_strings else data
        shape = list(array.shape)
        chunks = chunk_shape(encoding, shape)
        if is_strings:
            fill_value = None
        put_json(
            f"{name}/.zarray",
            {
                "zarr_format": 2,
                "shape": shape,
                "chunks": chunks,
                "dtype": dtype_text,
                "fill_value": fill_value,
                "order": encoding.order,
                "compressor": encoding.compressor,
                "filters": filters or None,
                "dimension_separator": encoding.separator,
            },
        )
        put_json(f"{name}/.zattrs", {**attributes, "_ARRAY_DIMENSIONS": dimensions})

        grid = [range((size + chunk - 1) // chunk) for size, chunk in zip(shape, chunks)]
        for chunk_index in np.ndindex(*[len(r) for r in grid]):
            selection = tuple(slice(i * c, min((i + 1) * c, s)) for i, c, s in zip(chunk_index, chunks, shape))
            part = array[selection]
            if not is_strings and fill_value is not None:
                fill = np.nan if fill_value == "NaN" else fill_value
                if np.all(part == fill) or (fill_value == "NaN" and np.all(np.isnan(part))):
                    continue  # Chunks with only the fill value are not written
            # A chunk always has the full chunk shape; the part outside the array is padding
            if is_strings and chunk_dtype is None:
                padded = np.full(chunks, "", dtype=object)
                padded[tuple(slice(0, n) for n in part.shape)] = part
                raw = encode_vlen_utf8(list(padded.ravel(order=encoding.order)))
                element_size = 1
            else:
                padded = np.zeros(chunks, dtype=chunk_dtype)
                padded[tuple(slice(0, n) for n in part.shape)] = part
                raw = padded.tobytes(order=encoding.order)
                element_size = chunk_dtype.itemsize
            for chunk_filter in filters or []:
                if chunk_filter["id"] == "shuffle" and chunk_filter["elementsize"] > 1:
                    raw = shuffle(raw, chunk_filter["elementsize"])
            key = encoding.separator.join(str(i) for i in chunk_index)
            path = os.path.join(store, name, key)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "wb") as file:
                file.write(compress(raw, encoding.compressor, element_size))

    if encoding.consolidated:
        with open(os.path.join(store, ".zmetadata"), "w") as file:
            json.dump({"metadata": metadata, "zarr_consolidated_format": 1}, file, indent=2)


ENCODINGS = [
    Encoding("consolidated_zlib_shuffle.zarr", {"id": "zlib", "level": 4}, filters=[{"id": "shuffle"}]),
    Encoding("unconsolidated_gzip_forder.zarr", {"id": "gzip", "level": 5}, order="F", byte_order=">", separator="/", consolidated=False, chunking="odd"),
    Encoding("consolidated_zstd.zarr", {"id": "zstd", "level": 3}, chunking="per_time"),
    Encoding("consolidated_blosc.zarr", {"id": "blosc", "cname": "lz4", "clevel": 5, "shuffle": 1, "blocksize": 0}),
    Encoding("consolidated_uncompressed.zarr", None, chunking="single"),
]


if __name__ == "__main__":
    variables = make_data()
    write_netcdf(variables)
    for encoding in ENCODINGS:
        write_store(variables, encoding)
    print("Wrote", NETCDF_FILE, "and", ", ".join(e.name for e in ENCODINGS))
