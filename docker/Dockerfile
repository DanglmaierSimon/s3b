# needed because of old glibc
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y \
        build-essential \
        cmake \
        ninja-build \
        git \
        zip \
        unzip \
        zlib1g-dev  && \
    rm -rf /var/lib/apt/lists/*

RUN useradd -ms /bin/bash botbuilder

USER botbuilder

WORKDIR /workspace

CMD ["/bin/bash"]