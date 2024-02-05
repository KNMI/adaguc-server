#!/usr/bin/env python3
import logging
import os

import setuptools

logging.basicConfig()
logger = logging.getLogger(__name__)
logger.setLevel(os.environ.get("LOG_LEVEL", "INFO"))

# Package meta-data.
NAME = "edr-pydantic-classes"

env_suffix = os.environ.get("ENVIRONMENT_SUFFIX", "")
logger.debug(f"Environment suffix: {env_suffix}")

if env_suffix:
    NAME += f"-{env_suffix}"
logger.debug(f"Package name: {NAME}")

setuptools.setup(
    name=NAME,
    version="0.0.9",
    description="The Pydantic models for EDR datatypes",
    package_data={"edr_pydantic_classes": ["py.typed"]},
    packages=["edr_pydantic_classes"],
    include_package_data=False,
    license="MIT",
    install_requires=["pydantic", "orjson", "covjson-pydantic"],
    python_requires=">=3.8.0",
)
