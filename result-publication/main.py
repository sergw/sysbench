import os
import sys


def main():
    if "MICROB_WEB_HOST" not in os.environ:
        print("MICROB_WEB_HOST not specified")
        exit(-1)

    if "MICROB_WEB_TOKEN" not in os.environ:
        print("MICROB_WEB_TOKEN not specified")
        exit(-1)

    if len(sys.argv) < 3:
        print('Usage:\n./main.py [benchmarks-results.file] [tarantool-version.file]')
        exit(-1)

    print("Hello world")


if __name__ == '__main__':
    main()