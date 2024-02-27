# Instructions on how to install dependencies for Mac

[Back to Developing](../../Developing.md)

These instructions were tested with Apple M1, but should work for Intel Macs as well.

### C++ dependencies

We will use Homebrew to install Unix tools and packages, see: https://brew.sh/


```shell
brew update
brew install netcdf postgresql@14 udunits proj gdal cmake
```

Compile Adaguc using the provided script:

```shell
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
pyenv install 3.10.12

# Set up a virtualenv using the installed Python version, calling it adaguc.
pyenv virtualenv 3.10.12 adaguc

# Activate the crated virtualenv
pyenv activate adaguc

# Install the required Python packages
pip install -r requirements.txt
pip install -r requirements-dev.txt

# Run the tests
./runtest.sh
```

Most of the above is one-time setup. Once the packages are installed, next time you can do
```shell
pyenv activate adaguc
./runtest.sh
```
