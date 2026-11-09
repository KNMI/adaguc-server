"""
Content-types are brotli compressed, unless they are binary or already compressed, like images and NetCDF
"""

# Binary or already compressed content-types, compression makes them hardly smaller
BR_SKIP_SET = {
    "application/netcdf",
    "application/x-netcdf",
    "application/octet-stream",
    "application/zip",
    "application/gzip",
    "application/x-gzip",
    "application/pdf",
}
BR_SKIP_PREFIXES = ("image/", "video/", "audio/", "font/")
CONTENT_TYPE_HEADER_LOWER = "content-type"


def requires_encoding(headers):
    """
    Content-types are brotli compressed, unless they are binary or already compressed, like images and NetCDF
    """
    for k, value in headers.items():
        if k.lower() == CONTENT_TYPE_HEADER_LOWER:
            # Ignore parameters like charset and compare case insensitive
            media_type = value.split(";")[0].strip().lower()
            return media_type != "" and media_type not in BR_SKIP_SET and not media_type.startswith(BR_SKIP_PREFIXES)
    return False
