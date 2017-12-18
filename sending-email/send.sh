#!/usr/bin/env bash

if [ ! -n "${MY_EMAIL}" ]; then exit 0; fi

/etc/init.d/postfix start

cat result.txt > letter.txt
echo "all results:" >> letter.txt
echo "http://bench.tarantool.org/?tab=tab-sysbench" >> letter.txt
echo "" >> letter.txt

cat letter.txt | mail -s "sysbench result for $(cat version.txt)" ${MY_EMAIL}
