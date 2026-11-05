"""
Certain content-types need brotli compression, like xml and json
"""

BR_COMPRESS_SET = {"application/json", "application/javascript", "text/xml", "text/html", "text/plain"}
CONTENT_TYPE_HEADER_LOWER = "content-type"


def requires_encoding(headers):
    """
    Certain content-types need brotli compression, like xml and json
    """
    for k, value in headers.items():
        if k.lower() == CONTENT_TYPE_HEADER_LOWER:
            return value in BR_COMPRESS_SET
    return False
