#!/usr/bin/env bash

if [ ! -n "${EMAIL_LOGIN}" ]; then exit 0; fi
if [ ! -n "${EMAIL_PASSWORD}" ]; then exit 0; fi

if [ -f "benchmarking/commit-author.txt" ]; then
    EMAIL_RCPT=`cat benchmarking/commit-author.txt`
else
    EMAIL_RCPT="${EMAIL_DEFAULT}"
fi

if [ ! -n "${BRANCH}" ]; then BRANCH="1.8"; fi

if [ -n "${SUCCESS_BENCHMARK}" ]; then
    VERSION=`cat benchmarking/version.txt`
    echo "Subject: [benchmarks] Success $VERSION $BRANCH" > letter.txt
    echo "Hello" >> letter.txt
    cat ./benchmarking/result.txt >> letter.txt
    echo "all results:" >> letter.txt
    echo "http://bench.tarantool.org/?tab=tab-sysbench" >> letter.txt
else
    echo "Subject: [benchmarks] Fail $BRANCH" > letter.txt
    echo "Hello" >> letter.txt
    echo "Fail benchmark. See" >> letter.txt
    echo "https://gitlab.com/tarantool/sysbench/pipelines" >> letter.txt
fi

curl --connect-timeout 15 -v \
    --insecure "smtp://smtp.mail.ru:2525" \
    -u "${EMAIL_LOGIN}:${EMAIL_PASSWORD}" \
    --mail-from "${EMAIL_LOGIN}" \
    --mail-rcpt "${EMAIL_RCPT}" \
    -T letter.txt --ssl --ipv4

