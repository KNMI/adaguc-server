import sys
import os
from flask import Flask
import logging
from configureLogging import configureLogging
configureLogging(logging)
from routeAdagucServer import routeAdagucServer
from routeAutoWMS import routeAutoWMS

app = Flask(__name__)
app.register_blueprint(routeAdagucServer)
app.register_blueprint(routeAutoWMS)

if __name__ == "__main__":
    app.secret_key = os.urandom(24)
    app.run(debug=True, host="0.0.0.0", port=8080)
