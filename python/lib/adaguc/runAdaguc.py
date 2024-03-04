from ast import Dict
import subprocess
import os
from os.path import expanduser
from PIL import Image
from io import BytesIO
import io
import tempfile
import shutil
import random
import string
from redis import from_url

import re
import calendar
import json
from datetime import datetime, timedelta

from adaguc.CGIRunner import CGIRunner


class runAdaguc:

    def __init__(self):
        """ADAGUC_LOGFILE is the location where logfiles are stored.
        In current config file adaguc.autoresource.xml, the DB is written to this temporary directory.
        Please note regenerating the DB each time for each request can cause performance problems.
        You can safely configure a permanent location for the database which is permanent in adaguc.autoresource.xml (or your own config)
        """
        self.ADAGUC_LOGFILE = ("/tmp/adaguc-server-" +
                               self.get_random_string(10) + ".log")
        self.ADAGUC_PATH = os.getenv("ADAGUC_PATH", "./")
        self.ADAGUC_CONFIG = self.ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"
        self.ADAGUC_DATA_DIR = os.getenv("ADAGUC_DATA_DIR",
                                         "/data/adaguc-data")
        self.ADAGUC_AUTOWMS_DIR = os.getenv("ADAGUC_AUTOWMS_DIR",
                                            "/data/adaguc-autowms")
        self.ADAGUC_DATASET_DIR = os.getenv("ADAGUC_DATASET_DIR",
                                            "/data/adaguc-datasets")
        self.ADAGUC_TMP = os.getenv("ADAGUC_TMP", "/tmp")
        self.ADAGUC_FONT = os.getenv(
            "ADAGUC_FONT", self.ADAGUC_PATH + "/data/fonts/Roboto-Medium.ttf")
        self.ADAGUC_REDIS = os.getenv("ADAGUC_REDIS", "")
        if self.ADAGUC_REDIS.startswith("redis://") or self.ADAGUC_REDIS.startswith("rediss://"):
            self.redis_url = self.ADAGUC_REDIS
            self.use_cache = True
            self.redis_client = from_url(self.redis_url)
            print("USE CACHE")
        else:
            self.use_cache = False

    def setAdagucPath(self, newAdagucPath):
        self.ADAGUC_PATH = newAdagucPath

    def setDatasetDir(self, newDataSetDir):
        self.ADAGUC_DATASET_DIR = newDataSetDir

    def setDataDir(self, newDataDir):
        self.ADAGUC_DATA_DIR = newDataDir

    def setAutoWMSDir(self, newAutoWMSDir):
        self.ADAGUC_AUTOWMS_DIR = newAutoWMSDir

    def setTmpDir(self, newTmpDir):
        self.ADAGUC_TMP = newTmpDir

    def setFontDir(self, newFontDir):
        self.ADAGUC_FONT = newFontDir

    def get_random_string(self, length):
        letters = string.ascii_lowercase
        return "".join(random.choice(letters) for i in range(length))

    def setConfiguration(self, configFile):
        self.ADAGUC_CONFIG = configFile

    def scanDataset(self, datasetName):
        config = self.ADAGUC_CONFIG + "," + datasetName
        """ Setup a new environment """
        adagucenv = {}
        """ Set required environment variables """
        adagucenv["ADAGUC_CONFIG"] = self.ADAGUC_CONFIG
        adagucenv["ADAGUC_LOGFILE"] = self.ADAGUC_LOGFILE
        adagucenv["ADAGUC_PATH"] = self.ADAGUC_PATH
        adagucenv["ADAGUC_DATA_DIR"] = self.ADAGUC_DATA_DIR
        adagucenv["ADAGUC_AUTOWMS_DIR"] = self.ADAGUC_AUTOWMS_DIR
        adagucenv["ADAGUC_DATASET_DIR"] = self.ADAGUC_DATASET_DIR
        adagucenv["ADAGUC_TMP"] = self.ADAGUC_TMP
        adagucenv["ADAGUC_FONT"] = self.ADAGUC_FONT

        status, data, headers = self.runADAGUCServer(
            args=["--updatedb", "--config", config],
            env=adagucenv,
            isCGI=False)

        return data.getvalue().decode()

    def runGetMapUrl(self, url):
        adagucenv = {}
        """ Set required environment variables """
        adagucenv["ADAGUC_CONFIG"] = self.ADAGUC_CONFIG
        adagucenv["ADAGUC_LOGFILE"] = self.ADAGUC_LOGFILE
        adagucenv["ADAGUC_PATH"] = self.ADAGUC_PATH
        adagucenv["ADAGUC_DATA_DIR"] = self.ADAGUC_DATA_DIR
        adagucenv["ADAGUC_AUTOWMS_DIR"] = self.ADAGUC_AUTOWMS_DIR
        adagucenv["ADAGUC_DATASET_DIR"] = self.ADAGUC_DATASET_DIR
        adagucenv["ADAGUC_TMP"] = self.ADAGUC_TMP
        adagucenv["ADAGUC_FONT"] = self.ADAGUC_FONT
        status, data, headers = self.runADAGUCServer(url,
                                                     env=adagucenv,
                                                     showLogOnError=False)
        logfile = self.getLogFile()
        self.removeLogFile()
        if data is not None:
            image = None
            try:
                image = Image.open(data)
            except:
                pass
            return image, logfile
        else:
            return None, logfile

    def removeLogFile(self):
        try:
            os.remove(self.ADAGUC_LOGFILE)
        except:
            pass

    def getLogFile(self):
        try:
            f = open(self.ADAGUC_LOGFILE, encoding="utf8", errors="ignore")
            data = f.read()
            f.close()
            return data
        except:
            pass
        return ""

    def printLogFile(self):
        ADAGUC_LOGFILE = self.ADAGUC_LOGFILE
        print("\n=== START ADAGUC LOGS ===")
        print(self.getLogFile())
        print("=== END ADAGUC LOGS ===")

    def cache_wanted(self, url: str):
        if not self.use_cache:
            return False
        if "request=getcapabilities" in url.lower():
            return True
        return False

    def runADAGUCServer(
        self,
        url=None,
        env=[],
        path=None,
        args=None,
        isCGI=True,
        showLogOnError=True,
        showLog=False,
    ):
        # adagucenv=os.environ.copy()
        # adagucenv.update(env)

        # url = re.sub(r"^(.*)(&[0-9\.]*)$", r"\g<1>&1", url) # This removes adaguc-viewer's unique-URL feature

        adagucenv = env

        adagucenv["ADAGUC_ENABLELOGBUFFER"] = os.getenv(
            "ADAGUC_ENABLELOGBUFFER", "TRUE")
        adagucenv["ADAGUC_CONFIG"] = self.ADAGUC_CONFIG
        adagucenv["ADAGUC_LOGFILE"] = self.ADAGUC_LOGFILE
        adagucenv["ADAGUC_PATH"] = self.ADAGUC_PATH
        adagucenv["ADAGUC_DATARESTRICTION"] = "FALSE"
        adagucenv["ADAGUC_DATA_DIR"] = self.ADAGUC_DATA_DIR
        adagucenv["ADAGUC_AUTOWMS_DIR"] = self.ADAGUC_AUTOWMS_DIR
        adagucenv["ADAGUC_DATASET_DIR"] = self.ADAGUC_DATASET_DIR
        adagucenv["ADAGUC_TMP"] = self.ADAGUC_TMP
        adagucenv["ADAGUC_FONT"] = self.ADAGUC_FONT
        ld_library_path = os.getenv("LD_LIBRARY_PATH")
        if ld_library_path:
            adagucenv["LD_LIBRARY_PATH"] = ld_library_path

        # Forward all environment variables starting with ADAGUCENV_
        prefix: str = "ADAGUCENV_"
        for key, value in os.environ.items():
            if key[:len(prefix)] == prefix:
                adagucenv[key] = value

        ADAGUC_PATH = adagucenv["ADAGUC_PATH"]
        ADAGUC_LOGFILE = adagucenv["ADAGUC_LOGFILE"]

        try:
            os.remove(ADAGUC_LOGFILE)
        except:
            pass

        adagucexecutable = ADAGUC_PATH + "/bin/adagucserver"

        adagucargs = [adagucexecutable]

        if args is not None:
            adagucargs = adagucargs + args

        # Check cache for entry with keys of (url,adagucargs) if configured
        if self.cache_wanted(url):
            cache_key = str((url, adagucargs))
            print(f"Checking cache for {cache_key}")

            age, headers, data = get_cached_response(self.redis_client, cache_key)
            if age is not None:
               return [0, data, headers]

            print(f"Generating {cache_key}")

        filetogenerate = BytesIO()
        status, headers, processErr = CGIRunner().run(
            adagucargs,
            url=url,
            output=filetogenerate,
            env=adagucenv,
            path=path,
            isCGI=isCGI,
        )

        if (status != 0 and showLogOnError == True) or showLog == True:
            print("\n\n--- START ADAGUC DEBUG INFO ---")
            print("Adaguc-server has non zero exit status %d " % status)
            if isCGI == False:
                print(filetogenerate.getvalue())
            else:
                self.printLogFile()
            if status == -9:
                print("Process: Killed")
            if status == -11:
                print("Process: Segmentation Fault ")

            if len(headers) != 0:
                print("=== START ADAGUC HTTP HEADER ===")
                print(headers)
                print("=== END ADAGUC HTTP HEADER ===")
            else:
                print("Process: No HTTP Headers written")

            print("--- END ADAGUC DEBUG INFO ---\n")
            return status, filetogenerate, headers

        else:
            if self.cache_wanted(url):
                print(f"CACHING {cache_key}")
                response_to_cache(self.redis_client, cache_key, headers, filetogenerate)
            print("HEADERS:", headers)
            return [status, filetogenerate, headers]

    def writetofile(self, filename, data):
        with open(filename, "wb") as f:
            f.write(data)

    def readfromfile(self, filename):
        ADAGUC_PATH = os.environ["ADAGUC_PATH"]
        with open(ADAGUC_PATH + "/tests/" + filename, "rb") as f:
            return f.read()

    def cleanTempDir(self):
        ADAGUC_TMP = os.environ["ADAGUC_TMP"]
        try:
            shutil.rmtree(ADAGUC_TMP)
        except:
            pass
        self.mkdir_p(os.environ["ADAGUC_TMP"])
        return

    def mkdir_p(self, directory):
        if not os.path.exists(directory):
            os.makedirs(directory)

