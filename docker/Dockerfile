# needed because of old glibc
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y \
        build-essential \
        ninja-build \
        git \
        zip \
        unzip \
        zlib1g-dev \
        ca-certificates \
        curl && \
    rm -rf /var/lib/apt/lists/* && \
    update-ca-certificates

ENV CMAKE_VERSION=3.31.1

RUN curl -fsSLO --compressed https://cmake.org/files/v3.31/cmake-3.31.1-linux-x86_64.tar.gz --output cmake-3.31.1-linux-x86_64.tar.gz && \
        tar xzf cmake-3.31.1-linux-x86_64.tar.gz -C /usr/local --strip-components=1 --no-same-owner && \
        rm -rfv cmake-3.31.1-linux-x86_64.tar.gz

WORKDIR /workspace

CMD ["/bin/bash"]