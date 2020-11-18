import setuptools

with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name="adaguc",
    version="0.0.1",
    author="KNMI Geospatial Task Force",
    author_email="adaguc@knmi.nl",
    description="Python wrapper arround the adaguc-server",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/KNMI/adaguc-server",
    packages=setuptools.find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: Apache2 License",
        "Operating System :: Linux",
    ],
    python_requires='>=3.6',
)