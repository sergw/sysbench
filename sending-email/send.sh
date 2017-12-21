#!/usr/bin/env bash

if [ ! -n "${MY_EMAIL}" ]; then exit 0; fi

cat ./benchmarking/result.txt > letter.txt
echo "all results:" >> letter.txt
echo "http://bench.tarantool.org/?tab=tab-sysbench" >> letter.txt

cat letter.txt | mutt -s "sysbench result for $(cat ./benchmarking/version.txt)" -- ${MY_EMAIL}

service postfix restart
