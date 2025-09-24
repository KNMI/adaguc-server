import asyncio
import os
from io import BytesIO
import shutil
from .CGIRunner import CGIRunner
import re
from lxml import etree, objectify
import urllib.request
from PIL import Image
import subprocess

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
        url=None,
        env=None,
        path=None,
        args=None,
        isCGI=True,
        showLogOnError=True,
        showLog=False,
        maxLogFileSize=8192,
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

        filetogenerate = BytesIO()
        status, headers, processErr = asyncio.run(
            CGIRunner().run(
                adagucargs,
                url=url,
                output=filetogenerate,
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
            print("Adaguc-server has non zero exit status %d " % status)
            if isCGI == False:
                print(filetogenerate.getvalue().decode())
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
            # The executable wrote to stderr, which is unwanted behaviour. Stderr should be empty when running adaguc-server.
            if processErr:
                # print(processErr.decode())
                print("[WARNING]: the process should not write to stderr")
            # Check if the code did not write a too big logfile
            logfileSize = self.getLogFileSize()
            if logfileSize > maxLogFileSize:
                self.printLogFile()
                print(
                    "[WARNING]: Adaguc-server writes too many lines to the logfile, size = %d kilobytes"
                    % (logfileSize / 1024)
                )
            return status, filetogenerate, headers

    def writetofile(self, filename, data):
        chars = set(":+,@#$%^&*()\\")
        if any((c in chars) for c in filename):
            raise Exception("Filename %s contains invalid characters" % filename)

        # path = os.getenv("ADAGUC_PATH", "") + "/tests/"
        # if not os.path.exists(path):
        #     os.makedirs(path)

        # with open(os.getenv("ADAGUC_PATH", "") + "/tests/" + filename, "wb") as f:
        #     f.write(data)

        with open(filename, "wb") as f:
            f.write(data)

    def readfromfile(self, filename):
        adagucpath = os.getenv("ADAGUC_PATH", "") + "/tests/"
        if adagucpath:
            # filename = filename.replace("{ADAGUC_PATH}/", adagucpath)
            filename = adagucpath + filename
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

        directory = ADAGUC_PATH + "/tests/" + directory
        if not os.path.exists(directory):
            os.makedirs(directory)

    def compareImage(
        self,
        expectedImagePath,
        returnedImagePath,
        maxAllowedColorDifference=1,
        maxAllowedColorPercentage=0.01,
    ):
        """Compare the pictures referred to by the arguments.

        Args:
            expectedImagePath (str): path of the image that we expected
            returnedImagePath (str): path of the image that we got as output from the test
            maxAllowedColorDifference(int): max allowed difference in a single color band
            maxAllowedColorPercentage(float): max allowed percentage of pixels that have different colors

        Returns:
            bool: True if the difference is "small enough"
        """
        expected_image = Image.open(expectedImagePath)
        returned_image = Image.open(returnedImagePath)
        if expected_image.size != returned_image.size:
            print(
                f"\nError, size of expected and actual image do not match: {expected_image.size} vs {returned_image.size}"
            )
            return False

        if expected_image.mode != returned_image.mode:
            print(
                f"\nError, mode of expected and actual image do not match: {expected_image.mode} vs {returned_image.mode}"
            )
            return False

        width, height = expected_image.size
        if returned_image.mode == "P":
            expected_palette_values = expected_image.getpalette()
            returned_palette_values = returned_image.getpalette()
            if returned_image.palette.mode == "RGB":
                print(f"\tApplying palette translation for comparison...")
                expected_palette = [
                    tuple(expected_palette_values[i : i + 3])
                    for i in range(0, len(expected_palette_values), 3)
                ]
                returned_palette = [
                    tuple(returned_palette_values[i : i + 3])
                    for i in range(0, len(returned_palette_values), 3)
                ]
            else:
                print(
                    f"\nError: Palette mode {returned_image.palette.mode} not supported!"
                )
                return False

        n_bands = len(expected_image.getbands())

        max_color_difference_pixel = None
        max_color_difference_value = -1
        sum_color_difference = 0
        count_pixels_with_color_difference = 0
        for x in range(0, width):
            for y in range(0, height):
                c = (x, y)
                expected_color = expected_image.getpixel(c)
                returned_color = returned_image.getpixel(c)

                if n_bands == 1:
                    if returned_image.mode == "P":
                        expected_color = expected_palette[expected_color]
                        returned_color = returned_palette[returned_color]
                    else:
                        expected_color = (expected_color,)
                        returned_color = (returned_color,)
                diff_color = tuple(
                    map(lambda e, g: e - g, expected_color, returned_color)
                )

                if expected_color != returned_color:
                    print(
                        f"Warning: pixel {c} has different color, (expected, actual, diff) = "
                        f"{expected_color} \t{returned_color} \t{diff_color}",
                        flush=True,
                    )
                    count_pixels_with_color_difference += 1
                    abs_color_diff = tuple(abs(d) for d in diff_color)
                    if max(abs_color_diff) > max_color_difference_value:
                        max_color_difference_value = max(abs_color_diff)
                        max_color_difference_pixel = c
                    sum_color_difference += sum(abs_color_diff)

        if count_pixels_with_color_difference > 0:
            print(
                f"Warning: Max color difference {max_color_difference_value} in pixel {max_color_difference_pixel}. "
                f"Sum of absolute color difference: {sum_color_difference}. "
                f"Number of pixels with color difference: {count_pixels_with_color_difference}. "
                f"Percentage of pixel with color difference: {count_pixels_with_color_difference*100.0/(width*height):.6f} %",
                flush=True,
            )

        if max_color_difference_value > maxAllowedColorDifference:
            print(
                f"Error, difference for pixel {max_color_difference_pixel} is too large ({max_color_difference_value} > {maxAllowedColorDifference})",
                flush=True,
            )
            return False

        if (
            count_pixels_with_color_difference * 100.0 / (width * height)
            > maxAllowedColorPercentage
        ):
            print(
                f"Error, percentage of pixels with color difference is too large "
                f"({count_pixels_with_color_difference*100.0/(width*height)} % > {maxAllowedColorPercentage} %)",
                flush=True,
            )
            return False

        return True

    def compareGetCapabilitiesXML(
        self, testresultFileLocation, expectedOutputFileLocation
    ):
        expectedxml = self.readfromfile(expectedOutputFileLocation)
        testxml = self.readfromfile(testresultFileLocation)

        obj1 = objectify.fromstring(
            re.sub(b' xmlns="[^"]+"', b"", expectedxml, count=1)
        )
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

        # Boundingbox extent values are too varying by different Proj libraries
        def removeBBOX(root):
            if root.tag.title() == "Boundingbox":
                # root.getparent().remove(root)
                try:
                    del root.attrib["minx"]
                    del root.attrib["miny"]
                    del root.attrib["maxx"]
                    del root.attrib["maxy"]
                except Exception:
                    pass
            for elem in root.getchildren():
                removeBBOX(elem)

        removeBBOX(obj1)
        removeBBOX(obj2)

        # Remove contents of problem envelopes EPSG:28992 and EPSG:7399 because they are inconsistent
        def removeGmlEnvelope(root, epsg_code):
            xpath_query = f".//gml:Envelope[@srsName='EPSG:{epsg_code}']"
            envelopes = root.xpath(
                xpath_query, namespaces={"gml": "http://www.opengis.net/gml"}
            )
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
