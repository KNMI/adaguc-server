FROM ubuntu:18.04
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive \
    apt-get -y --no-install-recommends install \
        software-properties-common \
        apt-utils \
        wget \
    && add-apt-repository --yes ppa:met-norway/fimex \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive \
    apt-get -y install \
        fimex-bin \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

