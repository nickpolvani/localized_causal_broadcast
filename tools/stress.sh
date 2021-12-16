#!/bin/bash

# Change the current working directory to the location of the present file
cd "$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
rm ../example/output/*
python3 ./stress.py -r ../template_cpp/run.sh -t fifo -l ../example/output -p 64 -m 1000000