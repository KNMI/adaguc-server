######### First stage (build) ############
FROM python:3.10-slim-bookworm AS build

USER root

LABEL maintainer="adaguc@knmi.nl"

# Version should be same as in Definitions.h
LABEL version="6.2.0"

# Try to update image packages
RUN apt-get -q -y update \
    && DEBIAN_FRONTEND=noninteractive apt-get -q -y upgrade \
    && apt-get -q -y install \
    cmake \
    postgresql \
    libcurl4-openssl-dev \
    libcairo2-dev \
    libxml2-dev \
    libgd-dev \
    postgresql-server-dev-all \
    postgresql-client \
    libudunits2-dev \
    udunits-bin \
    g++ \
    m4 \
    netcdf-bin \
    libnetcdf-dev \
    libhdf5-dev \
    libproj-dev \
    libgdal-dev \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install adaguc-server files need for compilation from context
COPY adagucserverEC /adaguc/adaguc-server-master/adagucserverEC
COPY CCDFDataModel /adaguc/adaguc-server-master/CCDFDataModel
COPY hclasses /adaguc/adaguc-server-master/hclasses
COPY cmake /adaguc/adaguc-server-master/cmake
COPY CMakeLists.txt /adaguc/adaguc-server-master/
COPY compile.sh /adaguc/adaguc-server-master/

WORKDIR /adaguc/adaguc-server-master

RUN bash compile.sh


######### Second stage, base image for test and prod ############
FROM python:3.10-slim-bookworm AS base

USER root


# Try to update image packages
RUN apt-get -q -y update \
    && DEBIAN_FRONTEND=noninteractive apt-get -q -y upgrade \
    && apt-get -q -y install \
    postgresql-client \
    udunits-bin \
    netcdf-bin \
    libcairo2 \
    libgdal-dev \
    libcurl4-openssl-dev \
    libgd-dev \
    libproj-dev \
    time \
    supervisor \
    pgbouncer \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /adaguc/adaguc-server-master

# Upgrade pip and install python requirements.txt
COPY requirements.txt /adaguc/adaguc-server-master/requirements.txt
RUN pip3 install --no-cache-dir --upgrade pip pip-tools setuptools \
    && pip install --no-cache-dir -r requirements.txt

# Install compiled adaguc binaries from stage one
COPY --from=build /adaguc/adaguc-server-master/bin /adaguc/adaguc-server-master/bin
COPY data /adaguc/adaguc-server-master/data
COPY python /adaguc/adaguc-server-master/python

######### Third stage, test ############
FROM base AS test

COPY requirements-dev.txt /adaguc/adaguc-server-master/requirements-dev.txt
RUN pip install --no-cache-dir -r requirements-dev.txt

COPY tests /adaguc/adaguc-server-master/tests
COPY runtests_psql.sh /adaguc/adaguc-server-master/runtests_psql.sh

# Determine if we're building in github actions or via a local docker build
ARG TEST_IN_CONTAINER
ENV TEST_IN_CONTAINER=${TEST_IN_CONTAINER:-local_build}

# Run adaguc-server functional and regression tests. See also `./doc/developing/testing.md`

RUN cd ./python/lib/ && python3 setup.py develop
RUN bash runtests_psql.sh

# Create a file indicating that the test succeeded. This file is used in the final stage
RUN echo "TESTSDONE" >  /adaguc/adaguc-server-master/testsdone.txt

######### Fourth stage, prod ############
FROM base AS prod

# Set same uid as vivid
RUN useradd -m adaguc -u 1000 && \
    # Setup directories
    mkdir -p /data/adaguc-autowms && \
    mkdir -p /data/adaguc-datasets && \
    mkdir -p /data/adaguc-data && \
    mkdir -p /adaguc/userworkspace && \
    mkdir -p /adaguc/adaguc-datasets-internal && \
    chown adaguc: /tmp -R


# Configure
COPY ./Docker/adaguc-server-config-python-postgres.xml /adaguc/adaguc-server-config.xml
COPY ./Docker/start.sh /adaguc/
COPY ./Docker/adaguc-server-*.sh /adaguc/
COPY ./Docker/baselayers.xml /adaguc/adaguc-datasets-internal/baselayers.xml
COPY scripts /adaguc/adaguc-server-master/scripts
COPY ./scripts/*.sh /adaguc/
# Copy pgbouncer and supervisord config files
COPY ./Docker/pgbouncer/ /adaguc/pgbouncer/
COPY ./Docker/supervisord/ /etc/supervisor/conf.d/
COPY ./Docker/run_supervisord.sh /adaguc/run_supervisord.sh
COPY ./Docker/start_autosync.sh /adaguc/start_autosync.sh

# Set permissions
RUN  chmod +x /adaguc/*.sh && \
    chmod +x /adaguc/adaguc-server-master/scripts/*.sh && \
    chown -R adaguc:adaguc /data/adaguc* /adaguc /adaguc/*

ENV ADAGUC_PATH=/adaguc/adaguc-server-master

ENV PYTHONPATH=${ADAGUC_PATH}/python/python_fastapi_server

# Build and test adaguc python support
WORKDIR /adaguc/adaguc-server-master/python/lib/
RUN python3 setup.py install
RUN bash -c "rm -f result.png && if [[ "${TEST_IN_CONTAINER}" == "github_build" ]]; then     db_host="localhost"; elif [[ "${TEST_IN_CONTAINER}" == "local_build" ]]; then     db_host="host.docker.internal"; else     db_host="localhost"; fi && export ADAGUC_DB="user=adaguc password=adaguc host=${db_host} dbname=postgres port=54321" && python3 /adaguc/adaguc-server-master/python/examples/runautowms/run.py && ls -lrtha result.png"

WORKDIR /adaguc/adaguc-server-master

# This checks if the test stage has ran without issues.
COPY --from=test /adaguc/adaguc-server-master/testsdone.txt /adaguc/adaguc-server-master/testsdone.txt

USER adaguc

# For HTTP
EXPOSE 8080

ENTRYPOINT ["bash", "/adaguc/run_supervisord.sh"]
