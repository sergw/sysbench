#!/usr/bin/env bash

cd ${PATH_TO_SYSBENCH}
tarantool sysbench-server.lua

STATUS="$(echo box.info.status | tarantoolctl connect /usr/local/var/run/tarantool/sysbench-server.control | grep -e "- running")"

while [ ${#STATUS} -eq "0" ]; do
    echo "waiting load snapshot to tarantool..."
    sleep 5
    STATUS="$(echo box.info.status | tarantoolctl connect /usr/local/var/run/tarantool/sysbench-server.control | grep -e "- running")"
done
cat /usr/local/var/log/tarantool/sysbench-server.log
