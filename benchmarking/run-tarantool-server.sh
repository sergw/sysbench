#!/usr/bin/env bash

tarantool sysbench-server.lua

STATUS="$(echo box.info.status | tarantoolctl connect /tmp/sysbench-server.sock | grep -e "- running")"

while [ ${#STATUS} -eq "0" ]; do
    echo "waiting load snapshot to tarantool..."
    sleep 5
    STATUS="$(echo box.info.status | tarantoolctl connect /tmp/sysbench-server.sock | grep -e "- running")"
done
cat ./sysbench-server.log
