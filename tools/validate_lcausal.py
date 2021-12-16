#!/usr/bin/env python3

import argparse
import os, atexit
import textwrap


import signal
import random
import time
from enum import Enum
import glob

from collections import defaultdict, OrderedDict



DELIVERED_MESSAGES = 0

def check_positive(value):
    ivalue = int(value)
    if ivalue <= 0:
        raise argparse.ArgumentTypeError("{} is an invalid positive int value".format(value))
    return ivalue


def checkProcess(filePath):
    global DELIVERED_MESSAGES
    i = 1
    nextMessage = defaultdict(lambda : 1)
    filename = os.path.basename(filePath)

    with open(filePath) as f:
        for lineNumber, line in enumerate(f):
            tokens = line.split()

            # Check broadcast
            if tokens[0] == 'b':
                msg = int(tokens[1])
                if msg != i:
                    print("File {}, Line {}: Messages broadcast out of order. Expected message {} but broadcast message {}".format(filename, lineNumber, i, msg))
                    return False
                i += 1

            # Check delivery
            if tokens[0] == 'd':
                sender = int(tokens[1])
                msg = int(tokens[2])
                if msg != nextMessage[sender]:
                    print("File {}, Line {}: Message delivered out of order. Expected message {}, but delivered message {}".format(filename, lineNumber, nextMessage[sender], msg))
                    return False
                else:
                    nextMessage[sender] = msg + 1
                    DELIVERED_MESSAGES += 1
    return True


def check_dir(path):
    if os.path.isdir(path):
        return path
    else:
        raise ValueError(path, "is not a directory")

def check_file(path):
    if os.path.isfile(path):
        return path
    else:
        raise ValueError(path, "is not a file")


def read_config(config_path):
    localities = {}
    num_processes = 0
    return num_processes, localities



def check_causal(out_dir: str, localities: dict):
    return


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--config_file",
        required=True,
        type=check_file,
        dest="proc_num",
        help="Path of config file",
    )

    parser.add_argument("--out_dir",
        required=True,
        type=check_dir,
        dest="out_dir",
        help="Output directory",)

    results = parser.parse_args()

    out_files = glob.glob(os.path.join(results.out_dir, "*.output"))

    process_numbers, localities = read_config(results.config_file)
    
    num_failures = 0
    for o in out_files:
        print("Checking {}".format(o))
        if checkProcess(o):
            print("Validation OK")
        else:
            print("Validation failed!")
            num_failures += 1

    if num_failures > 0:
        print("Number of failures detected for FIFO property:", num_failures)
        exit(1)
    else:
        print("All output files respect the FIFO Property")
        print("Number of delivered messages in total:", DELIVERED_MESSAGES)

    print("Checking Lcausal property...")
    check_causal(results.out_dir, localities)


