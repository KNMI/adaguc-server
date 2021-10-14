import sys
import os
from flask import Flask
import logging
from configureLogging import configureLogging
configureLogging(logging)
from routeAdagucServer import routeAdagucServer
from routeAutoWMS import routeAutoWMS
from routeAdagucOpenDAP import routeAdagucOpenDAP
from routeHealthCheck import routeHealthCheck
from routeRoot import routeRoot

app = Flask(__name__)
app.register_blueprint(routeAdagucServer)
app.register_blueprint(routeAutoWMS)
app.register_blueprint(routeAdagucOpenDAP)
app.register_blueprint(routeRoot)
app.register_blueprint(routeHealthCheck)

from setupAdaguc import setupAdaguc

def testadaguc():
    print("Checking adaguc-server.")
    adagucInstance = setupAdaguc()
    url = "SERVICE=WMS&REQUEST=GETCAPABILITIES"
    adagucenv={}

    """ Set required environment variables """
    baseUrl = "---"
    adagucenv['ADAGUC_ONLINERESOURCE']=os.getenv('EXTERNALADDRESS', baseUrl) + "/adaguc-server?"
    adagucenv['ADAGUC_DB']=os.getenv('ADAGUC_DB', "user=adaguc password=adaguc host=localhost dbname=adaguc")
    
    """ Run adaguc-server """
    status,data,headers = adagucInstance.runADAGUCServer(url, env = adagucenv,  showLogOnError = False)
    assert status == 0
    assert headers == ['Content-Type:text/xml']
    print("[OK]")


if __name__ == "__main__":
    app.secret_key = os.urandom(24)
    testadaguc()
    app.run(debug=True, host="0.0.0.0", port=8080)
