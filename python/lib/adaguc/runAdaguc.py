import asyncio
import logging
import os
from PIL import Image
from io import BytesIO
import brotli
import shutil
import random
import string
import redis.asyncio as redis  # This can also be used to connect to a Redis cluster

import calendar
import json
from datetime import datetime

from adaguc.CGIRunner import CGIRunner


class runAdaguc:

    def __init__(self):
        """ADAGUC_LOGFILE is the location where logfiles are stored.
        In current config file adaguc.autoresource.xml, the DB is written to this temporary directory.
        Please note regenerating the DB each time for each request can cause performance problems.
        You can safely configure a permanent location for the database which is permanent in adaguc.autoresource.xml (or your own config)
        """
        self.ADAGUC_LOGFILE = (
            "/tmp/adaguc-server-" + self.get_random_string(10) + ".log"
        )
        self.ADAGUC_PATH = os.getenv("ADAGUC_PATH", "./")
        self.ADAGUC_CONFIG = self.ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"
        self.ADAGUC_DATA_DIR = os.getenv("ADAGUC_DATA_DIR", "/data/adaguc-data")
        self.ADAGUC_AUTOWMS_DIR = os.getenv(
            "ADAGUC_AUTOWMS_DIR", "/data/adaguc-autowms"
        )
        self.ADAGUC_DATASET_DIR = os.getenv(
            "ADAGUC_DATASET_DIR", "/data/adaguc-datasets"
        )
        self.ADAGUC_TMP = os.getenv("ADAGUC_TMP", "/tmp")
        self.ADAGUC_FONT = os.getenv(
            "ADAGUC_FONT", self.ADAGUC_PATH + "/data/fonts/Roboto-Medium.ttf"
        )

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

    def isLoggingEnabled(self):
        return os.getenv(
            "ADAGUC_ENABLELOGBUFFER", "TRUE"
        ) != "DISABLELOGGING"

    def getAdagucEnv(self, adagucenv = {}):
        
        """ Set required environment variables for adaguc to run"""
        adagucenv["ADAGUC_CONFIG"] = self.ADAGUC_CONFIG
        adagucenv["ADAGUC_LOGFILE"] = self.ADAGUC_LOGFILE
        adagucenv["ADAGUC_PATH"] = self.ADAGUC_PATH
        adagucenv["ADAGUC_DATA_DIR"] = self.ADAGUC_DATA_DIR
        adagucenv["ADAGUC_AUTOWMS_DIR"] = self.ADAGUC_AUTOWMS_DIR
        adagucenv["ADAGUC_DATASET_DIR"] = self.ADAGUC_DATASET_DIR
        adagucenv["ADAGUC_TMP"] = self.ADAGUC_TMP
        adagucenv["ADAGUC_FONT"] = self.ADAGUC_FONT
        adagucenv["ADAGUC_DATARESTRICTION"] = "FALSE"
        if os.getenv("ADAGUC_DB"):
            adagucenv["ADAGUC_DB"] = os.getenv("ADAGUC_DB")
        adagucenv["ADAGUC_ENABLELOGBUFFER"] = os.getenv(
            "ADAGUC_ENABLELOGBUFFER", "TRUE"
        )
        adagucenv["ADAGUC_TRACE_TIMINGS"] = os.getenv(
            "ADAGUC_TRACE_TIMINGS", "FALSE"
        )
        ld_library_path = os.getenv("LD_LIBRARY_PATH")
        if ld_library_path:
            adagucenv["LD_LIBRARY_PATH"] = ld_library_path


        return adagucenv

    async def updateLayerMetadata(self):
        """Uses the adaguc executable to update the layermetadatatable"""
        adagucenv = self.getAdagucEnv()
        status, data, headers = await self.runADAGUCServer(
            args=["--updatelayermetadata"], env=adagucenv, isCGI=False, showLogOnError = True
        )

        return status, data.getvalue().decode()        

    def scanDataset(self, datasetName):
        config = self.ADAGUC_CONFIG + "," + datasetName
        adagucenv = self.getAdagucEnv()
        status, data, headers = asyncio.run(
            self.runADAGUCServer(
                args=["--updatedb", "--config", config], env=adagucenv, isCGI=False, showLogOnError = False
            )
        )


        return data.getvalue().decode()

    def runGetMapUrl(self, url):
        adagucenv = self.getAdagucEnv()

        status, data, headers = asyncio.run(
            self.runADAGUCServer(url, env=adagucenv, showLogOnError=False)
        )
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


    async def runADAGUCServer(
        self,
        url=None,
        env=[],
        path=None,
        args=None,
        isCGI=True,
        showLogOnError=True,
        showLog=False,
    ):
        adagucenv = self.getAdagucEnv(env)



        # Forward all environment variables starting with ADAGUCENV_
        prefix: str = "ADAGUCENV_"
        for key, value in os.environ.items():
            if key[: len(prefix)] == prefix:
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


        filetogenerate = BytesIO()
        status, headers, processErr = await CGIRunner().run(
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
            elif isCGI:
                print("Process: No HTTP Headers written")

            print("--- END ADAGUC DEBUG INFO ---\n")

        return status, filetogenerate, headers

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


