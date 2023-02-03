"""Entry for the adaguc-server pythonwrapper"""
import os
import logging
from werkzeug.serving import WSGIRequestHandler
from configureLogging import configureLogging
from setup_adaguc import setup_adaguc
from routeRoot import routeRoot
from routeHealthCheck import routeHealthCheck
from routeAdagucOpenDAP import routeAdagucOpenDAP
from routeAutoWMS import routeAutoWMS
from routeAdagucServer import routeAdagucServer
from ogcapi.routeOGCApi import routeOGCApi, init_views
from cacher import cacher, init_cache

from flask import Flask

configureLogging(logging)
logger = logging.getLogger(__name__)

def create_app():
    """Create the Flask/Gunicorn appliicaiton"""
    _app = Flask(__name__)

    init_cache(_app)

    _app.register_blueprint(routeAdagucServer)
    _app.register_blueprint(routeAutoWMS)
    _app.register_blueprint(routeAdagucOpenDAP)
    _app.register_blueprint(routeRoot)
    _app.register_blueprint(routeHealthCheck)
    _app.register_blueprint(routeOGCApi, url_prefix="/ogcapi")
    with _app.app_context():
        init_views()
    return _app


app = create_app()
app.config.update({'EXPLAIN_TEMPLATE_LOADING': True})

logger.info("APP:%s", app.url_map)

WSGIRequestHandler.protocol_version = "HTTP/1.1"


def testadaguc():
    """Test adaguc is setup correctly"""
    logger.info("Checking adaguc-server.")
    adaguc_instance = setup_adaguc()
    url = "SERVICE=WMS&REQUEST=GETCAPABILITIES"
    adagucenv = {}

    #  Set required environment variables
    baseurl = "---"
    adagucenv['ADAGUC_ONLINERESOURCE'] = os.getenv(
        'EXTERNALADDRESS', baseurl) + "/adaguc-server?"
    adagucenv['ADAGUC_DB'] = os.getenv(
        'ADAGUC_DB', "user=adaguc password=adaguc host=localhost dbname=adaguc")

    # Run adaguc-server
    # pylint: disable=unused-variable
    status, data, headers = adaguc_instance.runADAGUCServer(
        url, env=adagucenv,  showLogOnError=False)
    assert status == 0
    assert headers == ['Content-Type:text/xml']
    logger.info("adaguc-server seems [OK]")


if __name__ == "__main__":
    app.secret_key = os.urandom(24)
    testadaguc()
    app.run(debug=True, host="0.0.0.0", port=8081)
