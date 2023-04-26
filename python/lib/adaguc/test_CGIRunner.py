import math
import time

#  cd  ./python/lib/adaguc/
# pytest -rpP

from .CGIRunner import CGIRunner as CGIRunner
from io import BytesIO


def testCGIResponse80px():

    filetogenerate = BytesIO()
    cgiRunner = CGIRunner()
    status, headers = cgiRunner.run([
        "python3", "mockCGIResponse.py",
        "mockcgidata_headersandcontent80px.bin"
    ],
                                    "",
                                    output=filetogenerate,
                                    env=[])
    extractedPng = filetogenerate.getvalue()

    assert status == 0
    assert headers == ['Content-Type:image/png']

    with open('mockcgidata_out80px.png', mode='rb') as file:
        expectedPng = file.read()

    assert extractedPng == expectedPng


def testCGIResponse8000px():

    filetogenerate = BytesIO()
    cgiRunner = CGIRunner()
    status, headers = cgiRunner.run([
        "python3", "mockCGIResponse.py",
        "mockcgidata_headersandcontent8000px.bin"
    ],
                                    "",
                                    output=filetogenerate,
                                    env=[])
    extractedPng = filetogenerate.getvalue()

    assert status == 0
    assert headers == ['Content-Type:image/png']

    with open('mockcgidata_out8000px.png', mode='rb') as file:
        expectedPng = file.read()

    assert extractedPng == expectedPng
