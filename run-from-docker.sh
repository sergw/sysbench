#!/usr/bin/env bash

# Build Tarantool
cd /tarantool
git pull
if [ -n "${BRANCH}" ]; then git checkout ${BRANCH}; fi
branch_name="$(git symbolic-ref HEAD 2>/dev/null)"
branch_name=${branch_name##refs/heads/}
cmake . -DENABLE_DIST=ON; make; make install

# Build tarantool-c
cd /tarantool-c
git pull
cd third_party/msgpuck/; cmake . ; make; make install; cd ../..
cmake . ; make; make install

# Build tpcc
cd ${PATH_TO_SYSBENCH}
./autogen.sh; ./configure --with-tarantool --without-mysql; make; make install;

# Run Tarantool
cd ${PATH_TO_SYSBENCH}
tarantool sysbench-server.lua

STATUS="$(echo box.info.status | tarantoolctl connect /usr/local/var/run/tarantool/sysbench-server.control | grep -e "- running")"
while [ ${#STATUS} -eq "0" ]; do
    echo "waiting load snapshot to tarantool..."
    sleep 5
    STATUS="$(echo box.info.status | tarantoolctl connect /usr/local/var/run/tarantool/sysbench-server.control | grep -e "- running")"
done
cat /usr/local/var/log/tarantool/sysbench-server.log

# Run SysBench
ARRAY_TESTS=(
    "oltp_read_only"
    "oltp_point_select"
    "oltp_insert"
    "oltp_update_index"
    "oltp_update_non_index"
    "select_random_points"
    "select_random_ranges"
    "bulk_insert"
    "oltp_write_only"
    "oltp_read_write"
    "oltp_delete"
)

apt-get install -y -f gdb
rm -f result.txt

TAR_VER=$(tarantool -v | grep -e "Tarantool" |  grep -oP '\s\K\S*')
echo $TAR_VER"["$branch_name"]" | tee version.txt

if [ ! -n "${TIME}" ]; then TIME=220; fi

for test in "${ARRAY_TESTS[@]}"; do
    echo "------------" $test "------------"

    sysbench $test --db-driver=tarantool --threads=1 cleanup | tee $test".txt"
    sysbench $test --db-driver=tarantool --threads=1 prepare | tee -a $test".txt"
    sysbench $test --db-driver=tarantool --threads=1 --time=${TIME} --warmup-time=10 run | tee -a $test".txt"
    sysbench $test --db-driver=tarantool --threads=1 cleanup | tee -a $test".txt"

    echo -n $test":" | tee -a result.txt
    cat $test".txt" | grep -e 'transactions:' | grep -oP '\(\K\S*' | tee -a result.txt
done

echo "test_name:result[trps]"
cat result.txt