skip_headers = ["x-process-time", "age"]
def response_to_cache(redis_client, key, headers:str, data):
    cacheable_headers=[]
    ttl = 0
    for header in headers:
        k,v = header.split(":")
        if k not in skip_headers:
            cacheable_headers.append(header)
        if k.lower().startswith("cache-control"):
            for term in v.split(";"):
                if term.startswith("max-age"):
                    try:
                        ttl = int(term.split('=')[1])
                    except:
                        pass

    if ttl>0:
        cacheable_headers_json=json.dumps(cacheable_headers, ensure_ascii=True).encode('utf-8')

        entrytime=f"{calendar.timegm(datetime.utcnow().utctimetuple()):10d}".encode('utf-8')
        redis_client.set(key, entrytime+f"{len(cacheable_headers_json):06d}".encode('utf-8')+cacheable_headers_json+data.getvalue(), ex=ttl)

def get_cached_response(redis_client, key):
    cached = redis_client.get(key)
    redis_client.close()
    if not cached:
        print("Cache miss")
        return None, None, None
    print("Cache hit", len(cached))

    entrytime=int(cached[:10].decode('utf-8'))
    currenttime=calendar.timegm(datetime.utcnow().utctimetuple())
    age=currenttime-entrytime

    headers_len=int(cached[10:16].decode('utf-8'))
    headers=json.loads(cached[16:16+headers_len])
    headers.append(f"age: {age}")

    data = cached[16+headers_len:]
    print(f"HIT: {len(data)} bytes cached")
    return age, headers, BytesIO(data)
