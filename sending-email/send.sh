#!/usr/bin/env bash

if [ ! -n "${EMAIL_LOGIN}" ]; then exit 0; fi
if [ ! -n "${EMAIL_PASSWORD}" ]; then exit 0; fi
if [ ! -f "benchmarking/commit-author.txt" ]; then exit 0; fi

EMAIL_RCPT=`cat benchmarking/commit-author.txt`

echo "Hello" > letter.txt
cat ./benchmarking/result.txt >> letter.txt
echo "all results:" >> letter.txt
echo "http://bench.tarantool.org/?tab=tab-sysbench" >> letter.txt

curl --connect-timeout 15 -v \
    --insecure "smtp://smtp.mail.ru:2525" \
    -u "${EMAIL_LOGIN}:${EMAIL_PASSWORD}" \
    --mail-from "${EMAIL_LOGIN}" \
    --mail-rcpt "${EMAIL_RCPT}" \
    -T letter.txt --ssl --ipv4

