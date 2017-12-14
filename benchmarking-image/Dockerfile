# Version: 0.0.1
FROM ubuntu:16.04
MAINTAINER Ilya Petukhov <iproha94@tarantool.org>

RUN apt-get update && \
    apt-get install -y -f \
    gcc libc6-dev zlib1g-dev make libmysqlclient-dev \
    ssh vim git dh-autoreconf pkg-config libicu-dev \
    build-essential cmake coreutils sed libreadline-dev \
    libncurses5-dev libyaml-dev libssl-dev libcurl4-openssl-dev \
    libunwind-dev python python-pip python-setuptools python-dev \
    python-msgpack python-yaml python-argparse python-six python-gevent

RUN mkdir -p /usr/local/var/lib/tarantool/sysbench-server/ && \
    mkdir -p /usr/local/var/run/tarantool

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
