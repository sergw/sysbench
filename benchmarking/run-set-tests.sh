#!/usr/bin/env bash

set -e

ARRAY_TESTS=(
    "oltp_read_only"
    "oltp_write_only"
    "oltp_read_write"
    "oltp_update_index"
    "oltp_update_non_index"
    "oltp_insert"
    "oltp_delete"
    "oltp_point_select"
    "select_random_points"
    "select_random_ranges"
#    "bulk_insert"
)

if [ -n "${TEST}" ]; then ARRAY_TESTS=("${TEST}"); fi


if [ ! -n "${TIME}" ]; then TIME=220; fi
if [ ! -n "${DBMS}" ]; then DBMS="tarantool"; fi
if [ ! -n "${THREADS}" ]; then THREADS=1; fi
if [ ! -n "${RESULT_FILE_NAME}" ]; then RESULT_FILE_NAME="result.txt"; fi

if [ -n "${USER}" ]; then USER=--${DBMS}-user=${USER}; fi
if [ -n "${PASSWORD}" ]; then PASSWORD=--${DBMS}-password=${PASSWORD}; fi

rm -f ${RESULT_FILE_NAME}
for test in "${ARRAY_TESTS[@]}"; do
    echo "------------" $test "------------"

    sysbench $test --db-driver=${DBMS} --threads=${THREADS} cleanup | tee $test".txt"
    sysbench $test --db-driver=${DBMS} --threads=${THREADS} prepare | tee -a $test".txt"
    sysbench $test --db-driver=${DBMS} --threads=${THREADS} --time=${TIME} --warmup-time=10 run | tee -a $test".txt"
    sysbench $test --db-driver=${DBMS} --threads=${THREADS} cleanup | tee -a $test".txt"

    echo -n $test":" | tee -a ${RESULT_FILE_NAME}
    cat $test".txt" | grep -e 'transactions:' | grep -oP '\(\K\S*' | tee -a ${RESULT_FILE_NAME}
done

echo "test_name:result[trps]"
cat ${RESULT_FILE_NAME}
