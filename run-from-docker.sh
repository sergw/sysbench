#!/usr/bin/env bash

if [ ! -n "${PATH_TO_SYSBENCH}" ]; then PATH_TO_SYSBENCH=$(pwd); fi

# Build Tarantool
cd /tarantool
git pull
if [ ! -n "${BRANCH}" ]; then BRANCH="1.8"; fi
git checkout ${BRANCH};
cmake . -DENABLE_DIST=ON; make; make install

# define tarantool version
cd ${PATH_TO_SYSBENCH}
TAR_VER=$(tarantool -v | grep -e "Tarantool" |  grep -oP '\s\K\S*')
echo ${TAR_VER} | tee version.txt

# Build tarantool-c
cd /tarantool-c
git pull
cd third_party/msgpuck/; cmake . ; make; make install; cd ../..
cmake . ; make; make install

# Build sysbench
cd ${PATH_TO_SYSBENCH}
./autogen.sh; ./configure --with-tarantool --without-mysql; make; make install;

# Run Tarantool
PATH_TO_SYSBENCH=${PATH_TO_SYSBENCH} ./run-tarantool-server.sh

# Run SysBench
apt-get install -y -f gdb
TIME=${TIME} ./run-set-tests.sh

