#!/bin/sh

# download, build, install tarantool
git clone --recursive https://github.com/tarantool/tarantool.git /opt/tarantool
cd /opt/tarantool; cmake . -DENABLE_DIST=ON; make; make install;

# download, build, install tarantool-c
git clone --recursive https://github.com/tarantool/tarantool-c.git /opt/tarantool-c
cd /opt/tarantool-c/third_party/msgpuck/; cmake . ; make; make install;
cd /opt/tarantool-c; cmake . ; make; make install;

# this will exec the CMD from your Dockerfile
exec "$@"

