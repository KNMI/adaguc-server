FROM python:3.8-slim-bullseye as base

USER root

LABEL maintainer="adaguc@knmi.nl"

# Version should be same as in Definitions.h
LABEL version="2.8.0"

######### First stage (build) ############

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
    libsqlite3-dev \ 
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install adaguc-server from context
COPY . /adaguc/adaguc-server-master

WORKDIR /adaguc/adaguc-server-master

RUN bash compile.sh


# ######### Second stage (production) ############
FROM python:3.8-slim-bullseye

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
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /adaguc/adaguc-server-master

# Install compiled adaguc binaries from stage one
COPY --from=0 /adaguc/adaguc-server-master/bin /adaguc/adaguc-server-master/bin
COPY --from=0 /adaguc/adaguc-server-master/data /adaguc/adaguc-server-master/data
COPY --from=0 /adaguc/adaguc-server-master/python /adaguc/adaguc-server-master/python
COPY --from=0 /adaguc/adaguc-server-master/tests /adaguc/adaguc-server-master/tests
COPY --from=0 /adaguc/adaguc-server-master/runtests.sh /adaguc/adaguc-server-master/runtests.sh
COPY --from=0 /adaguc/adaguc-server-master/requirements.txt /adaguc/adaguc-server-master/requirements.txt

# Upgrade pip and install python requirements.txt
RUN pip3 install --no-cache-dir --upgrade pip \
   && pip3 install --no-cache-dir -r requirements.txt


# Run adaguc-server functional and regression tests

RUN bash runtests.sh

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
RUN  chmod +x /adaguc/adaguc-server-*.sh && \
    chmod +x /adaguc/start.sh && \
    chown -R adaguc:adaguc /data/adaguc* /adaguc /adaguc/*

ENV ADAGUC_PATH=/adaguc/adaguc-server-master

ENV PYTHONPATH=${ADAGUC_PATH}/python/python-adaguc-server

# Build and test adaguc python support
WORKDIR /adaguc/adaguc-server-master/python/lib/
RUN python3 setup.py install
RUN bash -c "python3 /adaguc/adaguc-server-master/python/examples/runautowms/run.py && ls result.png"
WORKDIR /adaguc/adaguc-server-master

USER adaguc

# For HTTP
EXPOSE 8080


ENTRYPOINT ["sh", "/adaguc/start.sh"]
