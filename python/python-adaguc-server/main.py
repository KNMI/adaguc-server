from werkzeug.serving import WSGIRequestHandler
from configureLogging import configureLogging
from setupAdaguc import setupAdaguc
from routeRoot import routeRoot
from routeHealthCheck import routeHealthCheck
from routeAdagucOpenDAP import routeAdagucOpenDAP
from routeAutoWMS import routeAutoWMS
from routeAdagucServer import routeAdagucServer
# from routeOGCApiFeatures import routeOGCApiFeatures
from routeOGCApi import routeOGCApi, init_views
import sys
import os
from flask import Flask
import logging

configureLogging(logging)
logger = logging.getLogger(__name__)

app = Flask(__name__)

app.register_blueprint(routeAdagucServer)
app.register_blueprint(routeAutoWMS)
app.register_blueprint(routeAdagucOpenDAP)
app.register_blueprint(routeRoot)
app.register_blueprint(routeHealthCheck)
# app.register_blueprint(routeOGCApiFeatures, url_prefix="/ogcapi-f")
app.register_blueprint(routeOGCApi, url_prefix="/ogcapi")
with app.app_context():
  init_views()

WSGIRequestHandler.protocol_version = "HTTP/1.1"

def testadaguc():
  logger.info("Checking adaguc-server.")
  adagucInstance = setupAdaguc()
  url = "SERVICE=WMS&REQUEST=GETCAPABILITIES"
  adagucenv = {}

  """ Set required environment variables """
  baseUrl = "---"
  adagucenv['ADAGUC_ONLINERESOURCE'] = os.getenv(
      'EXTERNALADDRESS', baseUrl) + "/adaguc-server?"
  adagucenv['ADAGUC_DB'] = os.getenv(
      'ADAGUC_DB', "user=adaguc password=adaguc host=localhost dbname=adaguc")

  """ Run adaguc-server """
  status, data, headers = adagucInstance.runADAGUCServer(
      url, env=adagucenv,  showLogOnError=False)
  assert status == 0
  assert headers == ['Content-Type:text/xml']
  logger.info("adaguc-server seems [OK]")


if __name__ == "__main__":
  app.secret_key = os.urandom(24)
  testadaguc()
  app.run(debug=True, host="0.0.0.0", port=8087)
