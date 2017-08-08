#!/bin/bash
echo "Bash version ${BASH_VERSION}..."

for i in "$@"
do
case $i in

    --db=*)
    db="${i#*=}"
    ;;

    --dir=*)
    dir="${i#*=}"
    ;;


    --test=*)
    test="${i#*=}"
    ;;
    *)
            # unknown option
    ;;
esac
done

cd $dir
i=1
while [ $i -lt 513 ];
do 
    ./src/sysbench $test --db-driver=$db --pgsql-password=1480 --threads=$i cleanup
    ./src/sysbench $test --db-driver=$db --pgsql-password=1480 --threads=$i prepare
    ./src/sysbench $test --db-driver=$db --pgsql-password=1480 --threads=$i run
	./src/sysbench $test --db-driver=$db --pgsql-password=1480 --threads=$i cleanup
	let i=i*2
done
