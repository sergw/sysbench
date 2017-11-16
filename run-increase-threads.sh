#!/usr/bin/env bash

i=1
max_threads=64

while [ $i -le $max_threads ];  do

    TIME=${TIME} DBMS=${DBMS} USER=${USER} PASSWORD=${PASSWORD} THREADS=${i} RESULT_FILE_NAME=${DBMS}_${i}.txt ./run-set-tests.sh

    let i=i*2
done
