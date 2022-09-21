# Setup a virtual env on ubuntu

[Back to Developing](../../Developing.md)

```
apt-get install python3-venv python3-pip
python3 -m pip install --user --upgrade setuptools wheel
# Setup a python3 virtual environment:
python3 -m venv env
source env/bin/activate
```