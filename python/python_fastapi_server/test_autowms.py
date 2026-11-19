import pytest
from fastapi import HTTPException
from routers.autowms import list_data_files

ONLINE_RESOURCE = "http://localhost:8080/"
PREFIX = "/adaguc::data"


@pytest.fixture
def data_dir(tmp_path):
    (tmp_path / "file.nc").write_bytes(b"")
    (tmp_path / "notes.txt").write_bytes(b"")
    (tmp_path / "subdir").mkdir()
    (tmp_path / ".hidden").mkdir()
    (tmp_path / "subdir" / ".hidden.zarr").mkdir()
    store = tmp_path / "subdir" / "RAD_202405280000.zarr"
    store.mkdir()
    (store / ".zmetadata").write_text("{}")
    (store / "reflectivity").mkdir()
    (tmp_path / "subdir" / "UPPER.ZARR").mkdir()
    return tmp_path


def test_list_data_files_lists_allowed_files_and_directories(data_dir):
    data = list_data_files(str(data_dir), PREFIX, ONLINE_RESOURCE, PREFIX)
    assert data == [
        {"path": "/adaguc::data/subdir", "name": "subdir", "leaf": False},
        {"path": "/adaguc::data/file.nc", "adaguc": "http://localhost:8080/adagucserver?source=file.nc&", "name": "file.nc", "leaf": True},
    ]


def test_list_data_files_lists_zarr_stores_as_files(data_dir):
    data = list_data_files(str(data_dir), f"{PREFIX}/subdir", ONLINE_RESOURCE, PREFIX)
    assert data == [
        {
            "path": "/adaguc::data/subdir/RAD_202405280000.zarr",
            "adaguc": "http://localhost:8080/adagucserver?source=subdir%2FRAD_202405280000.zarr&",
            "name": "RAD_202405280000.zarr",
            "leaf": True,
        },
        {
            "path": "/adaguc::data/subdir/UPPER.ZARR",
            "adaguc": "http://localhost:8080/adagucserver?source=subdir%2FUPPER.ZARR&",
            "name": "UPPER.ZARR",
            "leaf": True,
        },
    ]


def test_list_data_files_refuses_path_traversal(data_dir):
    with pytest.raises(HTTPException):
        list_data_files(str(data_dir / "subdir"), f"{PREFIX}/../..", ONLINE_RESOURCE, PREFIX)
