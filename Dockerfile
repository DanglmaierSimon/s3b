FROM ubuntu:22.04

RUN apt update -y
RUN  DEBIAN_FRONTEND=noninteractive  &&  apt install -y cmake git build-essential curl ca-certificates sudo zlib1g-dev

RUN useradd --create-home --shell /bin/bash simon

USER simon
WORKDIR /home/simon/playground/s3b/

COPY --chown=simon . /home/simon/playground/s3b/

