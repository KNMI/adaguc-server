FROM centos/devtoolset-7-toolchain-centos7:7
USER root

LABEL maintainer="adaguc@knmi.nl"

# Version should be same as in Definitions.h
LABEL version="2.6.0" 

######### First stage (build) ############

# production packages, same as stage two
RUN yum update -y && \
    yum install -y epel-release deltarpm && \
    yum install -y cairo \
    curl \
    python3 \
    gd \
    gdal \
    hdf5 \
    libxml2 \
    proj \
    udunits2 \
    openssl \
    netcdf \
    libwebp-devel \
    # building / development packages
    yum install -y centos-release-scl && \
    yum install -y devtoolset-7-gcc-c++ && \
    source /opt/rh/devtoolset-7/enable && \
    yum install -y cmake3 cairo-devel \
    gd-devel \
    gdal-devel \
    hdf5-devel \
    libxml2-devel \
    make \
    netcdf-devel \
    openssl \
    postgresql-devel \
    proj-devel \
    sqlite-devel \
    udunits2-devel && \
    yum clean all && \
    rm -rf /var/cache/yum

# Install adaguc-server from context
COPY . /adaguc/adaguc-server-master

WORKDIR /adaguc/adaguc-server-master

# Force to use Python 3
RUN ln -sf /usr/bin/python3 /usr/bin/python
RUN ln -sf /usr/bin/cmake3 /usr/bin/cmake
RUN ln -sf /usr/bin/ctest3 /usr/bin/ctest
RUN cp -r /usr/include/udunits2/* /usr/include/

RUN bash compile.sh


######### Second stage (production) ############
FROM centos:7
USER root

# production packages, same as stage one
RUN yum update -y && \
    yum install -y epel-release deltarpm && \
    yum install -y cairo \
    curl \
    gd \
    gdal \
    hdf5 \
    libxml2 \
    proj \
    python3 \
    python3-lxml \
    postgresql \
    udunits2 \
    openssl \
    netcdf \
    libwebp \
    python36-numpy \
    python36-netcdf4 \
    python36-six \
    python36-requests \
    python36-pillow \
    python36-lxml && \
    yum clean all && \
    rm -rf /var/cache/yum

RUN pip3 install flask flask-cors gunicorn

WORKDIR /adaguc/adaguc-server-master

# Install compiled adaguc binaries from stage one    
COPY --from=0 /adaguc/adaguc-server-master/bin /adaguc/adaguc-server-master/bin
COPY --from=0 /adaguc/adaguc-server-master/data /adaguc/adaguc-server-master/data
COPY --from=0 /adaguc/adaguc-server-master/python /adaguc/adaguc-server-master/python
COPY --from=0 /adaguc/adaguc-server-master/tests /adaguc/adaguc-server-master/tests
COPY --from=0 /adaguc/adaguc-server-master/runtests.sh /adaguc/adaguc-server-master/runtests.sh

# Run adaguc-server functional and regression tests
RUN  bash runtests.sh 

    # Set same uid as vivid
RUN useradd -m adaguc -u 1000 && \
    # Setup directories
    mkdir -p /data/adaguc-autowms && \
    mkdir -p /data/adaguc-datasets && \
    mkdir -p /data/adaguc-data && \
    mkdir -p /adaguc/userworkspace && \
    mkdir -p /adaguc/adaguc-datasets-internal

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
