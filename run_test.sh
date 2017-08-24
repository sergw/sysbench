#!/bin/bash
echo "Bash version ${BASH_VERSION}..."

let min_threads=1
let max_threads=64

for i in "$@"
do
case $i in

    --db=*)
    db="${i#*=}"
    ;;

    --max_threads=*)
    max_threads="${i#*=}"
    ;;

    --min_threads=*)
    min_threads="${i#*=}"
    ;;

    --dir=*)
    dir="${i#*=}"
    ;;

    --pgsql_password=*)
    pgsql_password="${i#*=}"
    ;;

    --mysql_password=*)
    mysql_password="${i#*=}"
    ;;

    --tarantool_password=*)
    tarantool_password="${i#*=}"
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

if [ -v $test ]; then
    ARRAY_TESTS=("oltp_read_only" "oltp_point_select" "oltp_insert" "oltp_update_index" "oltp_update_non_index" "select_random_points" "select_random_ranges")
else
    ARRAY_TESTS=($test)
fi

if [ -v $db ]; then
    ARRAY_DB=("tarantool" "mysql" "pgsql")
else
    ARRAY_DB=($db)
fi

for test in "${ARRAY_TESTS[@]}"; do
    echo $'\n'"TEST:" $test

    i=$min_threads
    while [ $i -le $max_threads ];  do
    echo $'\n'"threads:" $i

        for db in "${ARRAY_DB[@]}"; do
            echo $'\n'"    DB:" $db

            ./src/sysbench $test --db-driver=$db --pgsql-password=$pgsql_password --mysql-password=$mysql_password --tarantool-password=$tarantool_password --threads=$i cleanup > /dev/null
            ./src/sysbench $test --db-driver=$db --pgsql-password=$pgsql_password --mysql-password=$mysql_password --tarantool-password=$tarantool_password --threads=$i prepare > /dev/null
            ./src/sysbench $test --db-driver=$db --pgsql-password=$pgsql_password --mysql-password=$mysql_password --tarantool-password=$tarantool_password --threads=$i run | grep -e 'transactions:'
            ./src/sysbench $test --db-driver=$db --pgsql-password=$pgsql_password --mysql-password=$mysql_password --tarantool-password=$tarantool_password --threads=$i cleanup > /dev/null
        done

        let i=i*2

    done
done
