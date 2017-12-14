#!/usr/bin/env bash

set -e

if [ ! -n "${PATH_TO_SYSBENCH}" ]; then PATH_TO_SYSBENCH=$(pwd); fi

# build Tarantool
if [ ! -n "${BRANCH}" ]; then BRANCH="1.8"; fi
cd /opt/tarantool
git checkout ${BRANCH}
cmake . -DENABLE_DIST=ON; make; make install

# define tarantool version
cd ${PATH_TO_SYSBENCH}
TAR_VER=$(tarantool -v | grep -e "Tarantool" |  grep -oP '\s\K\S*')
echo ${TAR_VER} | tee version.txt

# run tarantool
cd ${PATH_TO_SYSBENCH}
./run-tarantool-server.sh

# build sysbench
cd ${PATH_TO_SYSBENCH}
./autogen.sh; ./configure --with-tarantool --without-mysql; make; make install;

# run sysbench
apt-get install -y -f gdb
./run-set-tests.sh
