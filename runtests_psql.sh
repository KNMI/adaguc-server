#!/bin/bash

# --rootdir = pytest rootdir, helps with finding ./tests/conftest.py
# -v        = verbose logging
# -s        = stdout shows up in console
# -o        = match test file regex
# -k        = select single test (e.g. -k test_CSV_12timesteps)

# Tests must be ran from the ./tests dir
FILTER_FILES=""
if [ -n "$1" ]; then
    FILTER_FILES="-k $1";
fi

cd tests
pytest --rootdir=./ ./AdagucTests/ ../python/python_fastapi_server/ -o python_files="Test*.py test_*.py" $FILTER_FILES
