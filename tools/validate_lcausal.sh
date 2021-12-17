#!/bin/bash

# Change the current working directory to the location of the present file
cd "$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

echo Validating FIFO... 

python validate_lcausal.py --config_file ../example/output/config --out_dir ../example/output