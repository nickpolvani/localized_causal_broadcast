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

were_errors = False

def vec_leq(vec1, vec2):
  """ Check that vec1 is less than vec2 elemtwise """
  assert isinstance(vec1, list), "Only work with lists"
  assert isinstance(vec2, list), "Only work with lists"
  assert len(vec1) == len(vec2), "Vector lengths should be the same"
  for x, y in zip(vec1, vec2):
    # if a coordinate is greater, returning false
    if x > y: return False

  # returning True if all xs are <= than ys
  return True

  
def soft_assert(condition, message = None):
    """ Print message if there was an error without exiting """
    global were_errors
    if not condition:
        if message:
            print("ASSERT failed " + message)
        were_errors = True


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
    config_file = open(config_path, "r")
    for i, line in enumerate(config_file):
        if i == 0:
            num_messages = int(line)
            continue
        localities[i] = [int(x) for x in line.split()][1:]
    num_processes = i
    return num_processes, num_messages, localities


  # CRB5: Causal delivery: For any message m1 that potentially caused a message m2, i.e., m1 -> m2 , no process delivers m2 unless it has already delivered m1.
  # (a) some process p broadcasts m1 before it broadcasts m2 ;
  # (b) some process p delivers m1 from a process (LOCALIZED) IT DEPENDS ON and subsequently broadcasts m2; or
  # (c) there exists some message m such that m1 -> m and m -> m2.
  # Process has dependencies dependencies[p] and itself

def check_causal(out_paths: list, localities: dict):
    events = {i: [] for i in range(1, len(out_paths) + 1)}
    for out_path in out_paths:
        process_num = int(out_path.split("/")[-1].split(".")[0])
        out_file = open(out_path, "r")
        for line in out_file:
            tokens = line.split()
            if tokens[0] == "b":
                events[process_num].append((tokens[0], process_num,  int(tokens[1])))
            elif tokens[0] == "d":
                events[process_num].append((tokens[0], int(tokens[1]), int(tokens[2])))
            else:
                raise ValueError("Unexpected token: ", tokens)
        
    msg_vc = {}
    for p in range(1, len(out_paths) + 1):
         # current vector clock for a message (dependencies of a newly sent message)
        v_send = [0 for _ in range(len(out_paths))]
        seqnum = 0

        # going over events
        for event in events[p]:
            # current message and type of event
            type_, msg = event[0], event[1:]

            # broadcast case: incrementing v_send
            if type_ == 'b':
                # copying v_send
                W = [x for x in v_send]

                # filling in seqnum
                W[p - 1] = seqnum

                # incrementing seqnum
                seqnum += 1

                # copying W to msg_vc
                msg_vc[msg] = [x for x in W]

            # delivery case: incrementing v_send if depend on the sender
            if type_ == 'd' and msg[0] in localities[p]:
                v_send[msg[0] - 1] += 1

    # PROPERTY TEST: for each process, for each delivery, must have W <= V_recv
    for p in range(1, len(out_paths) + 1):
        # currently delivered messages by process
        v_recv = [0 for _ in range(len(out_paths))]

        # loop over events
        for event in events[p]:
            # only care about deliveries here
            if event[0] != 'd': continue

            # parsing message = (sender, seq)
            msg = event[1:]

            # sanity check
            #print(msg)
            #print(msg_vc)
            assert msg in msg_vc, "Must have a vector clock for %s" % str(msg)
            # property test
            soft_assert(vec_leq(msg_vc[msg], v_recv), "CRB5 violated: Process %d have delivered %s with vector clock W = %s having V_recv = %s" % (p, str(msg), str(msg_vc[msg]), str(v_recv)))

            # incrementing v_recv
            v_recv[msg[0] - 1] += 1

  
    # printing the last line with status
    print("INCORRECT" if were_errors else "CORRECT")

    return not were_errors


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--config_file",
        required=True,
        type=check_file,
        dest="config_file",
        help="Path of config file",
    )

    parser.add_argument("--out_dir",
        required=True,
        type=check_dir,
        dest="out_dir",
        help="Output directory",)

    results = parser.parse_args()

    out_files = glob.glob(os.path.join(results.out_dir, "*.output"))

    num_processes, num_messages, localities = read_config(results.config_file)

    assert(len(out_files) == num_processes)
    print(num_processes)
    num_failures = 0
    for o in out_files:
        #print("Checking {}".format(o))
        if checkProcess(o):
            continue
            #print("Validation OK")
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
    check_causal(out_files, localities)


