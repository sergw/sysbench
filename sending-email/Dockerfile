FROM ubuntu:16.04

RUN apt-get update

RUN DEBIAN_FRONTEND="noninteractive" apt-get install -y -f \
    postfix mailutils vim libsasl2-2 ca-certificates \
    libsasl2-modules