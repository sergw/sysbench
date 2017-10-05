#!/bin/bash

if [ ! -d "$tarantool" ]; then
	git clone --recursive https://github.com/tarantool/tarantool.git -b 1.8 tarantool
fi

cd tarantool; git pull; cmake .; make; cd ..;

./autogen.sh; ./configure --with-tarantool; make;

./tarantool/src/tarantool start-server.lua &
sleep 2
TNT_PID=$!

echo "test_name:result[trps]"
./testing-tnt.sh --port=3301 --threads=1 | tee result.txt

kill $TNT_PID
wait
rm *.xlog
rm *.snap

python export.py auth.conf result.txt
