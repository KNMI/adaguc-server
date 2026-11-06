import asyncio
import datetime
import filecmp
import json
import logging
import os
from io import BytesIO
import shutil
from .CGIRunner import CGIRunner
import re
from lxml import etree, objectify
from . import imagecompare
import subprocess
from deepdiff import DeepDiff

logging.getLogger("PIL").setLevel(logging.WARNING)

ADAGUC_PATH = os.getenv("ADAGUC_PATH", " ")


class AdagucTestTools:

    def getLogFile(self):
        ADAGUC_LOGFILE = os.environ["ADAGUC_LOGFILE"]
        try:
            f = open(ADAGUC_LOGFILE, encoding="utf-8")
            data = f.read()
            f.close()
            return data
        except Exception:
            pass
        return ""

    def getLogFileSize(self):
        ADAGUC_LOGFILE = os.environ["ADAGUC_LOGFILE"]
        file_stats = os.stat(ADAGUC_LOGFILE)
        return file_stats.st_size

    def printLogFile(self):
        print("\n=== START ADAGUC LOGS ===")
        print(self.getLogFile())
        print("=== END ADAGUC LOGS ===")

    def runADAGUCServer(
        self,
        url: str = None,
        env: dict = None,
        path: str | None = None,
        args: dict = None,
        isCGI: bool = True,
        showLogOnError: bool = True,
        showLog: bool = False,
        maxLogFileSize: int = 8192,
    ):
        if env is None:
            env = []

        adagucenv = os.environ.copy()
        adagucenv.update(env)

        adagucenv["LD_LIBRARY_PATH"] = os.getenv("LD_LIBRARY_PATH", "")

        ADAGUC_LOGFILE = os.environ["ADAGUC_LOGFILE"]

        try:
            os.remove(ADAGUC_LOGFILE)
        except Exception:
            pass

        adagucexecutable = ADAGUC_PATH + "/bin/adagucserver"

        adagucargs = [adagucexecutable]

        if args is not None:
            adagucargs = adagucargs + args

        os.chdir(ADAGUC_PATH + "/tests")

        status, headers, processErr, output = asyncio.run(
            CGIRunner().run(
                adagucargs,
                url=url,
                env=adagucenv,
                path=path,
                isCGI=isCGI,
            )
        )

        # Convert HTTP status codes
        if status == 32:
            status = 404
        elif status == 33:
            status = 422

        if (status != 0 and showLogOnError == True) or showLog == True:
            print("LOG:", ADAGUC_LOGFILE)
            print("\n\n--- START ADAGUC DEBUG INFO ---")
            if status != 0:
                print(f"Adaguc-server has non zero exit status {status}")
            if isCGI == False:
                print(output.decode())
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
            return status, output, headers

        else:
            # The executable wrote to stderr, which is unwanted behaviour. Stderr should be empty when running adaguc-server.
            if processErr:
                # print(processErr.decode())
                print("[WARNING]: the process should not write to stderr")
            # Check if the code did not write a too big logfile
            logfileSize = self.getLogFileSize()
            if logfileSize > maxLogFileSize:
                self.printLogFile()
                print("[WARNING]: Adaguc-server writes too many lines to the logfile, size = %d kilobytes" % (logfileSize / 1024))
            return status, output, headers

    def writetofile(self, filename: str, data):
        chars = set(":+,@#$%^&*()\\")
        if any((c in chars) for c in filename):
            raise Exception("Filename %s contains invalid characters" % filename)
        with open(filename, "wb") as f:
            f.write(data)

    def writetojson(self, filename, data):
        chars = set(":+,@#$%^&*()\\")
        if any((c in chars) for c in filename):
            raise Exception("Filename %s contains invalid characters" % filename)
        jsonobject = json.loads(data)
        jsonstring = json.dumps(jsonobject, indent=2)
        with open(filename, "w") as f:
            f.write(jsonstring)

    def readfromfile(self, filename):
        adagucpath = os.getenv("ADAGUC_PATH")
        if adagucpath:
            filename = filename.replace("{ADAGUC_PATH}/", adagucpath)
        with open(filename, "rb") as f:
            return f.read()

    def cleanTempDir(self):
        ADAGUC_TMP = os.getenv("ADAGUC_TMP", "adaguctmp")
        try:
            shutil.rmtree(ADAGUC_TMP)
        except Exception:
            pass
        self.mkdir_p(ADAGUC_TMP)
        return

    def cleanPostgres(self):
        """Clean the postgres database if the ADAGUC_DB variable has been set.

        Some tests fail when using postgres if there is already data present in the database.
        Running the test separately works."""

        adaguc_db = os.getenv("ADAGUC_DB", None)
        if adaguc_db and not adaguc_db.endswith(".db"):
            subprocess.run(
                [
                    "psql",
                    adaguc_db,
                    "-c",
                    "DROP SCHEMA public CASCADE; CREATE SCHEMA public;",
                ]
            )

    def mkdir_p(self, directory):
        if not os.path.exists(directory):
            os.makedirs(directory)

    def compareImage(self, expectedImagePath, returnedImagePath, tolerance=imagecompare.EXACT):
        """Compare the pictures referred to by the arguments. On failure, a diff image is written next to the
        returned image.

        Args:
            expectedImagePath (str): path of the image that we expected
            returnedImagePath (str): path of the image that we got as output from the test
            tolerance (str): "exact" (default): all pixels identical. "geometry": each pixel may differ by a small
                margin, detects shifts of one pixel. "antialiasing": also accepts antialiasing differences at edges
                and in text, caused by other cairo, pixman or FreeType versions. Use the strictest tolerance that
                passes in all environments. See adaguc.imagecompare and doc/developing/antialiasing-differences.md.

        Returns:
            bool: True if the images match within the tolerance
        """

        if filecmp.cmp(returnedImagePath, expectedImagePath, shallow=False):
            diff_path = imagecompare.diff_image_path(returnedImagePath)
            if os.path.exists(diff_path):
                os.remove(diff_path)
            return True
        result = imagecompare.compare_image_files(expectedImagePath, returnedImagePath, tolerance)
        for message in result.messages:
            print(("" if result.match else "Error: ") + message, flush=True)
        return result.match

    def compareGetCapabilitiesXML(self, testresultFileLocation, expectedOutputFileLocation):
        expectedxml = self.readfromfile(expectedOutputFileLocation)
        testxml = self.readfromfile(testresultFileLocation)

        obj1 = objectify.fromstring(re.sub(b' xmlns="[^"]+"', b"", expectedxml, count=1))
        obj2 = objectify.fromstring(re.sub(b' xmlns="[^"]+"', b"", testxml, count=1))

        # Remove ADAGUC build date and version from keywordlists
        try:
            for child in obj1.findall("Service/KeywordList")[0]:
                child.getparent().remove(child)
            for child in obj2.findall("Service/KeywordList")[0]:
                child.getparent().remove(child)
        except Exception:
            pass
        try:
            for child in obj1.findall("Service/ServerInfo")[0]:
                child.getparent().remove(child)
            for child in obj2.findall("Service/ServerInfo")[0]:
                child.getparent().remove(child)
        except Exception:
            pass

        # Remove ContactInformation if it is empty element
        try:
            for child in obj1.findall("Service/ContactInformation")[0]:
                if child.text.strip() == "":
                    child.getparent().remove(child)
            for child in obj2.findall("Service/ContactInformation")[0]:
                if child.text.strip() == "":
                    child.getparent().remove(child)
        except Exception:
            pass

        # Boundingbox extent values are too varying by different Proj libraries, so
        # accept small differences instead of failing on them.
        def compareBBOX(root1, root2, threshold=100):
            if root1.tag.title() == "Boundingbox" and root2.tag.title() == "Boundingbox":
                for attrName in ("minx", "miny", "maxx", "maxy"):
                    try:
                        if abs(float(root1.attrib[attrName]) - float(root2.attrib[attrName])) <= threshold:
                            root2.attrib[attrName] = root1.attrib[attrName]
                    except Exception:
                        pass
            for elem1, elem2 in zip(root1.getchildren(), root2.getchildren()):
                compareBBOX(elem1, elem2)

        compareBBOX(obj1, obj2)

        # Remove contents of problem envelopes EPSG:28992 and EPSG:7399 because they are inconsistent
        def removeGmlEnvelope(root, epsg_code):
            xpath_query = f".//gml:Envelope[@srsName='EPSG:{epsg_code}']"
            envelopes = root.xpath(xpath_query, namespaces={"gml": "http://www.opengis.net/gml"})
            for envelope in envelopes:
                for child in envelope.getchildren():
                    envelope.remove(child)

        removeGmlEnvelope(obj1, "28992")
        removeGmlEnvelope(obj2, "28992")
        removeGmlEnvelope(obj1, "7399")
        removeGmlEnvelope(obj2, "7399")

        result = etree.tostring(obj1)
        expect = etree.tostring(obj2)

        if (result == expect) is False:
            print(
                '\nExpected XML is different, file \n"%s"\n should be equal to \n"%s"'
                % (testresultFileLocation, expectedOutputFileLocation)
            )

        return result == expect

    def compareFile(self, testresultFileLocation, expectedOutputFileLocation):
        a = AdagucTestTools().readfromfile(testresultFileLocation)
        b = AdagucTestTools().readfromfile(expectedOutputFileLocation)
        return a == b

    def getTimeDimension(self, xmlFileLocation, layerName):
        """Return (start, stop, interval) for a layer with the name provided"""
        xml_bytes = self.readfromfile(xmlFileLocation)
        try:
            root = etree.fromstring(xml_bytes)
        except Exception as e:
            print("[DEBUG] XML parsing failed:", e)
            print(xml_bytes[:500])  # print first 500 bytes for inspection
            raise

        ns = {"wms": "http://www.opengis.net/wms"}

        xpath = f".//wms:Layer[wms:Name='{layerName}']//wms:Dimension[@name='time']"
        print(f"[DEBUG] Running XPath: {xpath}")
        dim_node = root.find(xpath, namespaces=ns)

        if dim_node is None or not dim_node.text:
            raise ValueError(f"No <Dimension name='time'> found for layer '{layerName}'")

        parts = dim_node.text.strip().split("/")
        if len(parts) != 3:
            raise ValueError(f"Invalid time dimension format for '{layerName}': {dim_node.text}")

        return parts  # start, stop, and interval

    def compareTimeRange(self, xmlFileLocation, expected_days, expected_interval, layerName):
        """Check that time range and interval are a match"""
        start_str, stop_str, interval = self.getTimeDimension(xmlFileLocation, layerName)

        start = datetime.datetime.strptime(start_str, "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=datetime.timezone.utc)
        stop = datetime.datetime.strptime(stop_str, "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=datetime.timezone.utc)
        diff_days = (stop - start).total_seconds() / (86400.0 * 2)  # Offset appears twice, after and before now

        ok_days = abs(diff_days - expected_days) < 0.01
        ok_interval = interval == expected_interval

        if not ok_days or not ok_interval:
            print(f"\nTime dimension check failed for {xmlFileLocation}")
            print(f"  Found start: {start_str}, stop: {stop_str}, interval: {interval}")
            print(f"  Expected {expected_days} days and interval {expected_interval}")
        else:
            print(f"Time dimension OK: {diff_days:.2f} days, interval={interval}")

        return ok_days and ok_interval

    def compareJson(self, testresultFileLocation: str, expectedOutputFileLocation: str, significantDigits=6) -> bool:
        with open(expectedOutputFileLocation, "r") as fp:
            expected = json.load(fp)
        with open(testresultFileLocation, "r") as fp:
            result = json.load(fp)

        return DeepDiff(expected, result, significant_digits=significantDigits, number_format_notation="e") == {}
