import png

TEXT_CHUNK_FLAG = b"tEXt"


def generate_chunk_tuple(type_flag, content):
    return tuple([type_flag, content])


def generate_text_chunk_tuple(str_info):
    type_flag = TEXT_CHUNK_FLAG
    return generate_chunk_tuple(type_flag, bytes(str_info, "utf-8"))


def insert_text_chunk(target, text, index=1):
    if index < 0:
        raise Exception("The index value {} less than 0!".format(index))

    reader = png.Reader(filename=target)
    chunks = reader.chunks()
    chunk_list = list(chunks)
    print(chunk_list[0])
    print(chunk_list[1])
    print(chunk_list[2])
    chunk_item = generate_text_chunk_tuple(text)
    chunk_list.insert(index, chunk_item)

    with open(target, "wb") as dst_file:
        png.write_chunks(dst_file, chunk_list)


def _insert_text_chunk_to_png_test():
    src = "/home/plieger/code/github/KNMI/adaguc-server/data/datasets/MTG-FCI-FD_eur_atlantic_1km_true_color_202603171240-small.png"
    insert_text_chunk(src, "time\0" + "2026-03-17T12:40:00Z")
    insert_text_chunk(src, "bbox\0" + "-5570248.686685662,5570248.686685662,5567248.28340708,2000000.0")
    insert_text_chunk(
        src, "proj4_params\0" + " +proj=geos +lon_0=0 +h=35785831 +x_0=0 +y_0=0 +a=6378169 +rf=295.488065897001 +units=m +no_defs +type=crs"
    )


if __name__ == "__main__":
    _insert_text_chunk_to_png_test()
