#!/usr/bin/env bash

if [ ! -n "${EMAIL_LOGIN}" ]; then exit 1; fi
if [ ! -n "${EMAIL_PASSWORD}" ]; then exit 1; fi
if [ ! -n "${LAUNCH_TYPE}" ]; then exit 1; fi

if [ "${LAUNCH_TYPE}" == "MANUAL" ]; then
    if [ ! -f "benchmarking/commit-author.txt" ]; then exit 1; fi
    EMAIL_RCPT=`cat benchmarking/commit-author.txt`
elif [ "${LAUNCH_TYPE}" == "AUTO" ]; then
    if [ ! -n "${EMAIL_DEFAULT}" ]; then exit 1; fi
    EMAIL_RCPT="${EMAIL_DEFAULT}"
else
    exit 1;
fi

if [ ! -n "${BRANCH}" ]; then BRANCH="1.8"; fi
VERSION=`cat benchmarking/version.txt`

if [ -n "${SUCCESS_BENCHMARK}" ]; then
    echo "Subject: [benchmarks] Success $VERSION $BRANCH" > letter.txt
    echo "Hello" >> letter.txt
    cat ./benchmarking/result.txt >> letter.txt
    echo "all results:" >> letter.txt
    echo "http://bench.tarantool.org/?tab=tab-sysbench" >> letter.txt
else
    echo "Subject: [benchmarks] Fail $VERSION $BRANCH" > letter.txt
    echo "Hello" >> letter.txt
    echo "Fail benchmark." >> letter.txt

    if [ -f "benchmarking/last-test.txt" ]; then
        echo "----------" >> letter.txt
        echo "Fail test:" >> letter.txt
        cat ./benchmarking/last-test.txt >> letter.txt
    fi

    if [ -f "benchmarking/sysbench-server.log" ]; then
        echo "----------" >> letter.txt
        echo "Tarantool log:" >> letter.txt
        cat ./benchmarking/sysbench-server.log >> letter.txt
    fi
fi

curl --connect-timeout 15 -v \
    --insecure "smtp://smtp.mail.ru:2525" \
    -u "${EMAIL_LOGIN}:${EMAIL_PASSWORD}" \
    --mail-from "${EMAIL_LOGIN}" \
    --mail-rcpt "${EMAIL_RCPT}" \
    -T letter.txt --ssl --ipv4

