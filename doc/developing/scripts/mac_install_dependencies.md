# Instructions on how to install dependencies for Mac

[Back to Developing](../../Developing.md)

These instructions were tested with Apple M1, but should work for Intel Macs as well.

### C++ dependencies

We will use Homebrew to install Unix tools and packages, see: https://brew.sh/


```shell
brew update
brew install netcdf postgresql@14 udunits proj@7 gdal cmake
```

Note that Adaguc requires an older version of the PROJ library. When we install this with Homebrew,
it gets installed at a non-standard location.
CMake will only find this if you use the following export before running CMake:

```shell
export PROJ_ROOT=/opt/homebrew/opt/proj@7
./compile.sh
```

If you use CMake from IntelliJ/CLion, configure it to use the CMake that was installed with Homebrew,
as the bundled one might be too old and might not pick up the postgres libraries.
This can be set in the toolchain.

### Python dependencies

If you don't already have a working Python setup, a way to support multiple Python versions
is pyenv. In the description below we use it to set up a virtual environment for a specific Python version.
We then install the required packages, and run the tests.

```shell
# Install pyenv
brew install pyenv pyenv-virtualenv

# Install specific python version using pyenv
pyenv install 3.9.13

# Set up a virtualenv using the installed Python version, calling it adaguc.
pyenv virtualenv 3.9.13 adaguc

# Activate the crated virtualenv
pyenv activate adaguc

# Install the required Python packages
pip install flask flask-cors flask-caching gunicorn pytest marshmallow owslib pyproj==2.6.1 apispec apispec-webframeworks marshmallow-oneofschema defusedxml netcdf4

# Run the tests
./runtest.sh
```

Most of the above is one-time setup. Once the packages are installed, next time you can do
```shell
pyenv activate adaguc
./runtest.sh
```
