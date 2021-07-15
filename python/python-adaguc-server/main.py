import sys
import os
from flask import Flask
import logging

root = logging.getLogger()
root.setLevel(logging.DEBUG)
handler = logging.StreamHandler(sys.stdout)
handler.setLevel(logging.DEBUG)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
handler.setFormatter(formatter)
root.addHandler(handler)

app = Flask(__name__)

import routeAdagucServer
import routeAutoWMS

if __name__ == "__main__":
    app.secret_key = os.urandom(24)
    app.run(debug=True, port=9999)
