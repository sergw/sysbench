import sys
import re
import commands
from urllib import urlencode
import requests

# Microbench storage export tool for linkbench
# requirements: requests
# Usage:
# 1. Get auth token in microbench service
# 2. Create auth.conf file contains: [uri]:[token]
# 3. Run linkbench
# 4. export:
# python export.py [auth.conf] [benchmark output file] [metric group]
#
# results example: http://bench.farm.tarantool.org

def credentals(filename):
    conf = open(filename, 'r')
    auth_data = conf.readline()[:-1]
    conf.close()
    return auth_data.split(':')

def parse_bench(filename):
    fileHandle = open(filename)
    lastline = fileHandle.readlines()
    fileHandle.close()
    return lastline

def push(server, token, name, value, version, unit='trps', tab='sysbench'):
    uri = 'http://%s/push?%s' % (server, urlencode(dict(
        key=token, name=name, param=value,
        v=version, unit=unit, tab=tab
    )))

    r = requests.get(uri)
    if r.status_code == 200:
        print 'Export complete'
    else:
        print 'Export error http: %d' % r.status_code

def get_version(filename):
    fileHandle = open(filename)
    lastline = fileHandle.readlines()[-1]
    fileHandle.close()
    return lastline.split()[0]

def main():
    if len(sys.argv) < 4:
        print 'Usage:\n./export.py [auth.conf] [output.file] [tarantool-version.file]'
        return 0

    server, token = credentals(sys.argv[1])
    values = parse_bench(sys.argv[2])
    version = get_version(sys.argv[3])

    # push bench data to result server
    for value in values:
        push(server, token, value.split(":")[0], value.split(":")[1], version)
    
    return 0

if __name__ == '__main__':
    main()
